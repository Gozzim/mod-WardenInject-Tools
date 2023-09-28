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

#include "WardenInjectConfigMgr.h"

WardenInjectConfigMgr* WardenInjectConfigMgr::instance()
{
    static WardenInjectConfigMgr instance;
    return &instance;
}

void WardenInjectConfigMgr::LoadConfig(/*bool reload*/)
{
    WardenInjectEnabled = sConfigMgr->GetOption<bool>("WardenInject.Enable", true);
    Announce = sConfigMgr->GetOption<bool>("WardenInject.Announce", true);
    cGTN = sConfigMgr->GetOption<std::string>("WardenInject.ClientCacheTable", "wi");
    OnLoginInject = sConfigMgr->GetOption<bool>("WardenInject.OnLogin", true);
    ScriptsPath = std::filesystem::path(sConfigMgr->GetOption<std::string>("WardenInject.ScriptsPath", defaultScriptsPath));

    if (ScriptsPath.empty())
    {
        ScriptsPath = defaultScriptsPath;
    }

    if (!std::filesystem::exists(ScriptsPath))
    {
        LOG_ERROR("module", "WardenInject: ScriptsPath '{}' does not exist! Falling back to default.", ScriptsPath.string());
        ScriptsPath = defaultScriptsPath;
    }

    if (!std::filesystem::is_directory(ScriptsPath))
    {
        LOG_ERROR("module", "WardenInject: ScriptsPath '{}' is not a directory! Falling back to default.", ScriptsPath.string());
        ScriptsPath = defaultScriptsPath;
    }

    LoadOnLoginScripts();
}

void WardenInjectConfigMgr::LoadOnLoginScripts()
{
    //bool changed = false;

    std::vector<std::string> scripts = sConfigMgr->GetKeysByString("WardenInject.Scripts.");
    for (std::string const& script : scripts)
    {
        std::string configName = script.substr(strlen("WardenInject.Scripts."));
        std::string scriptName = configName.substr(0, configName.find("."));
        std::string scriptConfig = configName.substr(configName.find(".") + 1);

        if (!WardenScriptLoaded(scriptName))
        {
            wardenScriptsMap[scriptName] = WardenInjectData();
        }

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
        else if (scriptConfig == "Order")
        {
            wardenScriptsMap[scriptName].SetOrderNum(sConfigMgr->GetOption<uint16>(script, 0));
        }
        else if (scriptConfig == "File")
        {
            std::filesystem::path filePath(sConfigMgr->GetOption<std::string>(script, ""));

            std::string payload = sWardenInjectFileMgr->GetPayloadFromFile(filePath);
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

WardenInjectData& WardenInjectConfigMgr::GetWardenScript(std::string scriptName)
{
    return wardenScriptsMap[scriptName];
}

bool WardenInjectConfigMgr::WardenScriptLoaded(std::string scriptName)
{
    return wardenScriptsMap.find(scriptName) != wardenScriptsMap.end();
}
