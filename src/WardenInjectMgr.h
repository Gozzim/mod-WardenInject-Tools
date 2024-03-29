/*
 * This file was written for the AzerothCore Project.
 * Code and Implementation: Gozzim (https://github.com/Gozzim/mod-WardenInject-Tools)
 * Proof of Concept: Foereaper (https://github.com/Foereaper/WardenInject)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WARDENINJECTMGR_H_
#define _WARDENINJECTMGR_H_

#include "Common.h"
#include "Chat.h"
#include "Log.h"
#include "LZW.h"
#include "Player.h"
#include "SocialMgr.h"
#include "WardenInjectConfigMgr.h"
#include "WardenInjectData.h"
#include "WardenInjectFileMgr.h"
#include "WardenWin.h"
#include "World.h"
#include "WorldPacket.h"

#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

#define MAX_PAYLOAD_SIZE 900
#define dbg "true"

class WardenInjectData;

class WardenInjectMgr
{
public:
    static WardenInjectMgr* instance();

    const std::string helperFuns = "wh=function(a,c,d) if a=='ws' and c=='WHISPER' and d==UnitName('player') then return true end return false end wr=function() SendAddonMessage('wc', 'init', 'WHISPER', UnitName('player')) end";
    const std::string execMsgEvent = "local f=CreateFrame('Frame');f:RegisterEvent('CHAT_MSG_ADDON');f:SetScript('OnEvent', function(s,_,a,b,c,d) if _G['wh'](a, c, d) then forceinsecure(); loadstring(b)() end end) _G['wr']();";

    std::string ReplaceEmptyCurlyBraces(std::string& str);
    std::string ReplaceCurlyBraces(std::string& str);

    void InitialInjection(Player* player);
    void PushInitModule(Player* player);
    void SendAddonMessage(const std::string& prefix, const std::string& payload, ChatMsg msgType, Player* player);
    void SendAddonInjector(Player* player);
    void SendPayloadInform(Player* player);
    void SendLargePayload(Player* player, const std::string& addon, float version, bool cache, bool comp, const std::string& data);
    void SendSpecificPayload(Player* player, std::string payloadName);
    void OnAddonMessageReceived(Player* player, uint32 type, const std::string& header, const std::string& data);
};

#define sWardenInjectMgr WardenInjectMgr::instance()

#endif
