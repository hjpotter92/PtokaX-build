/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2012  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "Utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "UpdateCheckThread.h"
//---------------------------------------------------------------------------
#include "../gui.win/MainWindow.h"
//---------------------------------------------------------------------------
UpdateCheckThread * pUpdateCheckThread = NULL;
//---------------------------------------------------------------------------

UpdateCheckThread::UpdateCheckThread() {
    sSocket = INVALID_SOCKET;

	ui32FileLen = 0;

	ui32RecvBufLen = ui32RecvBufSize = 0;
    ui32BytesRead = ui32BytesSent = 0;

    hThread = INVALID_HANDLE_VALUE;

	sRecvBuf = NULL;

	bOk = bData = bTerminated = false;

    sMsg[0] = '\0';
}
//---------------------------------------------------------------------------

UpdateCheckThread::~UpdateCheckThread() {
	ui64BytesRead += (uint64_t)ui32BytesRead;
    ui64BytesSent += (uint64_t)ui32BytesSent;

    if(sSocket != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(sSocket, SD_SEND);
		closesocket(sSocket);
#else
        shutdown(sSocket, 1);
        close(sSocket);
#endif
        sSocket = INVALID_SOCKET;
    }

    free(sRecvBuf);

    if(hThread != INVALID_HANDLE_VALUE) {
        ::CloseHandle(hThread);
    }
}
//---------------------------------------------------------------------------

unsigned __stdcall ExecuteUpdateCheck(void * /*pArguments*/) {
	pUpdateCheckThread->Run();

	return 0;
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Resume() {
	hThread = (HANDLE)_beginthreadex(NULL, 0, ExecuteUpdateCheck, NULL, 0, NULL);
	if(hThread == 0) {
		AppendDebugLog("%s - [ERR] Failed to create new UpdateCheckThread\n", 0);
    }
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Run() {
    struct addrinfo * pResult = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_socktype = SOCK_STREAM;

    if(bUseIPv6 == true) {
        hints.ai_family = AF_UNSPEC;
    } else {
        hints.ai_family = AF_INET;
    }

    if(::getaddrinfo("www.PtokaX.org", "80", &hints, &pResult) != 0 || (pResult->ai_family != AF_INET && pResult->ai_family != AF_INET6)) {
        int iError = WSAGetLastError();
        int iMsgLen = sprintf(sMsg, "Update check resolve error %s (%d).", WSErrorStr(iError), iError);
        if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Run") == true) {
            Message(sMsg, iMsgLen);
        }

        ::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        if(pResult != NULL) {
            ::freeaddrinfo(pResult);
        }

        return;
    }

    // Initialise socket we want to use for connect
#ifdef _WIN32
    if((sSocket = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol)) == INVALID_SOCKET) {
        int iError = WSAGetLastError();
        int iMsgLen = sprintf(sMsg, "Update check create error %s (%d).", WSErrorStr(iError), iError);
#else
    if((sSocket = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol)) == -1) {
        int iMsgLen = sprintf(sMsg, "Update check create error %s (%d).", WSErrorStr(errno), errno);
#endif
        if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Run1") == true) {
            Message(sMsg, iMsgLen);
        }

        ::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        ::freeaddrinfo(pResult);

        return;
    }

    // Set the receive buffer
    int32_t bufsize = 8192;
#ifdef _WIN32
    if(setsockopt(sSocket, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        int iMsgLen = sprintf(sMsg, "Update check recv buff error %s (%d).", WSErrorStr(iError), iError);
#else
    if(setsockopt(sSocket, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
        int iMsgLen = sprintf(sMsg, "Update check recv buff error %s (%d).", WSErrorStr(errno), errno);
#endif
		if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Run2") == true) {
			Message(sMsg, iMsgLen);
		}

		::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        ::freeaddrinfo(pResult);

        return;
	}

	// Set the send buffer
	bufsize = 2048;
#ifdef _WIN32
	if(setsockopt(sSocket, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
		int iError = WSAGetLastError();
		int iMsgLen = sprintf(sMsg, "Update check send buff error %s (%d).", WSErrorStr(iError), iError);
#else
	if(setsockopt(sSocket, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        int iMsgLen = sprintf(sMsg, "Update check buff error %s (%d).", WSErrorStr(errno), errno);
#endif
        if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Run3") == true) {
            Message(sMsg, iMsgLen);
		}
			
		::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        ::freeaddrinfo(pResult);

        return;
    }

	Message("Connecting to PtokaX.org ...", 28);

    // Finally, time to connect ! ;)
#ifdef _WIN32
    if(connect(sSocket, pResult->ai_addr, (int)pResult->ai_addrlen) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
            int iMsgLen = sprintf(sMsg, "Update check connect error %s (%d).", WSErrorStr(iError), iError);
#else
    if(connect(sSocket, pResult->ai_addr, (int)pResult->ai_addrlen) == -1) {
        if(errno != EAGAIN) {
            int iMsgLen = sprintf(sMsg, "Update check connect error %s (%d).", WSErrorStr(errno), errno);
#endif
            if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Run4") == true) {
                Message(sMsg, iMsgLen);
            }

			::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

            ::freeaddrinfo(pResult);

            return;
        }
    }

    ::freeaddrinfo(pResult);

	Message("Connected to PtokaX.org, sending request...", 43);

    if(SendHeader() == false) {
        ::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        return;
    }

	Message("Request to PtokaX.org sent, receiving data...", 45);

    // Set non-blocking mode
#ifdef _WIN32
    uint32_t block = 1;
    if(ioctlsocket(sSocket, FIONBIO, (unsigned long *)&block) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        int iMsgLen = sprintf(sMsg, "Update check non-block error %s (%d).", WSErrorStr(iError), iError);
#else
    int32_t oldFlag = fcntl(u->s, F_GETFL, 0);
    if(fcntl(sSocket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
        int iMsgLen = sprintf(sMsg, "Update check non-block error %s (%d).", WSErrorStr(errno), errno);
#endif
        if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Run5") == true) {
            Message(sMsg, iMsgLen);
		}

		::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        return;
    }

    sRecvBuf = (char *)malloc(512);
    if(sRecvBuf == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 512 bytes for sRecvBuf in UpdateCheckThread::Run\n", 0);

		::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        return;
    }

    uint16_t iLoops = 0;

    while(bTerminated == false && iLoops < 4000) {
        iLoops++;

		if(Receive() == false) {
			::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

			return;
        }

        ::Sleep(75);
    }

    if(bTerminated == false) {
        Message("Update check timeout.", 21);

		::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);
    }
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Close() {
	bTerminated = true;
}
//---------------------------------------------------------------------------

void UpdateCheckThread::WaitFor() {
    ::WaitForSingleObject(hThread, INFINITE);
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Message(char * sMessage, const size_t &szLen) const {
	char *sMess = (char *)malloc(szLen + 1);
	if(sMess == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMess in UpdateCheckThread::Message\n", (uint64_t)(szLen+1));

		return;
	}

	memcpy(sMess, sMessage, szLen);
	sMess[szLen] = '\0';

	::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_MSG, 0, (LPARAM)sMess);
}
//---------------------------------------------------------------------------

bool UpdateCheckThread::SendHeader() {
	char * sDataToSend = "GET /version HTTP/1.1\r\nUser-Agent: PtokaX " PtokaXVersionString " [" BUILD_NUMBER "]"
		"\r\nHost: www.PtokaX.org\r\nConnection: close\r\nCache-Control: no-cache\r\nAccept: */*\r\nAccept-Language: en\r\n\r\n";

	int iBytes = send(sSocket, sDataToSend, (int)strlen(sDataToSend), 0);

#ifdef _WIN32
    if(iBytes == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
        int iMsgLen = sprintf(sMsg, "Update check send error %s (%d).", WSErrorStr(iError), iError);
#else
    if(iBytes == -1) {
        int iMsgLen = sprintf(sMsg, "Update check send error %s (%d).)", WSErrorStr(errno), errno);
#endif
        if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::SendHeader") == true) {
            Message(sMsg, iMsgLen);
        }

        return false;
    }

    ui32BytesSent += iBytes;
    
    return true;
}
//---------------------------------------------------------------------------

bool UpdateCheckThread::Receive() {
    u_long ui32bytes = 0;

	if(ioctlsocket(sSocket, FIONREAD, &ui32bytes) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
	    int iMsgLen = sprintf(sMsg, "Update check ioctlsocket(FIONREAD) error %s (%d).", WSErrorStr(iError), iError);
        if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Receive") == true) {
			Message(sMsg, iMsgLen);
        }

        return false;
    }
    
    if(ui32bytes == 0) {
        // we need to try receive to catch connection error, or if server closed connection
        ui32bytes = 16;
    } else if(ui32bytes > 8192) {
        // receive max. 8192 bytes to receive buffer
        ui32bytes = 8192;
    }

    if(ui32RecvBufSize < ui32RecvBufLen + ui32bytes) {
        size_t szAllignLen = ((ui32RecvBufLen + ui32bytes + 1) & 0xFFFFFE00) + 0x200;

        char * pOldBuf = sRecvBuf;

        sRecvBuf = (char *)realloc(sRecvBuf, szAllignLen);
        if(sRecvBuf == NULL) {
            sRecvBuf = pOldBuf;

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for sRecvBuf in UpdateCheckThread::Receive\n", (uint64_t)szAllignLen);

            return false;
        }

        ui32RecvBufSize = (int)szAllignLen - 1;
    }

    int iBytes = recv(sSocket, sRecvBuf + ui32RecvBufLen, ui32RecvBufSize - ui32RecvBufLen, 0);

#ifdef _WIN32
    if(iBytes == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {                  
			int iMsgLen = sprintf(sMsg, "Update check recv error %s (%d).", WSErrorStr(iError), iError);
#else
    if(iBytes == -1) {
        if(errno != EAGAIN) {
			int iMsgLen = sprintf(sMsg, "Update check recv error %s (%d).", WSErrorStr(errno), errno);
#endif
            if(CheckSprintf(iMsgLen, 2048, "UpdateCheckThread::Receive2") == true) {
                Message(sMsg, iMsgLen);
            }

            return false;
        } else {

            return true;
        }
    } else if(iBytes == 0) {
		Message("Update check closed connection by server.", 41);

		return false;
    }
    
    ui32BytesRead += iBytes;

	ui32RecvBufLen += iBytes;
	sRecvBuf[ui32RecvBufLen] = '\0';

	if(bData == false) {
		char *sBuffer = sRecvBuf;

		for(uint32_t ui32i = 0; ui32i < ui32RecvBufLen; ui32i++) {
			if(sRecvBuf[ui32i] == '\n') {
				sRecvBuf[ui32i] = '\0';
				uint32_t ui32iCommandLen = (uint32_t)((sRecvBuf+ui32i) - sBuffer) + 1;

				if(ui32iCommandLen > 7 && strncmp(sBuffer, "HTTP", 4) == NULL && strstr(sBuffer, "200") != NULL) {
					bOk = true;
				} else if(ui32iCommandLen == 2 && sBuffer[0] == '\r') {
					if(bOk == true && ui32FileLen != 0) {
						bData = true;
					} else {
						Message("Update check failed.", 20);
						::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

						return false;
                    }
				} else if(ui32iCommandLen > 16 && strncmp(sBuffer, "Content-Length: ", 16) == NULL) {
					ui32FileLen = atoi(sBuffer+16);
				}

				sBuffer += ui32iCommandLen;

				if(bData == true) {
					break;
                }
			}
		}

		ui32RecvBufLen -= (uint32_t)(sBuffer - sRecvBuf);

		if(ui32RecvBufLen == 0) {
			sRecvBuf[0] = '\0';
		} else if(ui32RecvBufLen != 1) {
			memmove(sRecvBuf, sBuffer, ui32RecvBufLen);
			sRecvBuf[ui32RecvBufLen] = '\0';
		} else {
			if(sBuffer[0] == '\n') {
				sRecvBuf[0] = '\0';
				ui32RecvBufLen = 0;
			} else {
				sRecvBuf[0] = sBuffer[0];
				sRecvBuf[1] = '\0';
			}
		}
	}

	if(bData == true) {
		if(ui32RecvBufLen == (uint32_t)ui32FileLen) {
			char *sMess = (char *)malloc(ui32RecvBufLen + 1);
			if(sMess == NULL) {
				AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sMess in UpdateCheckThread::Receive\n", (uint64_t)(ui32RecvBufLen+1));

				return false;
			}

			memcpy(sMess, sRecvBuf, ui32RecvBufLen);
			sMess[ui32RecvBufLen] = '\0';

			::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_DATA, 0, (LPARAM)sMess);

			::PostMessage(pMainWindow->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);
        }
    }

	return true;
}
//---------------------------------------------------------------------------
