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

#include "GameTime.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "WardenInjectMgr.h"

using namespace Acore::ChatCommands;

class wardeninject_commandscript : public CommandScript
{
public:
    wardeninject_commandscript() : CommandScript("wardeninject_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable cfbgCommands =
                {
                        { "code",   HandleInjectCommand,        SEC_ADMINISTRATOR, Console::No },
                        { "load",   HandleLoadLuaScriptCommand, SEC_ADMINISTRATOR, Console::No },
                        { "file",   HandleFileInjectCommand,    SEC_ADMINISTRATOR, Console::No },
                        { "init",   HandlePushInitCommand,      SEC_ADMINISTRATOR, Console::No}
                };

        static ChatCommandTable commandTable =
                {
                        { "inject",  injectCommands },
                };

        return commandTable;
    }

    static bool HandleInjectCommand(ChatHandler* handler, Tail message)
    {
        if (message.empty())
            return false;

        Player* player = handler->GetPlayer();

        std::string payload = std::string(message);
        if (message == "test")
        {
            payload = sWardenInjectMgr->testPayload;
        }
        LOG_INFO("module", "Sending payload '{}'.", payload);
        //sWardenInjectMgr->SendWardenInject(player, payload);
        sWardenInjectMgr->SendAddonMessage("ws", payload, CHAT_MSG_WHISPER, player);

        return true;
    }

    static bool HandlePushInitCommand(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        sWardenInjectMgr->PushInitModule(player);

        return true;
    }

    static bool HandleFileInjectCommand(ChatHandler* handler, Tail message)
    {
        if (message.empty())
            return false;

        Player* player = handler->GetPlayer();

        std::string payload = sWardenInjectMgr->GetPayloadFromFile(std::string(message));
        sWardenInjectMgr->ConvertToPayload(payload);
        LOG_INFO("module", "Sending payload '{}'.", payload);
        sWardenInjectMgr->SendWardenInject(player, payload);

        return true;
    }

    static bool HandleLoadLuaScriptCommand(ChatHandler* handler, Tail path)
    {
        std::string filePath = std::string(path);
        std::string payload = sWardenInjectMgr->GetPayloadFromFile(filePath);
        sWardenInjectMgr->ConvertToPayload(payload);
        LOG_INFO("module", "Loaded payload '{}'.", payload);

        return true;
    }
};

void AddSC_wardeninject_commandscript()
{
    new wardeninject_commandscript();
}