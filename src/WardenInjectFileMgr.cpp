/*
 * This file was written for the AzerothCore Project.
 * Code and Implementation: Gozzim (https://github.com/Gozzim/mod-WardenInject-Tools)
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

#include "WardenInjectFileMgr.h"

WardenInjectFileMgr* WardenInjectFileMgr::instance()
{
    static WardenInjectFileMgr instance;
    return &instance;
}

// Loads the contents of a Lua file into a string variable
std::string WardenInjectFileMgr::LoadLuaFile(const std::filesystem::path& filePath)
{
    if (filePath.extension() != ".lua")
    {
        LOG_INFO("module", "WardenInject::LoadLuaFile - File '{}' does not have 'lua' extension. Loading anyway.", filePath.string());
    }

    std::string luaCode;

    // ToDo: Catch exceptions due to different reasons - file not found, permissions error, etc.
    std::ifstream file(filePath);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        luaCode = buffer.str();
    }
    else
    {
        LOG_ERROR("module", "WardenInjectMgr::LoadLuaFile - Loading file '{}' failed.", filePath.string());
    }

    file.close();

    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::LoadLuaFile - File '{}' is empty.", filePath.string());
    }

    return luaCode;
}

// Converts the Lua code into a payload that can be sent to the client
// This is done by removing comments, line breaks and whitespaces
// Also replaces " with ' to avoid issues with the client
// It is not perfect but does the job
void WardenInjectFileMgr::ConvertToPayload(std::string& luaCode)
{
    // Replace " with '
    std::replace(luaCode.begin(), luaCode.end(), '"', '\'');

    // Remove comment blocks
    std::regex commentBlockRegex("--\\[=?\\[[\\s\\S]*?\\]=?\\]");
    luaCode = std::regex_replace(luaCode, commentBlockRegex, "");

    // Remove comments
    std::regex commentRegex("--.*\r?\n");
    luaCode = std::regex_replace(luaCode, commentRegex, "\n");

    // Remove leading tabs and whitespaces
    std::regex leadingSpaceRegex("\r?\n[ \t]+", std::regex_constants::optimize);
    luaCode = std::regex_replace(luaCode, leadingSpaceRegex, "\n");

    // Remove empty lines
    std::regex emptyLineRegex("\r?\n[ \t]*\r?\n", std::regex_constants::optimize);
    luaCode = std::regex_replace(luaCode, emptyLineRegex, "\n");

    // Replace line breaks with spaces
    std::regex lineBreakRegex("\r?\n");
    luaCode = std::regex_replace(luaCode, lineBreakRegex, " ");

    // Trim whitespace from the beginning and end of the string
    Acore::String::Trim(luaCode);
}

std::string WardenInjectFileMgr::GetPayloadFromFile(std::filesystem::path& filePath)
{
    if (!filePath.is_absolute())
    {
        filePath = sWardenInjectConfigMgr->ScriptsPath / filePath;
    }

    if (!std::filesystem::exists(filePath))
    {
        LOG_ERROR("module", "WardenInject::GetPayloadFromFile - File '{}' does not exist.", filePath.string());
        return "";
    }

    if (!std::filesystem::is_regular_file(filePath))
    {
        LOG_ERROR("module", "WardenInject::GetPayloadFromFile - File '{}' is not a file.", filePath.string());
        return "";
    }

    std::string luaCode = LoadLuaFile(filePath);
    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::GetPayloadFromFile - Code from file {} is empty.", filePath.string());
        return luaCode;
    }

    ConvertToPayload(luaCode);
    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::GetPayloadFromFile - Code from file {} is empty after removing whitespaces.", filePath.string());
    }

    return luaCode;
}
