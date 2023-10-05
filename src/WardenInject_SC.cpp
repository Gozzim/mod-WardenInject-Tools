/*
 * This file was written for the AzerothCore Project.
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

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "Warden.h"
#include "WardenInjectMgr.h"

class WardenInject_Config : public WorldScript
{
public: WardenInject_Config() : WorldScript("WardenInject_Config") { };

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sWardenInjectConfigMgr->LoadConfig(/*reload*/);
    }
};

class WardenInjectPlayer : public PlayerScript
{
public:
    WardenInjectPlayer() : PlayerScript("WardenInjectPlayer") { }

    void OnLogin(Player* player) override
    {
        if (sWardenInjectConfigMgr->OnLoginInject)
        {
            sWardenInjectMgr->InitialInjection(player);
        }
    }

    void OnBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg) override
    {
        if (lang != LANG_ADDON && type != CHAT_MSG_WHISPER)
        {
            return;
        }

        const std::string prefix = "wc";
        const std::string separator = "\t";

        if (msg.rfind(prefix + separator, 0) != 0)
        {
            return;
        }

        std::string payload = msg.substr((prefix + separator).length());

        sWardenInjectMgr->OnAddonMessageReceived(player, type, prefix, payload);
    }
};

void AddWardenInjectScripts()
{
    new WardenInjectPlayer();
    new WardenInject_Config();
}
