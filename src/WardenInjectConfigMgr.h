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

#ifndef _WARDENINJECTCONFIGMGR_H_
#define _WARDENINJECTCONFIGMGR_H_

#include "BuiltInConfig.h"
#include "Common.h"
#include "Config.h"
#include "Log.h"
#include "WardenInjectData.h"

#include <filesystem>
#include <map>

class WardenInjectData;

class WardenInjectConfigMgr
{
public:
    static WardenInjectConfigMgr* instance();

    const std::filesystem::path defaultScriptsPath = std::filesystem::path(BuiltInConfig::GetSourceDirectory()) / std::filesystem::path("modules/mod-WardenInject-Tools/payloads");

    bool WardenInjectEnabled;
    bool Announce;
    std::filesystem::path ScriptsPath;
    bool OnLoginInject;
    std::string cGTN;

    void LoadConfig(/*bool reload*/);
    void LoadOnLoginScripts();

    typedef std::map<std::string, WardenInjectData> WardenScriptsMap;
    WardenScriptsMap GetWardenScriptsMap() { return wardenScriptsMap; }
    WardenInjectData& GetWardenScript(std::string scriptName);
    bool WardenScriptLoaded(std::string scriptName);

private:
    WardenScriptsMap wardenScriptsMap;
};

#define sWardenInjectConfigMgr WardenInjectConfigMgr::instance()

#endif