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


std::vector<uint8_t> compress(const std::string& data) {
    std::unordered_map<std::string, int> dictionary;
    for (int i = 0; i < 256; i++) {
        dictionary[std::string(1, (char)i)] = i;
    }
    std::string buffer;
    std::vector<uint8_t> result;
    for (char c : data) {
        std::string s = buffer + c;
        if (dictionary.count(s) > 0) {
            buffer = s;
        } else {
            result.push_back(dictionary[buffer]);
            dictionary[s] = dictionary.size();
            buffer = std::string(1, c);
        }
    }
    if (!buffer.empty()) {
        result.push_back(dictionary[buffer]);
    }
    return result;
}

std::string decompress(const std::vector<uint8_t>& data) {
    std::unordered_map<int, std::string> dictionary;
    for (int i = 0; i < 256; i++) {
        dictionary[i] = std::string(1, (char)i);
    }
    std::string buffer = dictionary[data[0]];
    std::string result = buffer;
    std::string entry;
    for (int i = 1; i < data.size(); i++) {
        int code = data[i];
        if (dictionary.count(code) > 0) {
            entry = dictionary[code];
        } else if (code == dictionary.size()) {
            entry = buffer + buffer[0];
        } else {
            throw std::runtime_error("Invalid LZW code");
        }
        result += entry;
        dictionary[dictionary.size()] = buffer + entry[0];
        buffer = entry;
    }
    return result;
}

WorldPacket CreateAddonPacket(const std::string& prefix, const std::string& msg, ChatMsg msgType, Player* player)
{
    WorldPacket data;

    std::string fullMsg = prefix + "\t" + msg;
    size_t len = fullMsg.length();

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
    std::ifstream file(filePath);
    if (file.fail())
    {
        LOG_ERROR("module", "WardenInjectMgr::LoadLuaFile - Loading file {} failed.", filePath);
        return nullptr;
    }

    std::string luaCode;
    char buffer[1024];

    while (file.read(buffer, sizeof(buffer)))
    {
        luaCode.append(buffer, sizeof(buffer));
    }
    luaCode.append(buffer, file.gcount());
    file.close();

    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::LoadLuaFile - File {} is empty.", filePath);
        return nullptr;
    }

    return luaCode;
}

// Removes leading whitespaces and replaces line breaks with spaces
void WardenInjectMgr::ConvertToPayload(std::string& luaCode)
{
    // Trim whitespace from the beginning and end of the string
    Acore::String::Trim(luaCode);

    // Remove unwanted characters (all whitespace except spaces)
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\t'), luaCode.end());
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\n'), luaCode.end());
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\r'), luaCode.end());

    // Remove non-printable characters
    luaCode.erase(std::remove_if(luaCode.begin(), luaCode.end(), [](unsigned char c) { return !std::isprint(c); }), luaCode.end());
}

std::string WardenInjectMgr::GetPayloadFromFile(std::string filePath)
{
    std::string luaCode = LoadLuaFile(filePath);
    if (luaCode.empty())
    {
        return nullptr;
    }

    ConvertToPayload(luaCode);
    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::GetPayloadFromFile - Code from file {} is empty after removing whitespaces.", filePath);
        return nullptr;
    }

    return luaCode;
}

std::vector <std::string> WardenInjectMgr::GetChunks(std::string s, uint8_t chunkSize)
{
    std::vector <std::string> chunks;

    for (uint32_t i = 0; i < s.size(); i += chunkSize)
    {
        chunks.push_back(s.substr(i, chunkSize));
    }

    return chunks;
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

bool WardenInjectMgr::SendWardenInject(Player* player, std::string payload)
{
    if (payload.empty())
    {
        LOG_ERROR("module", "WardenInjectMgr::SendWardenInject - Payload for player {} is empty.", player->GetName());
        return false;
    }
    
    WorldSession* session = player->GetSession();

    if (!session)
    {
        return false;
    }

    WardenWin* warden = (WardenWin*)session->GetWarden();
    
    if (!warden)
    {
        LOG_ERROR("module", "WardenInjectMgr::SendWardenInject - Warden could not be found for player {}.", player->GetName());
        return false;
    }

    if (!warden->IsInitialized())
    {
        LOG_ERROR("module", "WardenInjectMgr::SendWardenInject - Warden is not initialized for player {}.", player->GetName());
        return false;
    }

    auto payloadMgr = warden->GetPayloadMgr();
    if (!payloadMgr)
    {
        LOG_ERROR("module", "WardenInjectMgr::SendWardenInject - Payload Manager could not be found for player {}.", player->GetName());
        return false;
    }

    for (uint32 i = 0; i < payloadMgr->GetPayloadCountInQueue(); i++)
    {
        warden->ForceChecks();
    }

    auto chunks = GetChunks(payload, 128);

    if (!payloadMgr->GetPayloadById(_prePayloadId))
    {
        payloadMgr->RegisterPayload(_prePayload, _prePayloadId);
    }

    payloadMgr->QueuePayload(_prePayloadId);
    warden->ForceChecks();

    for (auto const& chunk : chunks)
    {
        auto smallPayload = "wlbuf = wlbuf .. [[" + chunk + "]];";
        payloadMgr->RegisterPayload(smallPayload, _tmpPayloadId, true);
        uint32 payloadId = payloadMgr->RegisterPayload(payload);
        payloadMgr->QueuePayload(payloadId);
        warden->ForceChecks();

        LOG_INFO("module", "Sent payload '{}'.", smallPayload);
    }

    if (!payloadMgr->GetPayloadById(_postPayloadId))
    {
        payloadMgr->RegisterPayload(_postPayload, _postPayloadId);
    }

    payloadMgr->QueuePayload(_postPayloadId);
    warden->ForceChecks();

    return true;
}

void SendLargePayload(Player* player, const std::string& addon, uint8_t version, uint8_t cache, uint8_t comp, const std::string& data) {
    comp = comp > 0 ? 1 : 0;
    cache = cache > 0 ? 1 : 0;
    std::vector<uint8_t> compressedBytes;
    if (comp == 1) {
        compressedBytes = compress(data);
        if (compressedBytes.empty()) {
            comp = 0;
        }
    }
    const uint32_t maxChunkSize = 900;
    std::vector<std::string> chunks;
    for (size_t i = 0; i < data.length(); i += maxChunkSize) {
        chunks.push_back(data.substr(i, maxChunkSize));
    }
    if (chunks.size() > 99) {
        return;
    }
    std::string payloadSizeStr = std::to_string(chunks.size());
    if (chunks.size() < 10) {
        payloadSizeStr = "0" + payloadSizeStr;
    }
    for (size_t i = 0; i < chunks.size(); i++) {
        std::string chunkNumStr = std::to_string(i + 1);
        if (i + 1 < 10) {
            chunkNumStr = "0" + chunkNumStr;
        }
        std::string payload = "_G['" + addon + "'].f.p('" + chunkNumStr + "', '" + payloadSizeStr + "', '" + addon + "', " + std::to_string(version) + ", " + std::to_string(cache) + ", " + std::to_string(comp) + ", [[" + chunks[i] + "]]);";
        if (comp == 1) {
            payload = "_G['" + addon + "'].f.p('" + chunkNumStr + "', '" + payloadSizeStr + "', '" + addon + "', " + std::to_string(version) + ", " + std::to_string(cache) + ", " + std::to_string(comp) + ", [[" + std::string(compressedBytes.begin(), compressedBytes.end()) + "]]);";
        }

        WorldPacket payloadPacket = CreateAddonPacket("ws", payload, CHAT_MSG_WHISPER, player);
        player->SendDirectMessage(&payloadPacket);
    }
}
/*
void SendPayloadInform(Player* player)
{
    // If the player has any payloads already cached, then they will be loaded immediately
    // Otherwise, full payloads will be requested
    for (const auto& v : WardenLoader::order) {
        const auto& t = WardenLoader::data[v];
        // register all saved variables if there are any
        for (const auto& var : t.savedVars) {
            player->SendAddonMessage("ws", "_G['" + cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
        }

        player->SendAddonMessage("ws", "_G['" + cGTN + "'].f.i('" + v + "', " + t.version + ", " + t.cached + ", " + t.compressed + ", " + #WardenLoader.order + ")", CHAT_MSG_WHISPER, player);
    }
}
*/

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
    //SendPayloadInform(player);
}

void WardenInjectMgr::PushInitModule(Player* player)
{
    SendAddonInjector(player);
    SendAddonMessage("ws", "SendAddonMessage('wc', 'loaded', 'WHISPER', UnitName('player')); print('[WardenLoader]: Injection successful.')", CHAT_MSG_WHISPER, player);
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
        payloadMgr->RegisterPayload(strOne, 9601);
    }

    if (!payloadMgr->GetPayloadById(9602))
    {
        payloadMgr->RegisterPayload(strTwo, 9602);
    }

    payloadMgr->QueuePayload(9601);
    payloadMgr->QueuePayload(9602);

    ChatHandler(player->GetSession()).PSendSysMessage("[WardenLoader]: Awaiting injection...");
}
