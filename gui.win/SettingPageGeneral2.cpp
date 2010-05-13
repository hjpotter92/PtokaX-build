/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2010  Petr Kozelka, PPK at PtokaX dot org

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
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "SettingPageGeneral2.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageGeneral2::SettingPageGeneral2() {
    bUpdateTextFiles = bUpdateRedirectAddress = bUpdateRegOnlyMessage = bUpdateShareLimitMessage = bUpdateSlotsLimitMessage = false;
    bUpdateHubSlotRatioMessage = bUpdateMaxHubsLimitMessage = bUpdateNoTagMessage = bUpdateTempBanRedirAddress = bUpdatePermBanRedirAddress = false;
    bUpdateNickLimitMessage = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageGeneral2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_OWNER_EMAIL:
            case EDT_MAIN_REDIR_ADDR:
            case EDT_MSG_TO_NON_REGS:
            case EDT_NON_REG_REDIR_ADDR:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
            case BTN_ENABLE_TEXT_FILES:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM],
                        ::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case BTN_DONT_ALLOW_PINGER:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
                    ::EnableWindow(hWndPageItems[BTN_REPORT_PINGER], bEnable);
                    ::EnableWindow(hWndPageItems[EDT_OWNER_EMAIL], bEnable);
                }

                break;
            case BTN_REDIR_ALL:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[BTN_REDIR_HUB_FULL],
                        ::SendMessage(hWndPageItems[BTN_REDIR_ALL], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE);
                }

                break;
            case BTN_ALLOW_ONLY_REGS:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                    ::EnableWindow(hWndPageItems[EDT_MSG_TO_NON_REGS], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], bEnable);
                    ::EnableWindow(hWndPageItems[EDT_NON_REG_REDIR_ADDR],
                        (bEnable == TRUE && ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE);
                }

                break;
            case BTN_NON_REG_REDIR_ENABLE:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[EDT_NON_REG_REDIR_ADDR],
                        ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
        }
    }


	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::Save() {
    if(bCreated == false) {
        return;
    }

    if((::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES]) {
        bUpdateTextFiles = true;
    }

    SettingManager->SetBool(SETBOOL_ENABLE_TEXT_FILES, ::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_SEND_TEXT_FILES_AS_PM, ::SendMessage(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager->SetBool(SETBOOL_DONT_ALLOW_PINGERS, ::SendMessage(hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_REPORT_PINGERS, ::SendMessage(hWndPageItems[BTN_REPORT_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    char buf[257];
    int iLen = ::GetWindowText(hWndPageItems[EDT_OWNER_EMAIL], buf, 257);
    SettingManager->SetText(SETTXT_HUB_OWNER_EMAIL, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_MAIN_REDIR_ADDR], buf, 257);

    if((SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS]) != NULL)) {
        bUpdateRedirectAddress = true;
        bUpdateRegOnlyMessage = true;
        bUpdateShareLimitMessage = true;
        bUpdateSlotsLimitMessage = true;
        bUpdateHubSlotRatioMessage = true;
        bUpdateMaxHubsLimitMessage = true;
        bUpdateNoTagMessage = true;
        bUpdateTempBanRedirAddress = true;
		bUpdatePermBanRedirAddress = true;
		bUpdateNickLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_REDIRECT_ADDRESS, buf, iLen);

    SettingManager->SetBool(SETBOOL_REDIRECT_ALL, ::SendMessage(hWndPageItems[BTN_REDIR_ALL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_REDIRECT_WHEN_HUB_FULL, ::SendMessage(hWndPageItems[BTN_REDIR_HUB_FULL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager->SetBool(SETBOOL_REG_ONLY, ::SendMessage(hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    iLen = ::GetWindowText(hWndPageItems[EDT_MSG_TO_NON_REGS], buf, 257);

    if(bUpdateRegOnlyMessage == false &&
        ((::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager->bBools[SETBOOL_REG_ONLY_REDIR] ||
        strcmp(buf, SettingManager->sTexts[SETTXT_REG_ONLY_MSG]) != NULL)) {
        bUpdateRegOnlyMessage = true;
    }

    SettingManager->SetText(SETTXT_REG_ONLY_MSG, buf, iLen);

    SettingManager->SetBool(SETBOOL_REG_ONLY_REDIR, ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    iLen = ::GetWindowText(hWndPageItems[EDT_NON_REG_REDIR_ADDR], buf, 257);

    if(bUpdateRegOnlyMessage == false &&
        (SettingManager->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS]) != NULL)) {
        bUpdateRegOnlyMessage = true;
    }

    SettingManager->SetText(SETTXT_REG_ONLY_REDIR_ADDRESS, buf, iLen);

    SettingManager->SetBool(SETBOOL_NO_QUACK_SUPPORTS, ::SendMessage(hWndPageItems[BTN_KILL_THAT_DUCK], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
    bool & /*bUpdatedAutoReg*/, bool &/*bUpdatedMOTD*/, bool &/*bUpdatedHubSec*/, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
    bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
    bool &bUpdatedNickLimitMessage, bool &/*bUpdatedBotsSameNick*/, bool &/*bUpdatedBotNick*/, bool &/*bUpdatedBot*/, bool &/*bUpdatedOpChatNick*/,
    bool &/*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool &bUpdatedTextFiles, bool &bUpdatedRedirectAddress, bool &bUpdatedTempBanRedirAddress,
    bool &bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
    bUpdatedTextFiles = bUpdateTextFiles;
    bUpdatedRedirectAddress = bUpdateRedirectAddress;
    bUpdatedRegOnlyMessage = bUpdateRegOnlyMessage;
    bUpdatedShareLimitMessage = bUpdateShareLimitMessage;
    bUpdatedSlotsLimitMessage = bUpdateSlotsLimitMessage;
    bUpdatedHubSlotRatioMessage = bUpdateHubSlotRatioMessage;
    bUpdatedMaxHubsLimitMessage = bUpdateMaxHubsLimitMessage;
    bUpdatedNoTagMessage = bUpdateNoTagMessage;
    bUpdatedTempBanRedirAddress = bUpdateTempBanRedirAddress;
    bUpdatedPermBanRedirAddress = bUpdatePermBanRedirAddress;
    bUpdatedNickLimitMessage = bUpdateNickLimitMessage;
}

//------------------------------------------------------------------------------
bool SettingPageGeneral2::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_TEXT_FILES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_TEXT_FILES], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 55,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_ENABLE_TEXT_FILES] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_TEXT_FILES], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 17, 431, 16, m_hWnd, (HMENU)BTN_ENABLE_TEXT_FILES, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_SETCHECK, (SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_TEXT_FILES_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 36, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], BM_SETCHECK, (SettingManager->bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_PINGER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PINGER], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 58, 447, 96,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_DONT_ALLOW_PINGER] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISALLOW_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 72, 431, 16, m_hWnd, (HMENU)BTN_DONT_ALLOW_PINGER, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_SETCHECK, (SettingManager->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_REPORT_PINGER] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REPORT_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 91, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REPORT_PINGER], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REPORT_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_OWNER_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_OWNER_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 108, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_OWNER_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_OWNER_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 13, 123, 421, 18, m_hWnd, (HMENU)EDT_OWNER_EMAIL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_OWNER_EMAIL], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[GB_MAIN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAIN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 154, 447, 79,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 8, 169, 431, 18, m_hWnd, (HMENU)EDT_MAIN_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[BTN_REDIR_ALL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ALL_CONN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 192, 431, 16, m_hWnd, (HMENU)BTN_REDIR_ALL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REDIR_ALL], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REDIRECT_ALL] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_REDIR_HUB_FULL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_FULL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 211, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REDIR_HUB_FULL], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_REG_ONLY_HUB] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REG_ONLY_HUB], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 233, 447, 119,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_ALLOW_ONLY_REGS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ALLOW_ONLY_REGS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 247, 431, 16, m_hWnd, (HMENU)BTN_ALLOW_ONLY_REGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REG_ONLY] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_MSG_TO_NON_REGS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REG_ONLY_MSG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 264, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MSG_TO_NON_REGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_REG_ONLY_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 13, 279, 421, 18, m_hWnd, (HMENU)EDT_MSG_TO_NON_REGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MSG_TO_NON_REGS], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[GB_NON_REG_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 305, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_NON_REG_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 322, 80, 16, m_hWnd, (HMENU)BTN_NON_REG_REDIR_ENABLE, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REG_ONLY_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[EDT_NON_REG_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | ES_AUTOHSCROLL, 98, 320, 336, 18, m_hWnd, (HMENU)EDT_NON_REG_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NON_REG_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[GB_KILL_THAT_DUCK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_CLIENTS_BUGGY_SUPPORTS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 352, 447, 36, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_KILL_THAT_DUCK] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISALLOW_BUGGY_SUPPORTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 366, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_KILL_THAT_DUCK], BM_SETCHECK, (SettingManager->bBools[SETBOOL_NO_QUACK_SUPPORTS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", GetPageName(), MB_OK);
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], SettingManager->bBools[SETBOOL_ENABLE_TEXT_FILES] == true ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[BTN_REPORT_PINGER], SettingManager->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_OWNER_EMAIL], SettingManager->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[BTN_REDIR_HUB_FULL], SettingManager->bBools[SETBOOL_REDIRECT_ALL] == true ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_MSG_TO_NON_REGS], SettingManager->bBools[SETBOOL_REG_ONLY] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], SettingManager->bBools[SETBOOL_REG_ONLY] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[EDT_NON_REG_REDIR_ADDR],
        (SettingManager->bBools[SETBOOL_REG_ONLY] == true && SettingManager->bBools[SETBOOL_REG_ONLY_REDIR] == true) ? TRUE : FALSE);

    ::SetWindowLongPtr(hWndPageItems[BTN_KILL_THAT_DUCK], GWLP_USERDATA, (LONG_PTR)this);
    wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_KILL_THAT_DUCK], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral2::GetPageName() {
    return LanguageManager->sTexts[LAN_MORE_GENERAL];
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::FocusLastItem() {
    ::SetFocus(hWndPageItems[BTN_KILL_THAT_DUCK]);
}
//------------------------------------------------------------------------------