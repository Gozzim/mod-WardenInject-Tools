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

#include "WardenInjectMgr.h"
#include "StringFormat.h"

WardenInjectMgr* WardenInjectMgr::instance()
{
    static WardenInjectMgr instance;
    return &instance;
}

std::string WardenInjectMgr::ReplaceEmptyCurlyBraces(std::string& str) {
    std::string result;

    for (uint32 i = 0; i < str.size(); ++i)
    {
        if (str[i] == '{' && i + 1 < str.size() && str[i + 1] == '}')
        {
            result += "{{}}";
            ++i;
        }
        else
        {
            result += str[i];
        }
    }

    return result;
}

std::string WardenInjectMgr::ReplaceCurlyBraces(std::string& str) {
    std::string result;

    for (char c : str) {
        if (c == '{') {
            result += "{{";
        } else if (c == '}') {
            result += "}}";
        } else {
            result += c;
        }
    }

    return result;
}

WorldPacket CreateAddonPacket(const std::string& prefix, const std::string& msg, ChatMsg msgType, Player* player)
{
    WorldPacket data;

    std::string fullMsg = prefix + "\t" + msg;
    uint32 len = fullMsg.length();

    data.Initialize(SMSG_MESSAGECHAT, 1 + 4 + 8 + 4 + 8 + 4 + 1 + len + 1);
    data << uint8(msgType); //Type
    data << uint32(LANG_ADDON); //Lang
    data << uint64(player->GetGUID().GetRawValue()); //SenderGUID
    data << uint32(0); //Flags
    data << uint64(player->GetGUID().GetRawValue()); //ReceiverGUID
    data << uint32(len + 1); //MsgLen
    data << fullMsg; //Msg
    data << uint8(0);

    return data;
}

void WardenInjectMgr::SendAddonMessage(const std::string& prefix, const std::string& payload, ChatMsg msgType, Player* player)
{
    WorldPacket payloadPacket = CreateAddonPacket(prefix, payload, msgType, player);
    player->SendDirectMessage(&payloadPacket);
}

WardenPayloadMgr* GetWardenPayloadMgr(WorldSession* session)
{
    if (!session)
    {
        return nullptr;
    }

    WardenWin* warden = (WardenWin*)session->GetWarden();
    if (!warden)
    {
        return nullptr;
    }

    if (!warden->IsInitialized())
    {
        return nullptr;
    }

    return warden->GetPayloadMgr();
}

void WardenInjectMgr::SendLargePayload(Player* player, const std::string& addon, float version, bool cache, bool comp, const std::string& data)
{
    std::string payloadData = data; //TODO: better solution?
    std::string compressedData;

    if (comp == 1)
    {
        compressedData = std::get<0>(sLZW->Compress(payloadData));
        if (!compressedData.empty())
        {
            payloadData = compressedData;
        }
        else
        {
            comp = 0;
        }
    }

    const uint32 maxChunkSize = MAX_PAYLOAD_SIZE;
    std::vector<std::string> chunks;

    for (uint32 i = 0; i < payloadData.length(); i += maxChunkSize)
    {
        chunks.push_back(payloadData.substr(i, maxChunkSize));
    }

    if (chunks.size() > 99)
    {
        return;
    }

    std::string payloadSizeStr = std::to_string(chunks.size());
    if (chunks.size() < 10)
    {
        payloadSizeStr = "0" + payloadSizeStr;
    }

    for (uint32 i = 0; i < chunks.size(); i++)
    {
        std::string chunkNumStr = std::to_string(i + 1);
        if (i + 1 < 10)
        {
            chunkNumStr = "0" + chunkNumStr;
        }

        std::string payload = Acore::StringFormatFmt("_G['{}'].f.p('{}', '{}', '{}', {}, {}, {}, [[{}]]);", sWardenInjectConfigMgr->cGTN, chunkNumStr, payloadSizeStr, addon, version, cache, comp, chunks[i].c_str());

        SendAddonMessage("ws", payload, CHAT_MSG_WHISPER, player);
    }
}

void WardenInjectMgr::SendPayloadInform(Player* player)
{
    for (uint16 i = 0; i < sWardenInjectConfigMgr->GetWardenScriptsMap().size(); i++)
    {
        for (auto& [payloadName, data] : sWardenInjectConfigMgr->GetWardenScriptsMap()) {
            if (data.GetOrderNum() != i) {
                continue;
            }

            if (!data.IsOnLogin()) {
                continue;
            }
            // register all saved variables if there are any
            for (const auto& var : data.GetSavedVars()) {
                SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
            }

            LOG_DEBUG("module", "Sending payload: {} with orderNumber {}", payloadName, data.GetOrderNum());
            std::string message = Acore::StringFormatFmt("_G['{}'].f.i('{}', {}, {}, {}, {})", sWardenInjectConfigMgr->cGTN, payloadName, data.GetVersion(), data.IsCached(), data.IsCompressed(), data.GetOrderNum());
            SendAddonMessage("ws", message, CHAT_MSG_WHISPER, player);
        }
    }
}

void WardenInjectMgr::SendSpecificPayload(Player* player, std::string payloadName)
{
    if (!sWardenInjectConfigMgr->WardenScriptLoaded(payloadName))
    {
        LOG_ERROR("module", "Specific Payload not found: {}", payloadName);
        return;
    }

    WardenInjectData& data = sWardenInjectConfigMgr->GetWardenScript(payloadName);

    // register all saved variables if there are any
    for (const auto& var : data.GetSavedVars()) {
        SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
    }

    LOG_DEBUG("module", "Sending specific payload: {} with orderNumber {}", payloadName, data.GetOrderNum());
    std::string message = Acore::StringFormatFmt("_G['{}'].f.i('{}', {}, {}, {}, {})", sWardenInjectConfigMgr->cGTN, payloadName, data.GetVersion(), data.IsCached(), data.IsCompressed(), data.GetOrderNum());
    SendAddonMessage("ws", message, CHAT_MSG_WHISPER, player);
}

void WardenInjectMgr::SendAddonInjector(Player* player)
{
    // Overwrite reload function
    SendAddonMessage("ws", "local copy,new=_G['ReloadUI'];function new() SendAddonMessage('wc', 'reload', 'WHISPER', UnitName('player')) copy() end _G['ReloadUI'] = new", CHAT_MSG_WHISPER, player);
    SendAddonMessage("ws", "SlashCmdList['RELOAD'] = function() _G['ReloadUI']() end", CHAT_MSG_WHISPER, player);

    // Generate helper functions to load larger datasets
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'] = { f = {}, s = {}, i = {0}, d = " + dbg + "};", CHAT_MSG_WHISPER, player);
    // Load
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.l = function(s, n) local t=_G['" + sWardenInjectConfigMgr->cGTN + "'].i; forceinsecure(); loadstring(s)(); if(t.d) then print('[WardenLoader]: '..n..' loaded!') end t[1] = t[1]+1; if(t[1] == t[2]) then SendAddonMessage('wc', 'kill', 'WHISPER', UnitName('player')) end end", CHAT_MSG_WHISPER, player);
    // Concatenate
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.c = function(a) local b='' for _,d in ipairs(a) do b=b..d end; return b end", CHAT_MSG_WHISPER, player);
    // Execute
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.e = function(n) local t=_G['" + sWardenInjectConfigMgr->cGTN + "']; local lt = t.s[n] local fn = t.f.c(lt.ca); _G[n..'payload'] = {v = lt.v, p = fn}; if(lt.co==1) then fn = lualzw.decompress(fn) end t.f.l(fn, n) t.s[n]=nil end", CHAT_MSG_WHISPER, player);
    // Process
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.p = function(a, b, n, v, c, co, s) local t,tc=_G['" + sWardenInjectConfigMgr->cGTN + "'], _G['" + sWardenInjectConfigMgr->cGTN + "'].s; if not tc[n] then tc[n] = {['v']=v, ['co']=co, ['c']=c, ['ca']={}} end local lt = tc[n] a=tonumber(a) b=tonumber(b) table.insert(lt.ca, a, s) if a == b and #lt.ca == b then t.f.e(n) end end", CHAT_MSG_WHISPER, player);
    // Inform
    // One potential issue is dependency load order and requirement, this is something I'll have to look into at some point..
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.i = function(n, v, c, co, s) local t=_G['" + sWardenInjectConfigMgr->cGTN + "']; if not(t.i[2]) then t.i[2] = tonumber(s) end if(c == 1) then RegisterForSave(n..'payload'); local cc = _G[n..'payload'] if(cc) then if(cc.v == v) then local p = cc.p if(co == 1) then p = lualzw.decompress(p) end t.f.l(p, n) return; end end end SendAddonMessage('wc', 'req'..n, 'WHISPER', UnitName('player')) end", CHAT_MSG_WHISPER, player);
    // cache registry
    SendAddonMessage("ws", "_G['" + sWardenInjectConfigMgr->cGTN + "'].f.r = function(a) RegisterForSave(a); end", CHAT_MSG_WHISPER, player);

    // Sends an inform to the player about the available payloads
    SendPayloadInform(player);
}

void WardenInjectMgr::PushInitModule(Player* player)
{
    SendAddonInjector(player);
    SendAddonMessage("ws", "SendAddonMessage('wc', 'loaded', 'WHISPER', UnitName('player')); if(_G['" + sWardenInjectConfigMgr->cGTN + "'].d) then print('[WardenLoader]: Injection successful.') end", CHAT_MSG_WHISPER, player);
}

void WardenInjectMgr::InitialInjection(Player* player)
{
    WorldSession* session = player->GetSession();

    if (!session)
    {
        return;
    }

    WardenWin* warden = (WardenWin*)session->GetWarden();

    if (!warden)
    {
        LOG_ERROR("module", "WardenInjectMgr::InitialInjection - Warden could not be found for player {}.", player->GetName());
        return;
    }

    if (!warden->IsInitialized())
    {
        LOG_ERROR("module", "WardenInjectMgr::InitialInjection - Warden is not initialized for player {}.", player->GetName());
        return;
    }

    auto payloadMgr = warden->GetPayloadMgr();
    if (!payloadMgr)
    {
        LOG_ERROR("module", "WardenInjectMgr::InitialInjection - Payload Manager could not be found for player {}.", player->GetName());
        return;
    }

    if (!payloadMgr->GetPayloadById(9601))
    {
        payloadMgr->RegisterPayload(helperFuns, 9601);
    }

    if (!payloadMgr->GetPayloadById(9602))
    {
        payloadMgr->RegisterPayload(execMsgEvent, 9602);
    }

    // Push to front of queue if NumLuaChecks > 1 as it reverts the order
    // NumLuaChecks > 1 right now causes a weird behavior, rather don't use it
    bool pushToFront = sWorld->getIntConfig(CONFIG_WARDEN_NUM_LUA_CHECKS) > 1;

    // TODO: Add config for forced checks
    warden->ForceChecks();
    payloadMgr->QueuePayload(9601, pushToFront);
    warden->ForceChecks();
    payloadMgr->QueuePayload(9602, pushToFront);
    warden->ForceChecks();

    // TODO: add verbose config
    ChatHandler(player->GetSession()).PSendSysMessage("[WardenLoader]: Awaiting injection...");
}

void WardenInjectMgr::OnAddonMessageReceived(Player* player, uint32 type, const std::string& header, const std::string& data)
{
    if (type != CHAT_MSG_WHISPER)
    {
        return;
    }

    if (header != "wc")
    {
        return;
    }

    if (data == "reload")
    {
        // flag the player for re-injection if they reloaded their ui
        InitialInjection(player);
    }
    else if (data == "init")
    {
        LOG_DEBUG("module", "WardenInjectMgr::OnAddonMessageReceived - Received initialized from player {}.", player->GetName());
        PushInitModule(player);
    }
    else if (data == "loaded")
    {
        // module is loaded and ready to receive data
        LOG_DEBUG("module", "WardenInjectMgr::OnAddonMessageReceived - Received loaded command from player {}.", player->GetName());
    }
    else if (data == "kill")
    {
        LOG_DEBUG("module", "WardenInjectMgr::OnAddonMessageReceived - Received kill command from player {}.", player->GetName());
        // kill the initial loader, this is to prevent spoofed addon packets with access to the protected namespace
        // the initial injector can not be used after this point, and the injected helper functions above are the ones that need to be used.
        SendAddonMessage("ws", "false", CHAT_MSG_WHISPER, player);

        // if the below is printed in your chat, then the initial injector was not disabled and you have problems to debug :)
        SendAddonMessage("ws", "print('[WardenLoader]: This message should never show.')", CHAT_MSG_WHISPER, player);
    }
    else if (data.substr(0, 3) == "req")
    {
        std::string addon = data.substr(3);
        LOG_DEBUG("module", "WardenInjectMgr::OnAddonMessageReceived - Received request for addon {} from player {}.", addon, player->GetName());

        if (!sWardenInjectConfigMgr->WardenScriptLoaded(addon))
        {
            LOG_ERROR("module", "Requested Payload not found: {}", addon);
            return;
        }

        WardenInjectData& data = sWardenInjectConfigMgr->GetWardenScript(addon);
        SendLargePayload(player, addon, data.GetVersion(), data.IsCached(), data.IsCompressed(), data.GetPayload());
    }
}

