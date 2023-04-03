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

#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "Warden.h"

std::string welcomePayload = "message('Welcome to the server!');";

class WardenInjectPlayer : public PlayerScript
{
public:
    WardenInjectPlayer() : PlayerScript("WardenInjectPlayer") { }

    void RunTests(WardenPayloadMgr* mgr)
    {
        mgr->ClearQueuedPayloads();

        //Should fail
        ASSERT(!mgr->RegisterPayload("", 4000, false));
        LOG_INFO("module", "Warden unit test #1 passed.");
        //Should pass
        ASSERT(mgr->RegisterPayload("", 9000, false));
        LOG_INFO("module", "Warden unit test #2 passed.");
        //Should fail
        ASSERT(!mgr->RegisterPayload("", 9000, false));
        LOG_INFO("module", "Warden unit test #3 passed.");
        //Should pass
        ASSERT(mgr->RegisterPayload("", 9000, true));
        LOG_INFO("module", "Warden unit test #4 passed.");
        //Should pass
        ASSERT(mgr->UnregisterPayload(9000));
        LOG_INFO("module", "Warden unit test #5 passed.");

        mgr->ClearQueuedPayloads();
    }

    void OnLogin(Player* player) override
    {
        Warden* warden = player->GetSession()->GetWarden();
        if (!warden)
        {
            return;
        }

        auto payloadMgr = warden->GetPayloadMgr();
        if (!payloadMgr)
        {
            return;
        }

        RunTests(payloadMgr);

        uint32 payloadId = payloadMgr->RegisterPayload(welcomePayload);
        payloadMgr->QueuePayload(payloadId);
    }

    void OnBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg)
    {
        auto warden = player->GetSession()->GetWarden();
        if (!warden)
        {
            return;
        }

        auto payloadMgr = warden->GetPayloadMgr();
        if (!payloadMgr)
        {
            return;
        }

        std::string payload = Acore::StringFormatFmt("print('{}');", msg);
        uint32 payloadId = payloadMgr->RegisterPayload(payload);

        for (uint32 i = 0; i < payloadMgr->GetPayloadCountInQueue(); i++)
        {
            warden->ForceChecks();
        }

        payloadMgr->QueuePayload(payloadId);
        warden->ForceChecks();
    }
};

void AddWardenInjectScripts()
{
    new WardenInjectPlayer();
}