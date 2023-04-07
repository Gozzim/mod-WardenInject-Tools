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
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\r'), luaCode.end());
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\n'), luaCode.end());
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\t'), luaCode.end());

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
