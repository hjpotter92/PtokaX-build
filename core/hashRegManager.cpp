/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2015  Petr Kozelka, PPK at PtokaX dot org

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
#include "hashRegManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/RegisteredUserDialog.h"
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
clsRegManager * clsRegManager::mPtr = NULL;
//---------------------------------------------------------------------------
static const char sPtokaXRegiteredUsers[] = "PtokaX Registered Users";
static const size_t szPtokaXRegiteredUsersLen = sizeof(sPtokaXRegiteredUsers)-1;
//---------------------------------------------------------------------------

RegUser::RegUser() : tLastBadPass(0), sNick(NULL), pPrev(NULL), pNext(NULL), pHashTablePrev(NULL), pHashTableNext(NULL), ui32Hash(0), ui16Profile(0), ui8BadPassCount(0), bPassHash(false){
    sPass = NULL;
}
//---------------------------------------------------------------------------

RegUser::~RegUser() {
#ifdef _WIN32
    if(sNick != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate sNick in RegUser::~RegUser\n");
    }
#else
	free(sNick);
#endif

    if(bPassHash == true) {
#ifdef _WIN32
        if(ui8PassHash != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui8PassHash) == 0) {
		  AppendDebugLog("%s - [MEM] Cannot deallocate ui8PassHash in RegUser::~RegUser\n");
        }
#else
	   free(ui8PassHash);
#endif
    } else {
#ifdef _WIN32
        if(sPass != NULL && HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sPass) == 0) {
		  AppendDebugLog("%s - [MEM] Cannot deallocate sPass in RegUser::~RegUser\n");
        }
#else
	   free(sPass);
#endif
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RegUser * RegUser::CreateReg(char * sRegNick, size_t szRegNickLen, char * sRegPassword, size_t szRegPassLen, uint8_t * ui8RegPassHash, const uint16_t &ui16RegProfile) {
    RegUser * pReg = new (std::nothrow) RegUser();

    if(pReg == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new Reg in RegUser::CreateReg\n");

        return NULL;
    }

#ifdef _WIN32
    pReg->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szRegNickLen+1);
#else
	pReg->sNick = (char *)malloc(szRegNickLen+1);
#endif
    if(pReg->sNick == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for sNick in RegUser::RegUser\n", (uint64_t)(szRegNickLen+1));

        delete pReg;
        return NULL;
    }
    memcpy(pReg->sNick, sRegNick, szRegNickLen);
    pReg->sNick[szRegNickLen] = '\0';

    if(ui8RegPassHash != NULL) {
#ifdef _WIN32
        pReg->ui8PassHash = (uint8_t *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
        pReg->ui8PassHash = (uint8_t *)malloc(64);
#endif
        if(pReg->ui8PassHash == NULL) {
            AppendDebugLog("%s - [MEM] Cannot allocate 64 bytes for ui8PassHash in RegUser::RegUser\n");

            delete pReg;
            return NULL;
        }
        memcpy(pReg->ui8PassHash, ui8RegPassHash, 64);
        pReg->bPassHash = true;
    } else {
#ifdef _WIN32
        pReg->sPass = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szRegPassLen+1);
#else
        pReg->sPass = (char *)malloc(szRegPassLen+1);
#endif
        if(pReg->sPass == NULL) {
            AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for sPass in RegUser::RegUser\n", (uint64_t)(szRegPassLen+1));

            delete pReg;
            return NULL;
        }
        memcpy(pReg->sPass, sRegPassword, szRegPassLen);
        pReg->sPass[szRegPassLen] = '\0';
    }

    pReg->ui16Profile = ui16RegProfile;
	pReg->ui32Hash = HashNick(sRegNick, szRegNickLen);

    return pReg;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool RegUser::UpdatePassword(char * sNewPass, size_t &szNewLen) {
    if(clsSettingManager::mPtr->bBools[SETBOOL_HASH_PASSWORDS] == false) {
        if(bPassHash == true) {
            void * sOldBuf = ui8PassHash;
#ifdef _WIN32
            sPass = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldBuf, szNewLen+1);
#else
            sPass = (char *)realloc(sOldBuf, szNewLen+1);
#endif
            if(sPass == NULL) {
                ui8PassHash = (uint8_t *)sOldBuf;

                AppendDebugLogFormat("[MEM] Cannot reallocate %" PRIu64 " bytes for ui8PassHash->sPass in RegUser::UpdatePassword\n", (uint64_t)(szNewLen+1));

                return false;
            }
            memcpy(sPass, sNewPass, szNewLen);
            sPass[szNewLen] = '\0';

            bPassHash = false;
        } else if(strcmp(sPass, sNewPass) != 0) {
            char * sOldPass = sPass;
#ifdef _WIN32
            sPass = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPass, szNewLen+1);
#else
            sPass = (char *)realloc(sOldPass, szNewLen+1);
#endif
            if(sPass == NULL) {
                sPass = sOldPass;

                AppendDebugLogFormat("[MEM] Cannot reallocate %" PRIu64 " bytes for sPass in RegUser::UpdatePassword\n", (uint64_t)(szNewLen+1));

                return false;
            }
            memcpy(sPass, sNewPass, szNewLen);
            sPass[szNewLen] = '\0';
        }
    } else {
        if(bPassHash == true) {
            HashPassword(sNewPass, szNewLen, ui8PassHash);
        } else {
            char * sOldPass = sPass;
#ifdef _WIN32
            ui8PassHash = (uint8_t *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPass, 64);
#else
            ui8PassHash = (uint8_t *)realloc(sOldPass, 64);
#endif
            if(ui8PassHash == NULL) {
                sPass = sOldPass;

                AppendDebugLog("%s - [MEM] Cannot reallocate 64 bytes for sPass->ui8PassHash in RegUser::UpdatePassword\n");

                return false;
            }

            if(HashPassword(sNewPass, szNewLen, ui8PassHash) == true) {
                bPassHash = true;
            } else {
                sPass = (char *)ui8PassHash;
            }
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsRegManager::clsRegManager(void) : ui8SaveCalls(0), pRegListS(NULL), pRegListE(NULL) {
    memset(pTable, 0, sizeof(pTable));
}
//---------------------------------------------------------------------------

clsRegManager::~clsRegManager(void) {
    RegUser * curReg = NULL,
        * next = pRegListS;
        
    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;

		delete curReg;
    }
}
//---------------------------------------------------------------------------

bool clsRegManager::AddNew(char * sNick, char * sPasswd, const uint16_t &iProfile) {
    if(Find(sNick, strlen(sNick)) != NULL) {
        return false;
    }

    RegUser * pNewUser = NULL;

    if(clsSettingManager::mPtr->bBools[SETBOOL_HASH_PASSWORDS] == true) {
        uint8_t ui8Hash[64];

        size_t szPassLen = strlen(sPasswd);

        if(HashPassword(sPasswd, szPassLen, ui8Hash) == false) {
            return false;
        }

        pNewUser = RegUser::CreateReg(sNick, strlen(sNick), NULL, 0, ui8Hash, iProfile);
    } else {
        pNewUser = RegUser::CreateReg(sNick, strlen(sNick), sPasswd, strlen(sPasswd), NULL, iProfile);
    }

    if(pNewUser == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in clsRegManager::AddNew\n");
 
        return false;
    }

	Add(pNewUser);

#ifdef _BUILD_GUI
    if(clsRegisteredUsersDialog::mPtr != NULL) {
        clsRegisteredUsersDialog::mPtr->AddReg(pNewUser);
    }
#endif

    Save(true);

    if(clsServerManager::bServerRunning == false) {
        return true;
    }

	User * AddedUser = clsHashManager::mPtr->FindUser(pNewUser->sNick, strlen(pNewUser->sNick));

    if(AddedUser != NULL) {
        bool bAllowedOpChat = clsProfileManager::mPtr->IsAllowed(AddedUser, clsProfileManager::ALLOWEDOPCHAT);
        AddedUser->i32Profile = iProfile;

        if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            if(clsProfileManager::mPtr->IsAllowed(AddedUser, clsProfileManager::HASKEYICON) == true) {
                AddedUser->ui32BoolBits |= User::BIT_OPERATOR;
            } else {
                AddedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if(((AddedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
				clsUsers::mPtr->Add2OpList(AddedUser);
                clsGlobalDataQueue::mPtr->OpListStore(AddedUser->sNick);

                if(bAllowedOpChat != clsProfileManager::mPtr->IsAllowed(AddedUser, clsProfileManager::ALLOWEDOPCHAT)) {
					if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true &&
                        (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                        if(((AddedUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                            AddedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO],
                                clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                        }

                        AddedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                        AddedUser->SendFormat("clsRegManager::AddNew", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                    }
                }
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void clsRegManager::Add(RegUser * Reg) {
	Add2Table(Reg);
    
    if(pRegListE == NULL) {
    	pRegListS = Reg;
    	pRegListE = Reg;
    } else {
        Reg->pPrev = pRegListE;
    	pRegListE->pNext = Reg;
        pRegListE = Reg;
    }

	return;
}
//---------------------------------------------------------------------------

void clsRegManager::Add2Table(RegUser * Reg) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &Reg->ui32Hash, sizeof(uint16_t));

    if(pTable[ui16dx] != NULL) {
        pTable[ui16dx]->pHashTablePrev = Reg;
        Reg->pHashTableNext = pTable[ui16dx];
    }
    
    pTable[ui16dx] = Reg;
}
//---------------------------------------------------------------------------

void clsRegManager::ChangeReg(RegUser * pReg, char * sNewPasswd, const uint16_t &ui16NewProfile) {
    if(sNewPasswd != NULL) {
        size_t szPassLen = strlen(sNewPasswd);

        pReg->UpdatePassword(sNewPasswd, szPassLen);
    }

    pReg->ui16Profile = ui16NewProfile;

#ifdef _BUILD_GUI
    if(clsRegisteredUsersDialog::mPtr != NULL) {
        clsRegisteredUsersDialog::mPtr->RemoveReg(pReg);
        clsRegisteredUsersDialog::mPtr->AddReg(pReg);
    }
#endif

    clsRegManager::mPtr->Save(true);

    if(clsServerManager::bServerRunning == false) {
        return;
    }

    User *ChangedUser = clsHashManager::mPtr->FindUser(pReg->sNick, strlen(pReg->sNick));
    if(ChangedUser != NULL && ChangedUser->i32Profile != (int32_t)ui16NewProfile) {
        bool bAllowedOpChat = clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::ALLOWEDOPCHAT);

        ChangedUser->i32Profile = (int32_t)ui16NewProfile;

        if(((ChangedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) !=
            clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::HASKEYICON)) {
            if(clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::HASKEYICON) == true) {
                ChangedUser->ui32BoolBits |= User::BIT_OPERATOR;
                clsUsers::mPtr->Add2OpList(ChangedUser);
                clsGlobalDataQueue::mPtr->OpListStore(ChangedUser->sNick);
            } else {
                ChangedUser->ui32BoolBits &= ~User::BIT_OPERATOR;
                clsUsers::mPtr->DelFromOpList(ChangedUser->sNick);
            }
        }

        if(bAllowedOpChat != clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::ALLOWEDOPCHAT)) {
            if(clsProfileManager::mPtr->IsAllowed(ChangedUser, clsProfileManager::ALLOWEDOPCHAT) == true) {
                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true &&
                    (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                    if(((ChangedUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                        ChangedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_HELLO],
                        clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_HELLO]);
                    }

                    ChangedUser->SendCharDelayed(clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO], clsSettingManager::mPtr->ui16PreTextsLens[clsSettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                    ChangedUser->SendFormat("clsRegManager::ChangeReg1", true, "$OpList %s$$|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                }
            } else {
                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                    ChangedUser->SendFormat("clsRegManager::ChangeReg2", true, "$Quit %s|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                }
            }
        }
    }

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->RegChanged(pReg);
    }
#endif
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void clsRegManager::Delete(RegUser * pReg, const bool &bFromGui/* = false*/) {
#else
void clsRegManager::Delete(RegUser * pReg, const bool &/*bFromGui = false*/) {
#endif
	if(clsServerManager::bServerRunning == true) {
        User * pRemovedUser = clsHashManager::mPtr->FindUser(pReg->sNick, strlen(pReg->sNick));

        if(pRemovedUser != NULL) {
            pRemovedUser->i32Profile = -1;
            if(((pRemovedUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                clsUsers::mPtr->DelFromOpList(pRemovedUser->sNick);
                pRemovedUser->ui32BoolBits &= ~User::BIT_OPERATOR;

                if(clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true && (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false || clsSettingManager::mPtr->bBotsSameNick == false)) {
                    pRemovedUser->SendFormat("clsRegManager::Delete", true, "$Quit %s|", clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]);
                }
            }
        }
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && clsRegisteredUsersDialog::mPtr != NULL) {
        clsRegisteredUsersDialog::mPtr->RemoveReg(pReg);
    }
#endif

	Rem(pReg);

#ifdef _BUILD_GUI
    if(clsRegisteredUserDialog::mPtr != NULL) {
        clsRegisteredUserDialog::mPtr->RegDeleted(pReg);
    }
#endif

    delete pReg;

    Save(true);
}
//---------------------------------------------------------------------------

void clsRegManager::Rem(RegUser * Reg) {
	RemFromTable(Reg);
    
    RegUser *prev, *next;
    prev = Reg->pPrev; next = Reg->pNext;

    if(prev == NULL) {
        if(next == NULL) {
            pRegListS = NULL;
            pRegListE = NULL;
        } else {
            next->pPrev = NULL;
            pRegListS = next;
        }
    } else if(next == NULL) {
        prev->pNext = NULL;
        pRegListE = prev;
    } else {
        prev->pNext = next;
        next->pPrev = prev;
    }
}
//---------------------------------------------------------------------------

void clsRegManager::RemFromTable(RegUser * Reg) {
    if(Reg->pHashTablePrev == NULL) {
        uint16_t ui16dx = 0;
        memcpy(&ui16dx, &Reg->ui32Hash, sizeof(uint16_t));

        if(Reg->pHashTableNext == NULL) {
            pTable[ui16dx] = NULL;
        } else {
            Reg->pHashTableNext->pHashTablePrev = NULL;
			pTable[ui16dx] = Reg->pHashTableNext;
        }
    } else if(Reg->pHashTableNext == NULL) {
        Reg->pHashTablePrev->pHashTableNext = NULL;
    } else {
        Reg->pHashTablePrev->pHashTableNext = Reg->pHashTableNext;
        Reg->pHashTableNext->pHashTablePrev = Reg->pHashTablePrev;
    }

	Reg->pHashTablePrev = NULL;
    Reg->pHashTableNext = NULL;
}
//---------------------------------------------------------------------------

RegUser* clsRegManager::Find(char * sNick, const size_t &szNickLen) {
    uint32_t ui32Hash = HashNick(sNick, szNickLen);

    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

    RegUser * cur = NULL,
        * next = pTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashTableNext;

		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* clsRegManager::Find(User * u) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &u->ui32NickHash, sizeof(uint16_t));

	RegUser * cur = NULL,
        * next = pTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashTableNext;

		if(cur->ui32Hash == u->ui32NickHash && strcasecmp(cur->sNick, u->sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RegUser* clsRegManager::Find(uint32_t ui32Hash, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

	RegUser * cur = NULL,
        * next = pTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->pHashTableNext;

		if(cur->ui32Hash == ui32Hash && strcasecmp(cur->sNick, sNick) == 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

void clsRegManager::Load(void) {
#ifdef _WIN32
    if(FileExist((clsServerManager::sPath + "\\cfg\\RegisteredUsers.pxb").c_str()) == false) {
#else
    if(FileExist((clsServerManager::sPath + "/cfg/RegisteredUsers.pxb").c_str()) == false) {
#endif
        LoadXML();
        return;
    }

    uint16_t iProfilesCount = (uint16_t)(clsProfileManager::mPtr->ui16ProfileCount-1);

    PXBReader pxbRegs;

    // Open regs file
#ifdef _WIN32
    if(pxbRegs.OpenFileRead((clsServerManager::sPath + "\\cfg\\RegisteredUsers.pxb").c_str(), 4) == false) {
#else
    if(pxbRegs.OpenFileRead((clsServerManager::sPath + "/cfg/RegisteredUsers.pxb").c_str(), 4) == false) {
#endif
        return;
    }

    // Read file header
    uint16_t ui16Identificators[4] = { *((uint16_t *)"FI"), *((uint16_t *)"FV"), *((uint16_t *)"  "), *((uint16_t *)"  ") };

    if(pxbRegs.ReadNextItem(ui16Identificators, 2) == false) {
        return;
    }

    // Check header if we have correct file
    if(pxbRegs.ui16ItemLengths[0] != szPtokaXRegiteredUsersLen || strncmp((char *)pxbRegs.pItemDatas[0], sPtokaXRegiteredUsers, szPtokaXRegiteredUsersLen) != 0) {
        return;
    }

    {
        uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbRegs.pItemDatas[1])));

        if(ui32FileVersion < 1) {
            return;
        }
    }

    // Read regs =)
    ui16Identificators[0] = *((uint16_t *)"NI");
    ui16Identificators[1] = *((uint16_t *)"PS");
    ui16Identificators[2] = *((uint16_t *)"PR");
    ui16Identificators[3] = *((uint16_t *)"PA");

    uint16_t iProfile = UINT16_MAX;
    RegUser * pNewUser = NULL;
    uint8_t ui8Hash[64];
    size_t szPassLen = 0;

    bool bSuccess = pxbRegs.ReadNextItem(ui16Identificators, 3, 1);

    while(bSuccess == true) {
		if(pxbRegs.ui16ItemLengths[0] < 65 && pxbRegs.ui16ItemLengths[1] < 65 && pxbRegs.ui16ItemLengths[2] == 2) {
            iProfile = (uint16_t)ntohs(*((uint16_t *)(pxbRegs.pItemDatas[2])));

            if(iProfile > iProfilesCount) {
                iProfile = iProfilesCount;
            }

            pNewUser = NULL;

            if(pxbRegs.ui16ItemLengths[3] != 0) {
                if(clsSettingManager::mPtr->bBools[SETBOOL_HASH_PASSWORDS] == true) {
                    szPassLen = (size_t)pxbRegs.ui16ItemLengths[3];

                    if(HashPassword((char *)pxbRegs.pItemDatas[3], szPassLen, ui8Hash) == false) {
                        pNewUser = RegUser::CreateReg((char *)pxbRegs.pItemDatas[0], pxbRegs.ui16ItemLengths[0], (char *)pxbRegs.pItemDatas[3], pxbRegs.ui16ItemLengths[3], NULL, iProfile);
                    } else {
                        pNewUser = RegUser::CreateReg((char *)pxbRegs.pItemDatas[0], pxbRegs.ui16ItemLengths[0], NULL, 0, ui8Hash, iProfile);
                    }
                } else {
                    pNewUser = RegUser::CreateReg((char *)pxbRegs.pItemDatas[0], pxbRegs.ui16ItemLengths[0], (char *)pxbRegs.pItemDatas[3], pxbRegs.ui16ItemLengths[3], NULL, iProfile);
                }
            } else if(pxbRegs.ui16ItemLengths[1] == 64) {
#ifdef _WITHOUT_SKEIN
				AppendDebugLog("%s - [ERR] Hashed password found in RegisteredUsers, but PtokaX is compiled without hashing support!\n");

                exit(EXIT_FAILURE);
#endif
                pNewUser = RegUser::CreateReg((char *)pxbRegs.pItemDatas[0], pxbRegs.ui16ItemLengths[0], NULL, 0, (uint8_t *)pxbRegs.pItemDatas[1], iProfile);
            }

            if(pNewUser == NULL) {
				AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in clsRegManager::Load\n");

                exit(EXIT_FAILURE);
            }

			Add(pNewUser);
		}

        bSuccess = pxbRegs.ReadNextItem(ui16Identificators, 3, 1);
    }
}
//---------------------------------------------------------------------------

void clsRegManager::LoadXML() {
    uint16_t iProfilesCount = (uint16_t)(clsProfileManager::mPtr->ui16ProfileCount-1);

#ifdef _WIN32
    TiXmlDocument doc((clsServerManager::sPath+"\\cfg\\RegisteredUsers.xml").c_str());
#else
	TiXmlDocument doc((clsServerManager::sPath+"/cfg/RegisteredUsers.xml").c_str());
#endif

    if(doc.LoadFile() == false) {
        if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "Error loading file RegisteredUsers.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			CheckSprintf(imsgLen, clsServerManager::szGlobalBufferSize, "clsRegManager::LoadXML");
#ifdef _BUILD_GUI
			::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(clsServerManager::pGlobalBuffer);
#endif
            exit(EXIT_FAILURE);
        }
    } else {
        TiXmlHandle cfg(&doc);
        TiXmlNode *registeredusers = cfg.FirstChild("RegisteredUsers").Node();
        if(registeredusers != NULL) {
            bool bIsBuggy = false;
            TiXmlNode *child = NULL;

            while((child = registeredusers->IterateChildren(child)) != NULL) {
				TiXmlNode *registereduser = child->FirstChild("Nick");

				if(registereduser == NULL || (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *nick = (char *)registereduser->Value();
                
				if(strlen(nick) > 64 || (registereduser = child->FirstChild("Password")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

                char *pass = (char *)registereduser->Value();
                
				if(strlen(pass) > 64 || (registereduser = child->FirstChild("Profile")) == NULL ||
                    (registereduser = registereduser->FirstChild()) == NULL) {
					continue;
				}

				uint16_t iProfile = (uint16_t)atoi(registereduser->Value());

				if(iProfile > iProfilesCount) {
                    int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s %s %s! %s %s.", clsLanguageManager::mPtr->sTexts[LAN_USER], nick, clsLanguageManager::mPtr->sTexts[LAN_HAVE_NOT_EXIST_PROFILE],
                        clsLanguageManager::mPtr->sTexts[LAN_CHANGED_PROFILE_TO], clsProfileManager::mPtr->ppProfilesTable[iProfilesCount]->sName);
					CheckSprintf(imsgLen, clsServerManager::szGlobalBufferSize, "clsRegManager::Load");

#ifdef _BUILD_GUI
					::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
#else
					AppendLog(clsServerManager::pGlobalBuffer);
#endif

                    iProfile = iProfilesCount;
                    bIsBuggy = true;
                }

                if(Find((char*)nick, strlen(nick)) == NULL) {
                    RegUser * pNewUser = RegUser::CreateReg(nick, strlen(nick), pass, strlen(pass), NULL, iProfile);
                    if(pNewUser == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in clsRegManager::LoadXML\n");

                    	exit(EXIT_FAILURE);
                    }
					Add(pNewUser);
                } else {
                    int imsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s %s %s! %s.", clsLanguageManager::mPtr->sTexts[LAN_USER], nick, clsLanguageManager::mPtr->sTexts[LAN_IS_ALREADY_IN_REGS], 
                        clsLanguageManager::mPtr->sTexts[LAN_USER_DELETED]);
					CheckSprintf(imsgLen, clsServerManager::szGlobalBufferSize, "clsRegManager::Load1");

#ifdef _BUILD_GUI
					::MessageBox(NULL, clsServerManager::pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
#else
					AppendLog(clsServerManager::pGlobalBuffer);
#endif

                    bIsBuggy = true;
                }
            }
			if(bIsBuggy == true)
				Save();
        }
    }
}
//---------------------------------------------------------------------------

void clsRegManager::Save(const bool &bSaveOnChange/* = false*/, const bool &bSaveOnTime/* = false*/) {
    if(bSaveOnTime == true && ui8SaveCalls == 0) {
        return;
    }

    ui8SaveCalls++;

    if(bSaveOnChange == true && ui8SaveCalls < 100) {
        return;
    }

    ui8SaveCalls = 0;

    PXBReader pxbRegs;

    // Open regs file
#ifdef _WIN32
    if(pxbRegs.OpenFileSave((clsServerManager::sPath + "\\cfg\\RegisteredUsers.pxb").c_str(), 3) == false) {
#else
    if(pxbRegs.OpenFileSave((clsServerManager::sPath + "/cfg/RegisteredUsers.pxb").c_str(), 3) == false) {
#endif
        return;
    }

    // Write file header
    pxbRegs.sItemIdentifiers[0] = 'F';
    pxbRegs.sItemIdentifiers[1] = 'I';
    pxbRegs.ui16ItemLengths[0] = (uint16_t)szPtokaXRegiteredUsersLen;
    pxbRegs.pItemDatas[0] = (void *)sPtokaXRegiteredUsers;
    pxbRegs.ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbRegs.sItemIdentifiers[2] = 'F';
    pxbRegs.sItemIdentifiers[3] = 'V';
    pxbRegs.ui16ItemLengths[1] = 4;
    uint32_t ui32Version = 1;
    pxbRegs.pItemDatas[1] = (void *)&ui32Version;
    pxbRegs.ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if(pxbRegs.WriteNextItem(szPtokaXRegiteredUsersLen+4, 2) == false) {
        return;
    }

    pxbRegs.sItemIdentifiers[0] = 'N';
    pxbRegs.sItemIdentifiers[1] = 'I';
    pxbRegs.sItemIdentifiers[2] = 'P';
    pxbRegs.sItemIdentifiers[3] = 'A';
    pxbRegs.sItemIdentifiers[4] = 'P';
    pxbRegs.sItemIdentifiers[5] = 'R';

    pxbRegs.ui8ItemValues[0] = PXBReader::PXB_STRING;
    pxbRegs.ui8ItemValues[1] = PXBReader::PXB_STRING;
    pxbRegs.ui8ItemValues[2] = PXBReader::PXB_TWO_BYTES;

    RegUser * curReg = NULL,
        * next = pRegListS;

    while(next != NULL) {
        curReg = next;
		next = curReg->pNext;

        pxbRegs.ui16ItemLengths[0] = (uint16_t)strlen(curReg->sNick);
        pxbRegs.pItemDatas[0] = (void *)curReg->sNick;
        pxbRegs.ui8ItemValues[0] = PXBReader::PXB_STRING;

        if(curReg->bPassHash == true) {
            pxbRegs.sItemIdentifiers[3] = 'S';

            pxbRegs.ui16ItemLengths[1] = 64;
            pxbRegs.pItemDatas[1] = (void *)curReg->ui8PassHash;
        } else {
            pxbRegs.sItemIdentifiers[3] = 'A';

            pxbRegs.ui16ItemLengths[1] = (uint16_t)strlen(curReg->sPass);
            pxbRegs.pItemDatas[1] = (void *)curReg->sPass;
        }

        pxbRegs.ui16ItemLengths[2] = 2;
        pxbRegs.pItemDatas[2] = (void *)&curReg->ui16Profile;

        if(pxbRegs.WriteNextItem(pxbRegs.ui16ItemLengths[0] + pxbRegs.ui16ItemLengths[1] + pxbRegs.ui16ItemLengths[2], 3) == false) {
            break;
        }
    }

    pxbRegs.WriteRemaining();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsRegManager::HashPasswords() {
    size_t szPassLen = 0;
    char * sOldPass = NULL;

    RegUser * pCurReg = NULL,
        * pNextReg = pRegListS;

    while(pNextReg != NULL) {
        pCurReg = pNextReg;
		pNextReg = pCurReg->pNext;

        if(pCurReg->bPassHash == false) {
            sOldPass = pCurReg->sPass;
#ifdef _WIN32
            pCurReg->ui8PassHash = (uint8_t *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, 64);
#else
            pCurReg->ui8PassHash = (uint8_t *)malloc(64);
#endif
            if(pCurReg->ui8PassHash == NULL) {
                pCurReg->sPass = sOldPass;

                AppendDebugLog("%s - [MEM] Cannot reallocate 64 bytes for sPass->ui8PassHash in clsRegManager::HashPasswords\n");

                continue;
            }

            szPassLen = strlen(sOldPass);

            if(HashPassword(sOldPass, szPassLen, pCurReg->ui8PassHash) == true) {
                pCurReg->bPassHash = true;

#ifdef _WIN32
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPass) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate sOldPass in clsRegManager::HashPasswords\n");
                }
#else
                free(sOldPass);
#endif
            } else {
#ifdef _WIN32
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCurReg->ui8PassHash) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate pCurReg->ui8PassHash in clsRegManager::HashPasswords\n");
                }
#else
                free(pCurReg->ui8PassHash);
#endif

                pCurReg->sPass = sOldPass;
            }
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsRegManager::AddRegCmdLine() {
	char sNick[66];

nick:
	printf("Please enter Nick for new Registered User (Maximal length 64 characters. Characters |, $ and space are not allowed): ");
	if(fgets(sNick, 66, stdin) != NULL) {
		char * sMatch = strchr(sNick, '\n');
		if(sMatch != NULL) {
			sMatch[0] = '\0';
		}

		sMatch = strpbrk(sNick, " $|");
		if(sMatch != NULL) {
			printf("Character '%c' is not allowed in Nick!\n", sMatch[0]);

			if(WantAgain() == false) {
				return;
			}

			goto nick;
		}

		size_t szLen = strlen(sNick);

		if(szLen == 0) {
			printf("No Nick specified!\n");

			if(WantAgain() == false) {
				return;
			}

			goto nick;
		}

		RegUser * pReg = Find(sNick, strlen(sNick));
		if(pReg != NULL) {
			printf("Registered user with nick '%s' already exist!\n", sNick);

			if(WantAgain() == false) {
				return;
			}

			goto nick;
		}
	} else {
		printf("Error reading Nick... ending.\n");
		exit(EXIT_FAILURE);
	}

	char sPassword[66];

password:
	printf("Please enter Password for new Registered User (Maximal length 64 characters. Character | is not allowed): ");
	if(fgets(sPassword, 66, stdin) != NULL) {
		char * sMatch = strchr(sPassword, '\n');
		if(sMatch != NULL) {
			sMatch[0] = '\0';
		}

		sMatch = strchr(sPassword, '|');
		if(sMatch != NULL) {
			printf("Character | is not allowed in Password!\n");

			if(WantAgain() == false) {
				return;
			}

			goto password;
		}

		size_t szLen = strlen(sPassword);

		if(szLen == 0) {
			printf("No Password specified!\n");

			if(WantAgain() == false) {
				return;
			}

			goto password;
		}
	} else {
		printf("Error reading Password... ending.\n");
		exit(EXIT_FAILURE);
	}

	printf("\nAvailable profiles: \n");
    for(uint16_t ui16i = 0; ui16i < clsProfileManager::mPtr->ui16ProfileCount; ui16i++) {
    	printf("%hu - %s\n", ui16i, clsProfileManager::mPtr->ppProfilesTable[ui16i]->sName);
    }

	uint16_t ui16Profile = 0;
	char sProfile[7];

profile:

	printf("Please enter Profile number for new Registered User: ");
	if(fgets(sProfile, 7, stdin) != NULL) {
		char * sMatch = strchr(sProfile, '\n');
		if(sMatch != NULL) {
			sMatch[0] = '\0';
		}

		uint8_t ui8Len = (uint8_t)strlen(sProfile);

		if(ui8Len == 0) {
			printf("No Profile specified!\n");

			if(WantAgain() == false) {
				return;
			}

			goto profile;
		}

		for(uint8_t ui8i = 0; ui8i < ui8Len; ui8i++) {
			if(isdigit(sProfile[ui8i]) == 0) {
				printf("Character '%c' is not valid number!\n", sProfile[ui8i]);

				if(WantAgain() == false) {
					return;
				}

				goto profile;
			}
		}

		ui16Profile = (uint16_t)atoi(sProfile);
		if(ui16Profile >= clsProfileManager::mPtr->ui16ProfileCount) {
			printf("Profile number %hu not exist!\n", ui16Profile);

			if(WantAgain() == false) {
				return;
			}

			goto profile;
		}
	} else {
		printf("Error reading Profile... ending.\n");
		exit(EXIT_FAILURE);
	}

	if(AddNew(sNick, sPassword, ui16Profile) == false) {
		printf("Error adding new Registered User... ending.\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Registered User with Nick '%s' Password '%s' and Profile '%hu' was added.", sNick, sPassword, ui16Profile);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
