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
#include "Config.h"
#include "Chat.h"
#include "Warden.h"
#include "WardenInjectMgr.h"

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
        /*
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

        uint32 payloadId = payloadMgr->RegisterPayload(testPayload);
        payloadMgr->QueuePayload(payloadId);
         */

        sWardenInjectMgr->InitialInjection(player);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg) override
    {
        if (lang != LANG_ADDON || type != CHAT_MSG_WHISPER)
        {
            return;
        }
/*
        uint8 myMsgType;
        uint32 myLang;
        uint64 mySenderGUID;
        uint32 myFlags;
        uint64 myReceiverGUID;
        uint32 myMsgLen;
        std::string myFullMsg;

        data >> myMsgType;
        data >> myLang;
        data >> mySenderGUID;
        data >> myFlags;
        data >> myReceiverGUID;
        data >> myMsgLen;
        data >> myFullMsg;

        std::string header = myFullMsg.substr(0, myFullMsg.find("\t"));

        LOG_INFO("module", "Received addon myMsgType: {}", myMsgType);
        LOG_INFO("module", "Received addon myLang: {}", myLang);
        LOG_INFO("module", "Received addon mySenderGUID: {}", mySenderGUID);
        LOG_INFO("module", "Received addon myFlags: {}", myFlags);
        LOG_INFO("module", "Received addon myReceiverGUID: {}", myReceiverGUID);
        LOG_INFO("module", "Received addon myMsgLen: {}", myMsgLen);
        LOG_INFO("module", "Received addon myFullMsg: {}", myFullMsg);

        Player* sender = ObjectAccessor::FindPlayer(mySenderGUID);
        Player* receiver = ObjectAccessor::FindPlayer(myReceiverGUID);
*/
        LOG_INFO("module", "Received addon msg: {}", msg);
        //sWardenInjectMgr->OnAddonMessageReceived(sender, CHAT_MSG_WHISPER, header, msg, receiver);
    }
};

void AddWardenInjectScripts()
{
    new WardenInjectPlayer();
}