/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2008  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------
#include "LuaInc.h"
//------------------------------------------------------------------------------
#include "LuaScriptManager.h"
//------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------
#include "LuaScript.h"
//------------------------------------------------------------------------------
ScriptMan *ScriptManager = NULL;
//------------------------------------------------------------------------------

ScriptMan::ScriptMan() {
    RunningScriptS = NULL;
    RunningScriptE = NULL;

    ScriptTable = NULL;
    
    ActualUser = NULL;

    ui8ScriptCount = 0;
    ui8BotsCount = 0;

    bMoved = false;

	// PPK ... first start all script in order from xml file
	TiXmlDocument doc((PATH+"/cfg/Scripts.xml").c_str());
	if(doc.LoadFile()) {
		TiXmlHandle cfg(&doc);
		TiXmlNode *scripts = cfg.FirstChild("Scripts").Node();
		if(scripts != NULL) {
			TiXmlNode *child = NULL;
			while((child = scripts->IterateChildren(child)) != NULL) {
				TiXmlNode *script = child->FirstChild("Name");
    
				if(script == NULL || (script = script->FirstChild()) == NULL) {
					continue;
				}
    
				char *name = (char *)script->Value();

				if(FileExist((SCRIPT_PATH+string(name)).c_str()) == false) {
					continue;
				}

				if((script = child->FirstChild("Enabled")) == NULL ||
					(script = script->FirstChild()) == NULL) {
					continue;
				}
    
				bool enabled = atoi(script->Value()) == 0 ? false : true;

				if(FindScript(name) != NULL) {
					continue;
				}

				AddScript(name, enabled);
            }
        }
    }
}
//------------------------------------------------------------------------------

ScriptMan::~ScriptMan() {
    RunningScriptS = NULL;
    RunningScriptE = NULL;

    for(uint8_t i = 0; i < ui8ScriptCount; i++) {
		delete ScriptTable[i];
    }

	free(ScriptTable);
	ScriptTable = NULL;

	ui8ScriptCount = 0;

    ActualUser = NULL;

    ui8BotsCount = 0;
}
//------------------------------------------------------------------------------

void ScriptMan::Start() {
    ui8BotsCount = 0;
    ActualUser = NULL;

    
    // PPK ... first look for deleted and new scripts
    CheckForDeletedScripts();
    CheckForNewScripts();

    // PPK ... second start all enabled scripts
    for(uint8_t i = 0; i < ui8ScriptCount; i++) {
		if(ScriptTable[i]->bEnabled == true) {
        	if(ScriptStart(ScriptTable[i]) == true) {
				AddRunningScript(ScriptTable[i]);
			}
		}
	}
}
//------------------------------------------------------------------------------

bool ScriptMan::AddScript(char * sName, const bool &bEnabled/* = false*/) {
    if(ui8ScriptCount == 254) {
        return false;
    }

    ui8ScriptCount++;

    if(ScriptTable == NULL) {
        ScriptTable = (Script **) calloc(ui8ScriptCount, sizeof(Script *));
        if(ScriptTable == NULL) {
			string sDbgstr = "[BUF] Cannot allocate ScriptTable in ScriptMan::AddScript!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
    } else {
		ScriptTable = (Script **) realloc(ScriptTable, ui8ScriptCount*sizeof(Script *));
		if(ScriptTable == NULL) {
            string sDbgstr = "[BUF] Cannot reallocate ScriptTable in ScriptMan::AddScript!";
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
    }

    ScriptTable[ui8ScriptCount-1] = new Script(sName, bEnabled);

    if(ScriptTable[ui8ScriptCount-1] == NULL) {
		string sDbgstr = "[BUF] Cannot allocate new Script!";
		AppendSpecialLog(sDbgstr);
        return false;
    }

    return true;
}
//------------------------------------------------------------------------------

void ScriptMan::Stop() {
	Script *next = RunningScriptS;

    RunningScriptS = NULL;
	RunningScriptE = NULL;

    while(next != NULL) {
		Script *S = next;
		next = S->next;

		ScriptStop(S);
	}

	ActualUser = NULL;
}
//------------------------------------------------------------------------------

void ScriptMan::AddRunningScript(Script * curScript) {
	if(RunningScriptS == NULL) {
		RunningScriptS = curScript;
		RunningScriptE = curScript;
		return;
	}

   	curScript->prev = RunningScriptE;
    RunningScriptE->next = curScript;
    RunningScriptE = curScript;
}
//------------------------------------------------------------------------------

void ScriptMan::RemoveRunningScript(Script * curScript) {
	// single entry
	if(curScript->prev == NULL && curScript->next == NULL) {
    	RunningScriptS = NULL;
        RunningScriptE = NULL;
        return;
    }

    // first in list
	if(curScript->prev == NULL) {
        RunningScriptS = curScript->next;
        RunningScriptS->prev = NULL;
        return;
    }

    // last in list
    if(curScript->next == NULL) {
        RunningScriptE = curScript->prev;
    	RunningScriptE->next = NULL;
        return;
    }

    // in the middle
    curScript->prev->next = curScript->next;
    curScript->next->prev = curScript->prev;
}
//------------------------------------------------------------------------------

void ScriptMan::SaveScripts() {
    TiXmlDocument doc((PATH+"/cfg/Scripts.xml").c_str());
    doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement scripts("Scripts");

	for(uint8_t i = 0; i < ui8ScriptCount; i++) {
		if(FileExist((SCRIPT_PATH+string(ScriptTable[i]->sName)).c_str()) == false) {
			continue;
        }

        TiXmlElement name("Name");
        name.InsertEndChild(TiXmlText(ScriptTable[i]->sName));
        
        TiXmlElement enabled("Enabled");
        if(ScriptTable[i]->bEnabled == true) {
            enabled.InsertEndChild(TiXmlText("1"));
        } else {
            enabled.InsertEndChild(TiXmlText("0"));
        }
        
        TiXmlElement script("Script");
        script.InsertEndChild(name);
        script.InsertEndChild(enabled);
        
        scripts.InsertEndChild(script);
    }
    doc.InsertEndChild(scripts);
    doc.SaveFile();
}
//------------------------------------------------------------------------------

void ScriptMan::CheckForDeletedScripts() {
    uint8_t i = 0;

	while(i < ui8ScriptCount) {
		if(FileExist((SCRIPT_PATH+string(ScriptTable[i]->sName)).c_str()) == true || ScriptTable[i]->LUA != NULL) {
			i++;
			continue;
        }

		delete ScriptTable[i];

		for(uint8_t j = i; j+1 < ui8ScriptCount; j++) {
            ScriptTable[j] = ScriptTable[j+1];
        }

        ScriptTable[ui8ScriptCount-1] = NULL;
        ui8ScriptCount--;
    }
}
//------------------------------------------------------------------------------

void ScriptMan::CheckForNewScripts() {
    DIR * p_scriptdir = opendir(SCRIPT_PATH.c_str());

    if(p_scriptdir == NULL) {
        return;
    }

    struct dirent * p_dirent;
    struct stat s_buf;

    while((p_dirent = readdir(p_scriptdir)) != NULL) {
        if(stat((SCRIPT_PATH + p_dirent->d_name).c_str(), &s_buf) != 0 || 
            (s_buf.st_mode & S_IFDIR) != 0 || 
            strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name)-4), ".lua") != 0) {
            continue;
        }

		if(FindScript(p_dirent->d_name) != NULL) {
            continue;
        }

		AddScript(p_dirent->d_name);
    }

    closedir(p_scriptdir);
}
//------------------------------------------------------------------------------

void ScriptMan::Restart() {
	OnExit();
	Stop();

    CheckForDeletedScripts();

	Start();
	OnStartup();
}
//------------------------------------------------------------------------------

Script * ScriptMan::FindScript(char * sName) {
    for(uint8_t i = 0; i < ui8ScriptCount; i++) {
        if(strcasecmp(ScriptTable[i]->sName, sName) == 0) {
            return ScriptTable[i];
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------

Script * ScriptMan::FindScript(lua_State * L) {
    Script *next = RunningScriptS;

    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

        if(cur->LUA == L) {
            return cur;
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------

uint8_t ScriptMan::FindScriptIdx(char * sName) {
    for(uint8_t i = 0; i < ui8ScriptCount; i++) {
        if(strcasecmp(ScriptTable[i]->sName, sName) == 0) {
            return i;
        }
    }

    return ui8ScriptCount;
}
//------------------------------------------------------------------------------

bool ScriptMan::StartScript(Script * curScript) {
    uint8_t ui8dx = 255; 
    for(uint8_t i = 0; i < ui8ScriptCount; i++) {
        if(curScript == ScriptTable[i]) {
            ui8dx = i;
            break;
        }
    }

    if(ui8dx == 255) {
        return false;
	}

    if(ScriptStart(curScript) == false) {
        return false;
    }

	if(RunningScriptS == NULL) {
		RunningScriptS = curScript;
		RunningScriptE = curScript;
	} else {
		// previous script
		if(ui8dx != 0) {
			for(int16_t i = (int16_t)(ui8dx-1); i > -1; i--) {
				if(ScriptTable[i]->bEnabled == true) {
					ScriptTable[i]->next = curScript;
					curScript->prev = ScriptTable[i];
					break;
				}
			}

			if(curScript->prev == NULL) {
				RunningScriptS = curScript;
			}
		} else {
			curScript->next = RunningScriptS;
			RunningScriptS->prev = curScript;
			RunningScriptS = curScript;
        }

		// next script
		if(ui8dx != ui8ScriptCount-1) {
			for(int16_t i = (int16_t)(ui8dx+1); i < (int16_t)ui8ScriptCount; i++) {
				if(ScriptTable[i]->bEnabled == true) {
					ScriptTable[i]->prev = curScript;
					curScript->next = ScriptTable[i];
					break;
				}
			}

			if(curScript->next == NULL) {
				RunningScriptE = curScript;
			}
		} else {
			curScript->prev = RunningScriptE;
			RunningScriptE->next = curScript;
			RunningScriptE = curScript;
        }
	}


	if(bServerRunning == true) {
        ScriptOnStartup(curScript);
    }

    return true;
}
//------------------------------------------------------------------------------

void ScriptMan::StopScript(Script * curScript) {
	RemoveRunningScript(curScript);

    ScriptTimer * next = curScript->TimerList;

    while(next != NULL) {
        ScriptTimer * tmr = next;
        next = tmr->next;

        timer_delete(tmr->TimerId);
        tmr->TimerId = 0;
    }

    if(bServerRunning == true) {
        ScriptOnExit(curScript);
    }

	ScriptStop(curScript);
}
//------------------------------------------------------------------------------

void ScriptMan::MoveScript(const uint8_t &ui8ScriptPosInTbl, const bool &bUp) {
    if(bUp == true) {
		if(ui8ScriptPosInTbl == 0) {
            return;
        }

        Script * cur = ScriptTable[ui8ScriptPosInTbl];
		ScriptTable[ui8ScriptPosInTbl] = ScriptTable[ui8ScriptPosInTbl-1];
		ScriptTable[ui8ScriptPosInTbl-1] = cur;

		// if one of moved scripts not running then return
		if(cur->LUA == NULL || ScriptTable[ui8ScriptPosInTbl]->LUA == NULL) {
			return;
		}

		if(cur->prev == NULL) { // first running script, nothing to move
			return;
		} else if(cur->next == NULL) { // last running script
			// set prev script as last
			RunningScriptE = cur->prev;

			// change prev prev script next
			if(RunningScriptE->prev != NULL) {
				RunningScriptE->prev->next = cur;
			} else {
				RunningScriptS = cur;
			}

			// change cur script prev and next
			cur->prev = RunningScriptE->prev;
			cur->next = RunningScriptE;

			// change prev script prev to cur and his next to NULL
			RunningScriptE->prev = cur;
			RunningScriptE->next = NULL;
		} else { // not first, not last ...
			// remember original prev and next
			Script * prev = cur->prev;
			Script * next = cur->next;

			// change cur script prev
			cur->prev = prev->prev;

			// change prev script next
			prev->next = next;

			// change cur script next
			cur->next = prev;

			// change next script prev
			next->prev = prev;

			// change prev prev script next
			if(prev->prev != NULL) {
				prev->prev->next = cur;
			} else {
				RunningScriptS = cur;
			}

			// change prev script prev
			prev->prev = cur;
		}
    } else {
		if(ui8ScriptPosInTbl == ui8ScriptCount-1) {
            return;
		}

        Script * cur = ScriptTable[ui8ScriptPosInTbl];
		ScriptTable[ui8ScriptPosInTbl] = ScriptTable[ui8ScriptPosInTbl+1];
        ScriptTable[ui8ScriptPosInTbl+1] = cur;

		// if one of moved scripts not running then return
		if(cur->LUA == NULL || ScriptTable[ui8ScriptPosInTbl]->LUA == NULL) {
			return;
		}

        if(cur->next == NULL) { // last running script, nothing to move
            return;
        } else if(cur->prev == NULL) { // first running script
            //set next running script as first
            RunningScriptS = cur->next;

            // change next next script prev
			if(RunningScriptS->next != NULL) {
				RunningScriptS->next->prev = cur;
			} else {
				RunningScriptE = cur;
			}

			// change cur script prev and next
            cur->prev = RunningScriptS;
			cur->next = RunningScriptS->next;

            // change next script next to cur and his prev to NULL
			RunningScriptS->prev = NULL;
			RunningScriptS->next = cur;
		} else { // not first, not last ...
            // remember original prev and next
            Script * prev = cur->prev;
            Script * next = cur->next;

			// change cur script next
			cur->next = next->next;

			// change next script prev
			next->prev = prev;

			// change cur script prev
			cur->prev = next;

			// change prev script next
			prev->next = next;

			// change next next script prev
            if(next->next != NULL) {
                next->next->prev = cur;
			} else {
				RunningScriptE = cur;
			}

			// change next script next
			next->next = cur;
		}
	}
}
//------------------------------------------------------------------------------

void ScriptMan::DeleteScript(const uint8_t &ui8ScriptPosInTbl) {
    Script * cur = ScriptTable[ui8ScriptPosInTbl];

	if(cur->LUA != NULL) {
		ScriptManager->StopScript(cur);
	}

	if(FileExist((SCRIPT_PATH+string(cur->sName)).c_str()) == true) {
        unlink((SCRIPT_PATH+string(cur->sName)).c_str());
    }

    delete cur;

	for(uint8_t i = ui8ScriptPosInTbl; i+1 < ui8ScriptCount; i++) {
        ScriptTable[i] = ScriptTable[i+1];
    }

    ScriptTable[ui8ScriptCount-1] = NULL;
    ui8ScriptCount--;
}
//------------------------------------------------------------------------------

void ScriptMan::OnStartup() {
    if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    ActualUser = NULL;
    bMoved = false;

    Script *next = RunningScriptS;
        
    while(next != NULL) {
    	Script * cur = next;
        next = cur->next;

		if(((cur->ui16Functions & Script::ONSTARTUP) == Script::ONSTARTUP) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;
            ScriptOnStartup(cur);
        }
	}
}
//------------------------------------------------------------------------------

void ScriptMan::OnExit(bool bForce/* = false*/) {
    if(bForce == false && SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    ActualUser = NULL;
    bMoved = false;

    Script *next = RunningScriptS;
        
    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

		if(((cur->ui16Functions & Script::ONEXIT) == Script::ONEXIT) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;
            ScriptOnExit(cur);
        }
    }
}
//------------------------------------------------------------------------------

bool ScriptMan::Arrival(User * u, char * sData, const size_t &iLen, const unsigned char &iType) {
	if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
		return false;
	}

	static const uint32_t iLuaArrivalBits[] = {
        0x1, 
        0x2, 
        0x4, 
        0x8, 
        0x10, 
        0x20, 
        0x40, 
        0x80, 
        0x100, 
        0x200, 
        0x400, 
        0x800, 
        0x1000, 
        0x2000, 
        0x4000, 
        0x8000, 
        0x10000, 
        0x20000, 
        0x40000, 
        0x80000, 
        0x100000
	};

    bMoved = false;

    Script *next = RunningScriptS;
        
    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

        // if any of the scripts returns a nonzero value,
		// then stop for all other scripts
        if(((cur->ui32DataArrivals & iLuaArrivalBits[iType]) == iLuaArrivalBits[iType]) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;

            // PPK ... table of arrivals
            static const char* arrival[] = { "ChatArrival", "KeyArrival", "ValidateNickArrival", "PasswordArrival",
            "VersionArrival", "GetNickListArrival", "MyINFOArrival", "GetINFOArrival", "SearchArrival",
            "ToArrival", "ConnectToMeArrival", "MultiConnectToMeArrival", "RevConnectToMeArrival", "SRArrival",
            "UDPSRArrival", "KickArrival", "OpForceMoveArrival", "SupportsArrival", "BotINFOArrival", 
            "CloseArrival", "UnknownArrival" };

            lua_getglobal(cur->LUA, arrival[iType]);
            int i = lua_gettop(cur->LUA);
            if(lua_isnil(cur->LUA, i)) {
                cur->ui32DataArrivals &= ~iLuaArrivalBits[iType];

                lua_settop(cur->LUA, 0);
                continue;
            }

            ActualUser = u;

            lua_checkstack(cur->LUA, 2); // we need 2 empty slots in stack, check it to be sure

			ScriptPushUser(cur->LUA, u); // usertable
            lua_pushlstring(cur->LUA, sData, iLen); // sData

            // two passed parameters, zero returned
            if(lua_pcall(cur->LUA, 2, LUA_MULTRET, 0) != 0) {
                ScriptError(cur);

                lua_settop(cur->LUA, 0);
                continue;
            }

            ActualUser = NULL;
        
            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return it

            int top = lua_gettop(cur->LUA);
        
            // no return value
            if(top == 0) {
                continue;
            }

			if(lua_type(cur->LUA, top) != LUA_TBOOLEAN || lua_toboolean(cur->LUA, top) == 0) {
                lua_settop(cur->LUA, 0);
                continue;
            }

            // clear the stack for sure
            lua_settop(cur->LUA, 0);

			return true; // true means DO NOT process data by the hub's core
        }
    }

    return false;
}
//------------------------------------------------------------------------------

void ScriptMan::UserConnected(User * u) {
	if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    uint8_t ui8Type = 0; // User
    if(u->iProfile != -1) {
        if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            ui8Type = 1; // Reg
		} else {
			ui8Type = 2; // OP
		}
    }

    bMoved = false;

    Script *next = RunningScriptS;
        
    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

		static const uint32_t iConnectedBits[] = { Script::USERCONNECTED, Script::REGCONNECTED, Script::OPCONNECTED };

		if(((cur->ui16Functions & iConnectedBits[ui8Type]) == iConnectedBits[ui8Type]) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;

            // PPK ... table of connected functions
            static const char* ConnectedFunction[] = { "UserConnected", "RegConnected", "OpConnected" };

            lua_getglobal(cur->LUA, ConnectedFunction[ui8Type]);
            int i = lua_gettop(cur->LUA);
			if(lua_isnil(cur->LUA, i)) {
				switch(ui8Type) {
					case 0:
						cur->ui16Functions &= ~Script::USERCONNECTED;
						break;
					case 1:
						cur->ui16Functions &= ~Script::REGCONNECTED;
						break;
					case 2:
						cur->ui16Functions &= ~Script::OPCONNECTED;
						break;
				}

                lua_settop(cur->LUA, 0);
                continue;
            }

            ActualUser = u;

            lua_checkstack(cur->LUA, 1); // we need 1 empty slots in stack, check it to be sure

			ScriptPushUser(cur->LUA, u); // usertable

            // 1 passed parameters, zero returned
			if(lua_pcall(cur->LUA, 1, LUA_MULTRET, 0) != 0) {
                ScriptError(cur);

                lua_settop(cur->LUA, 0);
                continue;
            }

            ActualUser = NULL;
            
            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return
        
            int top = lua_gettop(cur->LUA);
        
            // no return value
            if(top == 0) {
                continue;
            }
        
			if(lua_type(cur->LUA, top) != LUA_TBOOLEAN || lua_toboolean(cur->LUA, top) == 0) {
                lua_settop(cur->LUA, 0);
                continue;
            }
       
            // clear the stack for sure
            lua_settop(cur->LUA, 0);

            return; // means DO NOT process by next scripts
        }
    }
}
//------------------------------------------------------------------------------

void ScriptMan::UserDisconnected(User * u) {
	if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    uint8_t ui8Type = 0; // User
    if(u->iProfile != -1) {
        if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            ui8Type = 1; // Reg
		} else {
			ui8Type = 2; // OP
		}
    }

    bMoved = false;

    Script *next = RunningScriptS;
        
    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

		static const uint32_t iDisconnectedBits[] = { Script::USERDISCONNECTED, Script::REGDISCONNECTED, Script::OPDISCONNECTED };

        if(((cur->ui16Functions & iDisconnectedBits[ui8Type]) == iDisconnectedBits[ui8Type]) == true && (bMoved == false || cur->bProcessed == false)) {
            cur->bProcessed = true;

            // PPK ... table of disconnected functions
            static const char* DisconnectedFunction[] = { "UserDisconnected", "RegDisconnected", "OpDisconnected" };

            lua_getglobal(cur->LUA, DisconnectedFunction[ui8Type]);
            int i = lua_gettop(cur->LUA);
            if(lua_isnil(cur->LUA, i)) {
				switch(ui8Type) {
					case 0:
						cur->ui16Functions &= ~Script::USERDISCONNECTED;
						break;
					case 1:
						cur->ui16Functions &= ~Script::REGDISCONNECTED;
						break;
					case 2:
						cur->ui16Functions &= ~Script::OPDISCONNECTED;
						break;
				}

                lua_settop(cur->LUA, 0);
                continue;
            }

            ActualUser = u;

            lua_checkstack(cur->LUA, 1); // we need 1 empty slots in stack, check it to be sure

			ScriptPushUser(cur->LUA, u); // usertable

            // 1 passed parameters, zero returned
			if(lua_pcall(cur->LUA, 1, 0, 0) != 0) {
                ScriptError(cur);

                lua_settop(cur->LUA, 0);
                continue;
            }

            ActualUser = NULL;

            // clear the stack for sure
            lua_settop(cur->LUA, 0);
        }
    }
}
//------------------------------------------------------------------------------

void ScriptMan::PrepareMove(lua_State * L) {
    if(bMoved == true) {
        return;
    }

    bool bBefore = true;

    bMoved = true;

    Script *next = RunningScriptS;
        
    while(next != NULL) {
    	Script *cur = next;
        next = cur->next;

        if(bBefore == true) {
            cur->bProcessed = true;
        } else {
            cur->bProcessed = false;
        }

        if(cur->LUA == L) {
            bBefore = false;
        }
    }
}
//------------------------------------------------------------------------------