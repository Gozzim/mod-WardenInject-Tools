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

std::vector<std::string> WardenInjectMgr::GetChunks(std::string s, uint8_t chunkSize)
{
    std::vector<std::string> chunks;

    for (uint32_t i = 0; i < s.size(); i += chunkSize)
    {
        chunks.push_back(s.substr(i, chunkSize));
    }

    return chunks;
}

bool WardenInjectMgr::TryReadFile(std::string& path, std::string& bn_Result)
{
    std::ifstream bn_File(path);

    std::string bn_Buffer = "";
    bn_Result = "";

    if (!bn_File.is_open()) {
        return false;
    }

    while (std::getline(bn_File, bn_Buffer)) {
        bn_Result = bn_Result + (bn_Buffer);
    }

    bn_Result.erase(std::remove(bn_Result.begin(), bn_Result.end(), '\r'), bn_Result.cend());
    bn_Result.erase(std::remove(bn_Result.begin(), bn_Result.end(), '\n'), bn_Result.cend());

    return true;
}

void WardenInjectMgr::SendWardenInject(Player* player, std::string payload)
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

   auto chunks = GetChunks(payload, 200);


    uint32 payloadId = payloadMgr->RegisterPayload(welcomePayload);
    payloadMgr->QueuePayload(payloadId);
}

void SendChunkedPayload(Warden* warden, WardenPayloadMgr* payloadMgr, std::string payload, uint32 chunkSize)
{
    bool verbose = sConfigMgr->GetOption<bool>("BreakingNews.Verbose", false);

    auto chunks = GetChunks(payload, chunkSize);

    if (!payloadMgr->GetPayloadById(_prePayloadId))
    {
        payloadMgr->RegisterPayload(_prePayload, _prePayloadId);
    }

    payloadMgr->QueuePayload(_prePayloadId);
    warden->ForceChecks();

    if (verbose)
    {
        LOG_INFO("module", "Sent pre-payload '{}'.", _prePayload);
    }

    for (auto const& chunk : chunks)
    {
        auto smallPayload = "wlbuf = wlbuf .. [[" + chunk + "]];";

        payloadMgr->RegisterPayload(smallPayload, _tmpPayloadId, true);
        payloadMgr->QueuePayload(_tmpPayloadId);
        warden->ForceChecks();

        if (verbose)
        {
            LOG_INFO("module", "Sent mid-payload '{}'.", smallPayload);
        }
    }

    if (!payloadMgr->GetPayloadById(_postPayloadId))
    {
        payloadMgr->RegisterPayload(_postPayload, _postPayloadId);
    }

    payloadMgr->QueuePayload(_postPayloadId);
    warden->ForceChecks();

    if (verbose)
    {
        LOG_INFO("module", "Sent post-payload '{}'.", _postPayload);
    }
}