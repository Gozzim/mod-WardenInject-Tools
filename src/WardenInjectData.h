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

#ifndef _WARDENINJECTDATA_H_
#define _WARDENINJECTDATA_H_

#include "WardenInjectMgr.h"

class WardenInjectData
{
public:
    WardenInjectData();
    ~WardenInjectData();

    float GetVersion();
    void SetVersion(float _version);

    bool IsCompressed();
    void SetCompressed(bool _compressed);

    bool IsCached();
    void SetCached(bool _cached);

    bool IsOnLogin();
    void SetOnLogin(bool _onLogin);

    uint16 GetOrderNum();
    void SetOrderNum(uint16 _orderNum);

    std::string GetPayload();
    void SetPayload(std::string _payload);

    std::vector<std::string> GetSavedVars();
    void SetSavedVars(std::vector<std::string> _savedVars);
    void AddSavedVar(std::string _savedVar);
    void RemoveSavedVar(std::string _savedVar);
    void ClearSavedVars();

private:
    float version;
    bool compressed;
    bool cached;
    bool onLogin;
    uint16 orderNum;
    std::string payload;
    std::vector<std::string> savedVars;
};

#endif
