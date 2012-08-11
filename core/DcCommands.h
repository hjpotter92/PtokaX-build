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
#ifndef DcCommandsH
#define DcCommandsH
//---------------------------------------------------------------------------
struct User;
struct PassBf;
//---------------------------------------------------------------------------

class cDcCommands {
private:
    PassBf * PasswdBfCheck;
    char msg[1024];
    
	void BotINFO(User * curUser, char * sData, const uint32_t &iLen);
    void ConnectToMe(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck, const bool &bMulti);
	void GetINFO(User * curUser, char * sData, const uint32_t &iLen);
    bool GetNickList(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck);
	void Key(User * curUser, char * sData, const uint32_t &iLen);
	void Kick(User * curUser, char * sData, const uint32_t &iLen);
    bool SearchDeflood(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck, const bool &bMulti) const;
    void Search(User * curUser, char * sData, uint32_t iLen, const bool &bCheck, const bool &bMulti);
    bool MyINFODeflood(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck);
	bool MyINFO(User * curUser, char * sData, const uint32_t &iLen) const;
	void MyPass(User * curUser, char * sData, const uint32_t &iLen);
	void OpForceMove(User * curUser, char * sData, const uint32_t &iLen);
	void RevConnectToMe(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck);
	void SR(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck);
	void Supports(User * curUser, char * sData, const uint32_t &iLen);
    void To(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck);
	void ValidateNick(User * curUser, char * sData, const uint32_t &iLen);
	void Version(User * curUser, char * sData, const uint32_t &iLen);
    bool ChatDeflood(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck) const;
	void Chat(User * curUser, char * sData, const uint32_t &iLen, const bool &bCheck);
	void Close(User * curUser, char * sData, const uint32_t &iLen);
    
    void Unknown(User * curUser, char * sData, const uint32_t &iLen);
    void MyNick(User * pUser, char * sData, const uint32_t &ui32Len);
    
    bool ValidateUserNick(User * curUser, char * Nick, const size_t &szNickLen, const bool &ValidateNick);

	PassBf * Find(const uint8_t * ui128IpHash);
	void Remove(PassBf * PassBfItem);

    bool CheckIP(const User * curUser, const char * sIP) const;
    char * GetPort(char * sData, char cPortEnd, size_t &szPortLen);
    void SendIncorrectIPMsg(User * curUser, char * sBadIP, const bool &bCTM);
    void SendIPFixedMsg(User * pUser, char * sBadIP, char * sRealIP) const;

    void AddActiveSearch(User * pUser, char * sSearch, const size_t &szLen) const;
protected:
public:
	cDcCommands();
    ~cDcCommands();

    void PreProcessData(User * curUser, char * sData, const bool &bCheck, const uint32_t &iLen);
    void ProcessCmds(User * curUser);

    void SRFromUDP(User * curUser, char * sData, const size_t &szLen) const;
    
	uint32_t iStatChat, iStatCmdUnknown, iStatCmdTo, iStatCmdMyInfo, iStatCmdSearch, iStatCmdSR, iStatCmdRevCTM;
	uint32_t iStatCmdOpForceMove, iStatCmdMyPass, iStatCmdValidate, iStatCmdKey, iStatCmdGetInfo, iStatCmdGetNickList;
	uint32_t iStatCmdConnectToMe, iStatCmdVersion, iStatCmdKick, iStatCmdSupports, iStatBotINFO, iStatZPipe;
    uint32_t iStatCmdMultiSearch, iStatCmdMultiConnectToMe, iStatCmdClose;
};

//---------------------------------------------------------------------------
extern cDcCommands *DcCommands;
//---------------------------------------------------------------------------

#endif
