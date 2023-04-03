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

#ifndef _WardenInjectMgr_H_
#define _WardenInjectMgr_H_

#include "Common.h"
#include "Chat.h"
#include "Player.h"
#include "SocialMgr.h"

class WardenInjectMgr
{
public:
    static WardenInjectMgr* instance();

    std::vector<std::string> GetChunks(std::string s, uint8_t chunkSize);
    bool TryReadFile(std::string& path, std::string& bn_Result);
    void SendWardenInject(Player* player, std::string payload);

private:

};

#define sWardenInjectMgr WardenInjectMgr::instance()

#endif