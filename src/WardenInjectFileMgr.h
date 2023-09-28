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

#ifndef _WARDENINJECT_FILEMGR_H_
#define _WARDENINJECT_FILEMGR_H_

#include "Common.h"
#include "Log.h"
#include "WardenInjectConfigMgr.h"

#include <filesystem>
#include <fstream>
#include <regex>

class WardenInjectFileMgr
{
public:
    static WardenInjectFileMgr* instance();

    std::string LoadLuaFile(const std::filesystem::path& filePath);
    void ConvertToPayload(std::string& luaCode);
    std::string GetPayloadFromFile(std::filesystem::path& filePath);

};

#define sWardenInjectFileMgr WardenInjectFileMgr::instance()

#endif