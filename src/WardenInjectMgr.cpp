/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
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

void WardenInjectMgr::LoadConfig(bool reload)
{
    WardenInjectEnabled = sConfigMgr->GetOption<bool>("WardenInject.Enable", true);
    Announce = sConfigMgr->GetOption<bool>("WardenInject.Announce", true);
    ScriptsPath = sConfigMgr->GetOption<std::string>("WardenInject.ScriptsPath", "~/acore-server/payloads");
    OnLoginInject = sConfigMgr->GetOption<bool>("WardenInject.OnLogin", true);

    LoadOnLoginScripts(reload);
}

void WardenInjectMgr::LoadOnLoginScripts(bool reload)
{
    bool changed = false;

    std::vector<std::string> scripts = sConfigMgr->GetKeysByString("WardenInject.Scripts.");
    for (std::string const& script : scripts)
    {
        std::string configName = script.substr(strlen("WardenInject.Scripts."));
        std::string scriptName = configName.substr(0, configName.find("."));
        std::string scriptConfig = configName.substr(configName.find(".") + 1);

        if (scriptConfig == "Version")
        {
            wardenScriptsMap[scriptName].SetVersion(sConfigMgr->GetOption<float>(script, 1.0));
        }
        else if (scriptConfig == "Compressed")
        {
            wardenScriptsMap[scriptName].SetCompressed(sConfigMgr->GetOption<bool>(script, false));
        }
        else if (scriptConfig == "Cached")
        {
            wardenScriptsMap[scriptName].SetCached(sConfigMgr->GetOption<bool>(script, true));
        }
        else if (scriptConfig == "OnLogin")
        {
            wardenScriptsMap[scriptName].SetOnLogin(sConfigMgr->GetOption<bool>(script, true));
        }
        else if (scriptConfig == "Path")
        {
            std::string filePath = sConfigMgr->GetOption<std::string>(script, "");
            std::string payload = GetPayloadFromFile(filePath);
            if (payload.empty())
            {
                LOG_WARN("module", "WardenInject: Failed to load payload from file {}", filePath);
                continue;
            }
            wardenScriptsMap[scriptName].SetPayload(payload);
        }
        else if (scriptConfig == "SavedVars")
        {
            std::istringstream savedVars(sConfigMgr->GetOption<std::string>(script, ""));
            std::string var;
            while (std::getline(savedVars, var, ';'))
            {
                wardenScriptsMap[scriptName].AddSavedVar(var);
            }
        }
        else
        {
            LOG_WARN("module", "WardenInject: Unknown config {}", configName);
        }
    }

    // ToDo: Send reload message to all players
}

int32 WardenInjectMgr::GetOrderNumber(std::string payloadName)
{
    auto it = wardenScriptsMap.find(payloadName);

    if (it != wardenScriptsMap.end()) {
        int32 index = std::distance(wardenScriptsMap.begin(), it);
        return index;
    }

    LOG_WARN("module", "WardenInject: OrderNumber for {} not found", payloadName);
    return -1;
}

std::string WardenInjectMgr::ReplaceEmptyCurlyBraces(std::string str) {
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

std::string WardenInjectMgr::ReplaceCurlyBraces(std::string str) {
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

// Loads the contents of a Lua file into a string variable
std::string WardenInjectMgr::LoadLuaFile(const std::string& filePath)
{
    std::string luaCode;

#ifdef WIN32
    std::replace(filename.begin(), filename.end(), '/', '\\');
#endif

    std::ifstream file(filePath);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        luaCode = buffer.str();
    }
    else
    {
        LOG_ERROR("module", "WardenInjectMgr::LoadLuaFile - Loading file {} failed.", filePath);
    }

    file.close();

    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::LoadLuaFile - File {} is empty.", filePath);
    }

    return luaCode;
}

// Converts the Lua code into a payload that can be sent to the client
// This is done by removing comments, line breaks and whitespaces
// It is not perfect but does the job
void WardenInjectMgr::ConvertToPayload(std::string& luaCode)
{
    // Trim whitespace from the beginning and end of the string
    Acore::String::Trim(luaCode);

    // Remove comment blocks
    std::regex commentBlockRegex("--\\[\\[[\\s\\S]*?\\]\\]");
    luaCode = std::regex_replace(luaCode, commentBlockRegex, "");

    // Remove comments
    std::regex commentRegex("--.*\r?\n");
    luaCode = std::regex_replace(luaCode, commentRegex, "\n");

    // Remove leading tabs and whitespaces
    std::regex leadingSpaceRegex("\n[ \t]+", std::regex_constants::optimize);
    luaCode = std::regex_replace(luaCode, leadingSpaceRegex, "\n");

    // Remove empty lines
    std::regex emptyLineRegex("([ \t]*\r?\n)", std::regex_constants::optimize);
    luaCode = std::regex_replace(luaCode, emptyLineRegex, "\n");

    // Replace line breaks with spaces
    std::regex lineBreakRegex("\r?\n");
    luaCode = std::regex_replace(luaCode, lineBreakRegex, " ");

}

std::string WardenInjectMgr::GetPayloadFromFile(std::string filePath)
{
    std::string luaCode = LoadLuaFile(filePath);
    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::GetPayloadFromFile - Code from file {} is empty.", filePath);
    }

    ConvertToPayload(luaCode);
    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::GetPayloadFromFile - Code from file {} is empty after removing whitespaces.", filePath);
    }

    return luaCode;
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
    std::vector <uint8_t> compressedBytes;
    /* TODO
    if (comp == 1)
    {
        compressedBytes = compress(data);
        if (compressedBytes.empty())
        {
            comp = 0;
        }
    }
    */
    const uint32_t maxChunkSize = 900;
    std::vector<std::string> chunks;
    for (uint32 i = 0; i < data.length(); i += maxChunkSize)
    {
        chunks.push_back(data.substr(i, maxChunkSize));
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
        std::string payload = Acore::StringFormatFmt("_G['{}'].f.p('{}', '{}', '{}', {}, {}, {}, [[{}]]);", cGTN, chunkNumStr, payloadSizeStr, addon, version, cache, comp, chunks[i].c_str());
        if (comp == 1)
        {
            std::string payload = Acore::StringFormatFmt("_G['{}'].f.p('{}', '{}', '{}', {}, {}, {}, [[{}]]);", cGTN, chunkNumStr, payloadSizeStr, addon, version, cache, comp, (std::string(compressedBytes.begin(), compressedBytes.end())));
        }

        SendAddonMessage("ws", payload, CHAT_MSG_WHISPER, player);
    }
}

void WardenInjectMgr::SendPayloadInform(Player* player)
{
    uint32 orderNum = 0;
    for (auto& [payloadName, data] : wardenScriptsMap) {
        if (!data.IsOnLogin()) {
            orderNum++;
            continue;
        }
        // register all saved variables if there are any
        for (const auto& var : data.GetSavedVars()) {
            SendAddonMessage("ws", "_G['" + cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
        }

        std::string message = Acore::StringFormatFmt("_G['{}'].f.i('{}', {}, {}, {}, {})", cGTN, payloadName, data.GetVersion(), data.IsCached(), data.IsCompressed(), orderNum++);
        SendAddonMessage("ws", message, CHAT_MSG_WHISPER, player);
    }
}

void WardenInjectMgr::SendSpecificPayload(Player* player, std::string payloadName)
{
    WardenInjectData& data = wardenScriptsMap[payloadName];
    if (wardenScriptsMap.find(payloadName) == wardenScriptsMap.end())
    {
        LOG_ERROR("module", "Specific Payload not found: {}", payloadName);
        return;
    }

    // register all saved variables if there are any
    for (const auto& var : data.GetSavedVars()) {
        SendAddonMessage("ws", "_G['" + cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
    }

    std::string message = Acore::StringFormatFmt("_G['{}'].f.i('{}', {}, {}, {}, {})", cGTN, payloadName, data.GetVersion(), data.IsCached(), data.IsCompressed(), GetOrderNumber(payloadName));
    SendAddonMessage("ws", message, CHAT_MSG_WHISPER, player);
}

void WardenInjectMgr::SendAddonInjector(Player* player)
{
    // Overwrite reload function
    SendAddonMessage("ws", "local copy,new=_G['ReloadUI'];function new() SendAddonMessage('wc', 'reload', 'WHISPER', UnitName('player')) copy() end _G['ReloadUI'] = new", CHAT_MSG_WHISPER, player);
    SendAddonMessage("ws", "SlashCmdList['RELOAD'] = function() _G['ReloadUI']() end", CHAT_MSG_WHISPER, player);

    // Generate helper functions to load larger datasets
    SendAddonMessage("ws", "_G['" + cGTN + "'] = { f = {}, s = {}, i = {0}, d = " + dbg + "};", CHAT_MSG_WHISPER, player);
    // Load
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.l = function(s, n) local t=_G['" + cGTN + "'].i; forceinsecure(); loadstring(s)(); if(t.d) then print('[WardenLoader]: '..n..' loaded!') end t[1] = t[1]+1; if(t[1] == t[2]) then SendAddonMessage('wc', 'kill', 'WHISPER', UnitName('player')) end end", CHAT_MSG_WHISPER, player);
    // Concatenate
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.c = function(a) local b='' for _,d in ipairs(a) do b=b..d end; return b end", CHAT_MSG_WHISPER, player);
    // Execute
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.e = function(n) local t=_G['" + cGTN + "']; local lt = t.s[n] local fn = t.f.c(lt.ca); _G[n..'payload'] = {v = lt.v, p = fn}; if(lt.co==1) then fn = lualzw.decompress(fn) end t.f.l(fn, n) t.s[n]=nil end", CHAT_MSG_WHISPER, player);
    // Process
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.p = function(a, b, n, v, c, co, s) local t,tc=_G['" + cGTN + "'], _G['" + cGTN + "'].s; if not tc[n] then tc[n] = {['v']=v, ['co']=co, ['c']=c, ['ca']={}} end local lt = tc[n] a=tonumber(a) b=tonumber(b) table.insert(lt.ca, a, s) if a == b and #lt.ca == b then t.f.e(n) end end", CHAT_MSG_WHISPER, player);
    // Inform
    // One potential issue is dependency load order and requirement, this is something I'll have to look into at some point..
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.i = function(n, v, c, co, s) local t=_G['" + cGTN + "']; if not(t.i[2]) then t.i[2] = tonumber(s) end if(c == 1) then RegisterForSave(n..'payload'); local cc = _G[n..'payload'] if(cc) then if(cc.v == v) then local p = cc.p if(co == 1) then p = lualzw.decompress(p) end t.f.l(p, n) return; end end end SendAddonMessage('wc', 'req'..n, 'WHISPER', UnitName('player')) end", CHAT_MSG_WHISPER, player);
    // cache registry
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.r = function(a) RegisterForSave(a); end", CHAT_MSG_WHISPER, player);

    // Sends an inform to the player about the available payloads
    SendPayloadInform(player);
}

void WardenInjectMgr::PushInitModule(Player* player)
{
    SendAddonInjector(player);
    SendAddonMessage("ws", "SendAddonMessage('wc', 'loaded', 'WHISPER', UnitName('player')); if(_G['" + cGTN + "'].d) then print('[WardenLoader]: Injection successful.') end", CHAT_MSG_WHISPER, player);
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

        if(wardenScriptsMap.find(addon) == wardenScriptsMap.end())
        {
            LOG_ERROR("module", "Requested Payload not found: {}", addon);
            return;
        }

        WardenInjectData& data = wardenScriptsMap[addon];
        SendLargePayload(player, addon, data.GetVersion(), data.IsCached(), data.IsCompressed(), data.GetPayload());
    }
}
