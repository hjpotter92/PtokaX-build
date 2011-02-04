/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2011  Petr Kozelka, PPK at PtokaX dot org

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
#ifndef SettingXmlH
#define SettingXmlH
//---------------------------------------------------------------------------

const char* SetBoolXmlStr[] = {
    "AntiMoGlo", 
    "AutoStart", 
    "RedirectAll", 
    "RedirectWhenHubFull", 
    "AutoRegister", 
    "RegOnly", 
    "RegOnlyRedir", 
    "ShareLimitRedir", 
    "SlotsLimitRedir", 
    "HubSlotRatioRedir", 
    "MaxHubsLimitRedir", 
    "ModeToMyINFO", 
    "ModeToDescription", 
    "StripDescription", 
    "StripTag", 
    "StripConnection", 
    "StripEmail", 
    "RegBot", 
    "UseBotNickAsHubSec", 
    "RegOpChat", 
    "TempBanRedir", 
    "PermBanRedir", 
    "EnableScripting", 
    "KeepSlowUsers", 
    "CheckNewReleases", 
    "EnableTrayIcon", 
    "StartMinimized", 
    "FilterKickMessages", 
    "SendKickMessagesToOps", 
    "SendStatusMessages", 
    "SendStatusMessagesAsPm", 
    "EnableTextFiles", 
    "SendTextFilesAsPm", 
    "StopScriptOnError", 
    "MOTDAsPm", 
    "DefloodReport", 
    "ReplyToHubCommandsAsPm", 
    "DisableMOTD", 
    "DontAllowPingers", 
    "ReportPingers", 
    "Report3xBadPass", 
    "AdvancedPassProtection", 
    "BindOnlySingleIp", 
    "ResolveToIp", 
    "NickLimitRedir", 
    "SendUserIP2ToUserOnLogin", 
    "BanMessageShowIp", 
    "BanMessageShowRange", 
    "BanMessageShowNick", 
    "BanMessageShowReason", 
    "BanMessageShowBy", 
    "ReportSuspiciousTag", 
    "AcceptUnknownTag", 
    "CheckIpInCommands", 
    "PopupScriptsWindow", 
    "LogScriptErrors", 
    "NoQuackSupports", 
};

const char* SetShortXmlStr[] = {
    "MaxUsers", 
    "MinShareLimit", 
    "MinShareUnits", 
    "MaxShareLimit", 
    "MaxShareUnits", 
    "MinSlotsLimit", 
    "MaxSlotsLimit", 
    "HubSlotRatioHubs", 
    "HubSlotRatioSlots", 
    "MaxHubsLimit", 
    "NoTagOption", 
    "FullMyINFOOption", 
    "MaxChatLen", 
    "MaxChatLines", 
    "MaxPmLen", 
    "MaxPmLines", 
    "DefaultTempBanTime", 
    "MaxPassiveSr", 
    "MyINFODelay", 
    "MainChatMessages", 
    "MainChatTime", 
    "MainChatAction", 
    "SameMainChatMessages", 
    "SameMainChatTime", 
    "SameMainChatAction", 
    "SameMultiMainChatMessages", 
    "SameMultiMainChatLines", 
    "SameMultiMainChatAction", 
    "PmMessages", 
    "PmTime", 
    "PmAction", 
    "SamePmMessages", 
    "SamePmTime", 
    "SamePmAction", 
    "SameMultiPmMessages", 
    "SameMultiPmLines", 
    "SameMultiPmAction", 
    "SearchMessages", 
    "SearchTime", 
    "SearchAction", 
    "SameSearchMessages", 
    "SameSearchTime", 
    "SameSearchAction", 
    "MyINFOMessages", 
    "MyINFOTime", 
    "MyINFOAction", 
    "GetNickListMessages", 
    "GetNickListTime", 
    "GetNickListAction", 
    "NewConnectionsCount", 
    "NewConnectionsTime", 
    "DefloodWarningCount", 
    "DefloodWarningAction", 
    "DefloodTempBanTime", 
    "GlobalMainChatMessages", 
    "GlobalMainChatTime", 
    "GlobalMainChatTimeOut", 
    "GlobalMainChatAction", 
    "MinSearchLen", 
    "MaxSearchLen", 
    "MinNickLen", 
    "MaxNickLen", 
    "BruteForcePassProtectBanType", 
    "BruteForcePassProtectTempBanTime", 
    "MaxPmCountToUser", 
    "MaxSimultaneousLogins", 
    "MainChatMessages2", 
    "MainChatTime2", 
    "MainChatAction2", 
    "PmMessages2", 
    "PmTime2", 
    "PmAction2", 
    "SearchMessages2", 
    "SearchTime2", 
    "SearchAction2", 
    "MyINFOMessages2", 
    "MyINFOTime2", 
    "MyINFOAction2", 
    "MaxMyINFOLen", 
    "CTMMessages", 
    "CTMTime", 
    "CTMAction", 
    "CTMMessages2", 
    "CTMTime2", 
    "CTMAction2", 
    "RCTMMessages", 
    "RCTMTime", 
    "RCTMAction", 
    "RCTMMessages2", 
    "RCTMTime2", 
    "RCTMAction2", 
    "MaxCTMLen", 
    "MaxRCTMLen", 
    "SRMessages", 
    "SRTime", 
    "SRAction", 
    "SRMessages2", 
    "SRTime2", 
    "SRAction2", 
    "MaxSRLen", 
    "MaxDownAction", 
    "MaxDownKb", 
    "MaxDownTime", 
    "MaxDownAction2", 
    "MaxDownKb2", 
    "MaxDownTime2", 
    "ChatIntervalMessages", 
    "ChatIntervalTime", 
    "PMIntervalMessages", 
    "PMIntervalTime", 
    "SearchIntervalMessages", 
    "SearchIntervalTime", 
    "MaxConnSameIP", 
    "MinReConnTime", 
    "MaxUsersPeak", 
};

const char* SetTxtXmlStr[] = {
    "HubName", 
    "AdminNick", 
    "HubAddress", 
    "TCPPorts", 
    "UDPPort", 
    "HubDescription", 
    "RedirectAddress", 
    "RegisterServers", 
    "RegOnlyMessage", 
    "RegOnlyRedirAddress", 
    "HubTopic", 
    "ShareLimitMessage", 
    "ShareLimitRedirAddress", 
    "SlotsLimitMessage", 
    "SlotsLimitRedirAddress", 
    "HubSlotRatioMessage", 
    "HubSlotRatioRedirAddress", 
    "MaxHubsLimitMessage", 
    "MaxHubsLimitRedirAddress", 
    "NoTagMessage", 
    "NoTagRedirAddress", 
    "BotNick", 
    "BotDescription", 
    "BotEmail", 
    "OpChatNick", 
    "OpChatDescription", 
    "OpChatEmail", 
    "TempBanRedirAddress", 
    "PermBanRedirAddress", 
    "ChatCommandsPrefixes", 
    "HubOwnerEmail", 
    "NickLimitMessage", 
    "NickLimitRedirAddress", 
    "MessageToAddToBanMessage", 
    "Language", 
};
//---------------------------------------------------------------------------

#endif
