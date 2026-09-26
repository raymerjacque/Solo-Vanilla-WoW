#include "Config/Config.h"
#include "../botpch.h"
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "PlayerbotAI.h"
#include "Player.h"
#include "AiFactory.h"
#include "GuildMgr.h"
#include "Guild.h"
#include "Group.h"
#include "World.h"
#include <tuple>
#include <utility>

INSTANTIATE_SINGLETON_1(RandomPlayerbotMgr);

RandomPlayerbotMgr::RandomPlayerbotMgr() : PlayerbotHolder(), processTicks(0)
{
}

RandomPlayerbotMgr::~RandomPlayerbotMgr()
{
}

void RandomPlayerbotMgr::UpdateAIInternal(uint32 elapsed)
{
    SetNextCheckDelay(sPlayerbotAIConfig.randomBotUpdateInterval * 1000);

    if (!sPlayerbotAIConfig.randomBotAutologin || !sPlayerbotAIConfig.enabled)
    {
        return;
    }

    sLog.outBasic("Processing random bots...");

    uint32 humanCount = players.size();
    int maxAllowedBotCount = humanCount > 0 ? std::min(300, 40 + (int)humanCount * 10) : 0;

    // Prune excess bots if player count decreased or 0, skipping bots in active groups
    list<uint32> bots = GetBots();
    for (list<uint32>::iterator it = bots.begin(); it != bots.end() && bots.size() > (size_t)maxAllowedBotCount; )
    {
        uint32 botToLogout = *it;
        Player* botPlayer = GetPlayerBot(botToLogout);
        if (botPlayer && botPlayer->GetGroup())
        {
            ++it;
            continue;
        }

        LogoutPlayerBot(botToLogout);
        it = bots.erase(it);
    }

    int botCount = bots.size();
    int allianceNewBots = 0, hordeNewBots = 0;
    int randomBotsPerInterval = 10;

    while (botCount++ < maxAllowedBotCount)
    {
        bool alliance = botCount % 2;
        uint32 bot = AddRandomBot(alliance);
        if (bot)
        {
            if (alliance)
            {
                allianceNewBots++;
            }
            else
            {
                hordeNewBots++;
            }

            bots.push_back(bot);
        }
        else
        {
            break;
        }
    }

    int botProcessed = 0;
    for (list<uint32>::iterator i = bots.begin(); i != bots.end(); ++i)
    {
        uint32 bot = *i;
        if (ProcessBot(bot))
        {
            botProcessed++;
        }

        if (botProcessed >= randomBotsPerInterval)
        {
            break;
        }
    }

    if ((int)playerBots.size() < maxAllowedBotCount)
    {
        SetNextCheckDelay(1000);
    }
    else
    {
        SetNextCheckDelay(sPlayerbotAIConfig.randomBotUpdateInterval * 1000);
    }

    sLog.outString("%d bots processed. %d alliance and %d horde bots added. %d bots online (%u human players). Next check in %d seconds",
            botProcessed, allianceNewBots, hordeNewBots, playerBots.size(), humanCount, (int)playerBots.size() < maxAllowedBotCount ? 1 : sPlayerbotAIConfig.randomBotUpdateInterval);

    if (processTicks++ == 1)
    {
        PrintStats();
    }

    TeleportBotsToPlayers();

    if ((int)playerBots.size() >= maxAllowedBotCount)
    {
        FormRandomGroups();
        RelocateIdleBots();
        ProcessGuilds();
        FormRaidGroups();
        FormCityInvasionRaids();
    }
}



uint32 RandomPlayerbotMgr::AddRandomBot(bool alliance)
{
    vector<uint32> bots = GetFreeBots(alliance);
    if (bots.size() == 0)
    {
        return 0;
    }

    int index = urand(0, bots.size() - 1);
    uint32 bot = bots[index];
    SetEventValue(bot, "add", 1, urand(sPlayerbotAIConfig.minRandomBotInWorldTime, sPlayerbotAIConfig.maxRandomBotInWorldTime));
    uint32 randomTime = 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
    ScheduleRandomize(bot, randomTime);
    sLog.outDetail("Random bot %d added", bot);
    return bot;
}

void RandomPlayerbotMgr::ScheduleRandomize(uint32 bot, uint32 time)
{
    SetEventValue(bot, "randomize", 1, time);
    SetEventValue(bot, "logout", 1, time + 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
}

void RandomPlayerbotMgr::ScheduleTeleport(uint32 bot)
{
    SetEventValue(bot, "teleport", 1, 60 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
}

bool RandomPlayerbotMgr::ProcessBot(uint32 bot)
{
    uint32 isValid = GetEventValue(bot, "add");
    if (!isValid)
    {
        Player* player = GetPlayerBot(bot);
        if (!player || !player->GetGroup())
        {
            sLog.outDetail("Bot %d expired", bot);
            SetEventValue(bot, "add", 0, 0);
        }
        return true;
    }

    if (!GetPlayerBot(bot))
    {
        sLog.outDetail("Bot %d logged in", bot);
        AddPlayerBot(bot, 0);
        if (!GetEventValue(bot, "online"))
        {
            SetEventValue(bot, "online", 1, sPlayerbotAIConfig.minRandomBotInWorldTime);
        }
        return true;
    }

    Player* player = GetPlayerBot(bot);
    if (!player)
    {
        return false;
    }

    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai)
    {
        return false;
    }

    if (player->GetGroup())
    {
        sLog.outDetail("Skipping bot %d as it is in group", bot);
        return false;
    }

    if (player->IsDead())
    {
        if (!GetEventValue(bot, "dead"))
        {
            sLog.outDetail("Setting dead flag for bot %d", bot);
            uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotReviveTime, sPlayerbotAIConfig.maxRandomBotReviveTime);
            SetEventValue(bot, "dead", 1, randomTime);
            SetEventValue(bot, "revive", 1, randomTime - 60);
            return false;
        }

        if (!GetEventValue(bot, "revive"))
        {
            sLog.outDetail("Reviving dead bot %d", bot);
            SetEventValue(bot, "dead", 0, 0);
            SetEventValue(bot, "revive", 0, 0);
            RandomTeleport(player, player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
            return true;
        }

        return false;
    }

    uint32 randomize = GetEventValue(bot, "randomize");
    if (!randomize)
    {
        sLog.outDetail("Randomizing bot %d", bot);
        Randomize(player);
        uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
        ScheduleRandomize(bot, randomTime);
        return true;
    }

    uint32 logout = GetEventValue(bot, "logout");
    if (!logout)
    {
        sLog.outDetail("Logging out bot %d", bot);
        LogoutPlayerBot(bot);
        SetEventValue(bot, "logout", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    uint32 teleport = GetEventValue(bot, "teleport");
    if (!teleport)
    {
        sLog.outDetail("Random teleporting bot %d", bot);
        RandomTeleportForLevel(ai->GetBot());
        SetEventValue(bot, "teleport", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::RandomTeleport(Player* bot, vector<WorldLocation> &locs)
{
    if (bot->IsBeingTeleported())
    {
        return;
    }

    if (locs.empty())
    {
        sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
        return;
    }

    for (int attemtps = 0; attemtps < 10; ++attemtps)
    {
        int index = urand(0, locs.size() - 1);
        WorldLocation loc = locs[index];
        float x = loc.coord_x + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float y = loc.coord_y + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float z = loc.coord_z;

        Map* map = sMapMgr.FindMap(loc.mapid);
        if (!map)
        {
            continue;
        }

        const TerrainInfo * terrain = map->GetTerrain();
        if (!terrain)
        {
            continue;
        }

        AreaTableEntry const* area = sAreaStore.LookupEntry(terrain->GetAreaId(x, y, z));
        if (!area)
        {
            continue;
        }

        if (!terrain->IsOutdoors(x, y, z) ||
                terrain->IsUnderWater(x, y, z) ||
                terrain->IsInWater(x, y, z))
            continue;

        sLog.outDetail("Random teleporting bot %s to %s %f,%f,%f", bot->GetName(), area->area_name[0], x, y, z);
        float height = map->GetTerrain()->GetHeightStatic(x, y, 0.5f + z, true, MAX_HEIGHT);
        if (height <= INVALID_HEIGHT)
        {
            continue;
        }

        z = 0.05f + map->GetTerrain()->GetHeightStatic(x, y, 0.05f + z, true, MAX_HEIGHT);

        bot->GetMotionMaster()->Clear();
        bot->TeleportTo(loc.mapid, x, y, z, 0);
        return;
    }

    sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
}

void RandomPlayerbotMgr::RandomTeleportForLevel(Player* bot)
{
    static map<pair<uint32, uint32>, vector<WorldLocation>> levelLocsCache;
    pair<uint32, uint32> cacheKey(bot->getLevel(), sPlayerbotAIConfig.randomBotTeleLevel);

    if (levelLocsCache.find(cacheKey) == levelLocsCache.end() || levelLocsCache[cacheKey].empty())
    {
        vector<WorldLocation> locs;
        QueryResult* results = WorldDatabase.PQuery("SELECT `map`, `position_x`, `position_y`, `position_z` FROM ("
            "SELECT MIN(`c`.`map`) `map`, MIN(`c`.`position_x`) `position_x`, MIN(`c`.`position_y`) `position_y`, "
            "MIN(`c`.`position_z`) `position_z`, AVG(`t`.`maxlevel`), AVG(`t`.`minlevel`), "
            "%u - (AVG(`t`.`maxlevel`) + AVG(`t`.`minlevel`)) / 2 `delta` FROM `creature` `c` "
            "INNER JOIN `creature_template` `t` ON `c`.`id` = `t`.`entry` GROUP BY `t`.`entry`) `q` "
            "WHERE `delta` >= 0 AND `delta` <= %u AND `map` IN (%s)",
            bot->getLevel(), sPlayerbotAIConfig.randomBotTeleLevel, sPlayerbotAIConfig.randomBotMapsAsString.c_str());
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                uint32 mapId = fields[0].GetUInt32();
                float x = fields[1].GetFloat();
                float y = fields[2].GetFloat();
                float z = fields[3].GetFloat();
                WorldLocation loc(mapId, x, y, z, 0);
                locs.push_back(loc);
            } while (results->NextRow());
            delete results;
        }
        levelLocsCache[cacheKey] = locs;
    }

    if (!levelLocsCache[cacheKey].empty())
    {
        RandomTeleport(bot, levelLocsCache[cacheKey]);
    }
}

void RandomPlayerbotMgr::RandomTeleport(Player* bot, uint32 mapId, float teleX, float teleY, float teleZ)
{
    static map<tuple<uint32, int, int>, vector<WorldLocation>> areaLocsCache;
    int gridX = (int)(teleX / 250.0f);
    int gridY = (int)(teleY / 250.0f);
    auto cacheKey = make_tuple(mapId, gridX, gridY);

    if (areaLocsCache.find(cacheKey) == areaLocsCache.end() || areaLocsCache[cacheKey].empty())
    {
        vector<WorldLocation> locs;
        QueryResult* results = WorldDatabase.PQuery("SELECT `position_x`, `position_y`, `position_z` FROM `creature` WHERE `map` = '%u' AND ABS(`position_x` - '%f') < '%u' AND ABS(`position_y` - '%f') < '%u'",
                mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                float x = fields[0].GetFloat();
                float y = fields[1].GetFloat();
                float z = fields[2].GetFloat();
                WorldLocation loc(mapId, x, y, z, 0);
                locs.push_back(loc);
            } while (results->NextRow());
            delete results;
        }
        areaLocsCache[cacheKey] = locs;
    }

    if (!areaLocsCache[cacheKey].empty())
    {
        RandomTeleport(bot, areaLocsCache[cacheKey]);
    }
    Refresh(bot);
}

void RandomPlayerbotMgr::Randomize(Player* bot)
{
    if (bot->getLevel() == 1)
    {
        RandomizeFirst(bot);
    }
    else
    {
        IncreaseLevel(bot);
    }
}

void RandomPlayerbotMgr::IncreaseLevel(Player* bot)
{
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    uint32 level = min(bot->getLevel() + 1, maxLevel);
    PlayerbotFactory factory(bot, level);
    if (bot->GetGuildId())
    {
        factory.Refresh();
    }
    else
    {
        factory.Randomize();
    }
    RandomTeleportForLevel(bot);
}

void RandomPlayerbotMgr::RandomizeFirst(Player* bot)
{
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        int index = urand(0, sPlayerbotAIConfig.randomBotMaps.size() - 1);
        uint32 mapId = sPlayerbotAIConfig.randomBotMaps[index];

        vector<GameTele const*> locs;
        GameTeleMap const & teleMap = sObjectMgr.GetGameTeleMap();
        for(GameTeleMap::const_iterator itr = teleMap.begin(); itr != teleMap.end(); ++itr)
        {
            GameTele const* tele = &itr->second;
            if (tele->mapId == mapId)
            {
                locs.push_back(tele);
            }
        }

        index = urand(0, locs.size() - 1);
        if (index >= locs.size())
        {
            return;
        }
        uint32 level = urand(sPlayerbotAIConfig.randomBotMinLevel, maxLevel);
        if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomBotMaxLevelChance)
        {
            level = maxLevel;
        }


        if (level < sPlayerbotAIConfig.randomBotMinLevel)
        {
            continue;
        }

        PlayerbotFactory factory(bot, level);
        factory.CleanRandomize();
        RandomTeleportForLevel(bot);
        break;

    }
}

uint32 RandomPlayerbotMgr::GetZoneLevel(uint32 mapId, float teleX, float teleY, float teleZ)
{
    static map<tuple<uint32, int, int>, uint32> zoneLevelCache;
    int gridX = (int)(teleX / 250.0f);
    int gridY = (int)(teleY / 250.0f);
    auto key = make_tuple(mapId, gridX, gridY);
    auto it = zoneLevelCache.find(key);
    if (it != zoneLevelCache.end())
    {
        return it->second;
    }

    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);

    uint32 level;
    QueryResult *results = WorldDatabase.PQuery("SELECT AVG(`t`.`minlevel`) `minlevel`, AVG(`t`.`maxlevel`) `maxlevel` FROM `creature` `c` "
            "INNER JOIN `creature_template` `t` ON `c`.`id` = `t`.`entry` "
            "WHERE `map` = '%u' AND `minlevel` > 1 AND ABS(`position_x` - '%f') < '%u' AND ABS(`position_y` - '%f') < '%u'",
            mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);

    if (results)
    {
        Field* fields = results->Fetch();
        uint32 minLevel = fields[0].GetUInt32();
        uint32 maxLevel = fields[1].GetUInt32();
        level = urand(minLevel, maxLevel);
        if (level > maxLevel)
        {
            level = maxLevel;
        }
        delete results;
    }
    else
    {
        level = urand(1, maxLevel);
    }

    zoneLevelCache[key] = level;
    return level;
}

void RandomPlayerbotMgr::Refresh(Player* bot)
{
    if (bot->IsDead())
    {
        PlayerbotChatHandler ch(bot);
        ch.revive(*bot);
        bot->GetPlayerbotAI()->ResetStrategies();
    }

    bot->GetPlayerbotAI()->Reset();

    HostileReference *ref = bot->GetHostileRefManager().getFirst();
    while( ref )
    {
        ThreatManager *threatManager = ref->getSource();
        Unit *unit = threatManager->getOwner();
        float threat = ref->getThreat();

        unit->RemoveAllAttackers();
        unit->ClearInCombat();

        ref = ref->next();
    }

    bot->RemoveAllAttackers();
    bot->ClearInCombat();

    bot->DurabilityRepairAll(false, 1.0f);
    bot->SetHealthPercent(100);
    bot->SetPvP(true);

    if (bot->GetMaxPower(POWER_MANA) > 0)
    {
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));
    }

    if (bot->GetMaxPower(POWER_ENERGY) > 0)
    {
        bot->SetPower(POWER_ENERGY, bot->GetMaxPower(POWER_ENERGY));
    }
}


bool RandomPlayerbotMgr::IsRandomBot(Player* bot)
{
    return IsRandomBot(bot->GetObjectGuid());
}

bool RandomPlayerbotMgr::IsRandomBot(uint32 bot)
{
    return GetEventValue(bot, "add");
}

list<uint32> RandomPlayerbotMgr::GetBots()
{
    list<uint32> bots;

    QueryResult* results = CharacterDatabase.Query(
            "SELECT `bot` FROM `ai_playerbot_random_bots` WHERE `owner` = 0 AND `event` = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.push_back(bot);
        } while (results->NextRow());
        delete results;
    }

    return bots;
}

vector<uint32> RandomPlayerbotMgr::GetFreeBots(bool alliance)
{
    set<uint32> bots;

    QueryResult* results = CharacterDatabase.PQuery(
        "SELECT `bot` FROM `ai_playerbot_random_bots` WHERE `event` = 'add'"
    );

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.insert(bot);
        } while (results->NextRow());
        delete results;
    }

    vector<uint32> guids;
    for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); i++)
    {
        uint32 accountId = *i;
        if (!sAccountMgr.GetCharactersCount(accountId))
        {
            continue;
        }

        QueryResult *result = CharacterDatabase.PQuery("SELECT `guid`, `race` FROM `characters` WHERE `account` = '%u'", accountId);
        if (!result)
        {
            continue;
        }

        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint32 race = fields[1].GetUInt32();
            if (bots.find(guid) == bots.end() &&
                    ((alliance && IsAlliance(race)) || ((!alliance && !IsAlliance(race))
            )))
                guids.push_back(guid);
        } while (result->NextRow());
        delete result;
    }


    return guids;
}

uint32 RandomPlayerbotMgr::GetEventValue(uint32 bot, string event)
{
    uint32 value = 0;

    QueryResult* results = CharacterDatabase.PQuery(
            "SELECT `value`, `time`, `validIn` FROM `ai_playerbot_random_bots` WHERE `owner` = 0 AND `bot` = '%u' AND `event` = '%s'",
            bot, event.c_str());

    if (results)
    {
        Field* fields = results->Fetch();
        value = fields[0].GetUInt32();
        uint32 lastChangeTime = fields[1].GetUInt32();
        uint32 validIn = fields[2].GetUInt32();
        if ((time(0) - lastChangeTime) >= validIn)
        {
            value = 0;
        }
        delete results;
    }

    return value;
}

uint32 RandomPlayerbotMgr::SetEventValue(uint32 bot, string event, uint32 value, uint32 validIn)
{
    CharacterDatabase.PExecute("DELETE FROM `ai_playerbot_random_bots` WHERE `owner` = 0 and `bot` = '%u' and `event` = '%s'",
            bot, event.c_str());
    if (value)
    {
        CharacterDatabase.PExecute(
                "INSERT INTO `ai_playerbot_random_bots` (`owner`, `bot`, `time`, `validIn`, `event`, `value`) VALUES ('%u', '%u', '%u', '%u', '%s', '%u')",
                0, bot, (uint32)time(0), validIn, event.c_str(), value);
    }

    return value;
}

bool ChatHandler::HandlePlayerbotConsoleCommand(char* args)
{
    if (!sPlayerbotAIConfig.enabled)
    {
        PSendSysMessage("Playerbot system is currently disabled!");
        SetSentErrorMessage(true);
        return false;
    }

    if (!args || !*args)
    {
        sLog.outError("Usage: rndbot stats/update/reset/init/refresh/add/remove");
        return false;
    }

    string cmd = args;

    if (cmd == "reset")
    {
        CharacterDatabase.PExecute("DELETE FROM `ai_playerbot_random_bots`");
        sLog.outBasic("Random bots were reset for all players");
        return true;
    }
    else if (cmd == "stats")
    {
        sRandomPlayerbotMgr.PrintStats();
        return true;
    }
    else if (cmd == "update")
    {
        sRandomPlayerbotMgr.UpdateAIInternal(0);
        return true;
    }
    else if (cmd == "init" || cmd == "refresh")
    {
        sLog.outString("Randomizing bots for %d accounts", sPlayerbotAIConfig.randomBotAccounts.size());
        BarGoLink bar(sPlayerbotAIConfig.randomBotAccounts.size());
        for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); ++i)
        {
            uint32 account = *i;
            bar.step();
            if (QueryResult *results = CharacterDatabase.PQuery("SELECT `guid` FROM `characters` where `account` = '%u'", account))
            {
                do
                {
                    Field* fields = results->Fetch();
                    ObjectGuid guid = ObjectGuid(fields[0].GetUInt64());
                    Player* bot = sObjectMgr.GetPlayer(guid, true);
                    if (!bot)
                    {
                        continue;
                    }

                    if (cmd == "init")
                    {
                        sLog.outDetail("Randomizing bot %s for account %u", bot->GetName(), account);
                        sRandomPlayerbotMgr.RandomizeFirst(bot);
                    }
                    else
                    {
                        sLog.outDetail("Refreshing bot %s for account %u", bot->GetName(), account);
                        bot->SetLevel(bot->getLevel() - 1);
                        sRandomPlayerbotMgr.IncreaseLevel(bot);
                    }
                    uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
                    CharacterDatabase.PExecute("UPDATE `ai_playerbot_random_bots` SET `validIn` = '%u' WHERE `event` = 'randomize' AND `bot` = '%u'",
                            randomTime, bot->GetGUIDLow());
                    CharacterDatabase.PExecute("UPDATE `ai_playerbot_random_bots` SET `validIn` = '%u' WHERE `event` = 'logout' AND `bot` = '%u'",
                            sPlayerbotAIConfig.maxRandomBotInWorldTime, bot->GetGUIDLow());
                } while (results->NextRow());

                delete results;
            }
        }
        return true;
    }
    else
    {
        list<string> messages = sRandomPlayerbotMgr.HandlePlayerbotCommand(args, NULL);
        for (list<string>::iterator i = messages.begin(); i != messages.end(); ++i)
        {
            sLog.outString(i->c_str());
        }
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::HandleCommand(uint32 type, const string& text, Player& fromPlayer)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        bot->GetPlayerbotAI()->HandleCommand(type, text, fromPlayer);
    }
}

void RandomPlayerbotMgr::OnPlayerLogout(Player* player)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (player == ai->GetMaster())
        {
            ai->SetMaster(NULL);
            ai->ResetStrategies();
        }
    }

    if (!player->GetPlayerbotAI())
    {
        vector<Player*>::iterator i = find(players.begin(), players.end(), player);
        if (i != players.end())
        {
            players.erase(i);
        }
        UpdateAIInternal(0);
    }
}

void RandomPlayerbotMgr::OnPlayerLogin(Player* player)
{
    if (!player || player->GetPlayerbotAI())
        return;

    Group* group = player->GetGroup();
    if (group)
    {
        Group::MemberSlotList const& slots = group->GetMemberSlots();
        for (Group::member_citerator it = slots.begin(); it != slots.end(); ++it)
        {
            uint32 botGuid = it->guid.GetCounter();
            if (IsRandomBot(botGuid))
            {
                if (!GetPlayerBot(botGuid))
                {
                    sLog.outString("OnPlayerLogin: Auto-logging in party bot %u for human player %s", botGuid, player->GetName());
                    AddPlayerBot(botGuid, 0);
                }

                Player* bot = GetPlayerBot(botGuid);
                if (bot && bot->GetPlayerbotAI())
                {
                    PlayerbotAI* ai = bot->GetPlayerbotAI();
                    ai->SetMaster(player);
                    ai->ResetStrategies();
                    ai->TellMaster("Hello");

                    if (bot->IsInWorld() && player->IsInWorld() && !bot->IsBeingTeleported())
                    {
                        bot->TeleportTo(player->GetMapId(), player->GetPositionX() + urand(1, 3), player->GetPositionY() + urand(1, 3), player->GetPositionZ(), player->GetOrientation());
                    }
                }
            }
        }
    }

    players.push_back(player);
    UpdateAIInternal(0);
}

Player* RandomPlayerbotMgr::GetRandomPlayer()
{
    if (players.empty())
    {
        return NULL;
    }

    uint32 index = urand(0, players.size() - 1);
    return players[index];
}

void RandomPlayerbotMgr::PrintStats()
{
    sLog.outString("%d Random Bots online", playerBots.size());

    map<uint32, int> alliance, horde;
    for (uint32 i = 0; i < 10; ++i)
    {
        alliance[i] = 0;
        horde[i] = 0;
    }

    map<uint8, int> perRace, perClass;
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        perRace[race] = 0;
    }
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        perClass[cls] = 0;
    }

    int dps = 0, heal = 0, tank = 0;
    for (PlayerBotMap::iterator i = playerBots.begin(); i != playerBots.end(); ++i)
    {
        Player* bot = i->second;
        if (IsAlliance(bot->getRace()))
        {
            alliance[bot->getLevel() / 10]++;
        }
        else
        {
            horde[bot->getLevel() / 10]++;
        }

        perRace[bot->getRace()]++;
        perClass[bot->getClass()]++;

        int spec = AiFactory::GetPlayerSpecTab(bot);
        switch (bot->getClass())
        {
        case CLASS_DRUID:
            if (spec == 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_PALADIN:
            if (spec == 1)
            {
                tank++;
            }
            else if (spec == 0)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_PRIEST:
            if (spec != 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_SHAMAN:
            if (spec == 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_WARRIOR:
            if (spec == 2)
            {
                tank++;
            }
            else
            {
                dps++;
            }
            break;
        default:
            dps++;
            break;
        }
    }

    sLog.outString("Per level:");
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    for (uint32 i = 0; i < 10; ++i)
    {
        if (!alliance[i] && !horde[i])
        {
            continue;
        }

        uint32 from = i*10;
        uint32 to = min(from + 9, maxLevel);
        if (!from) from = 1;
        {
            sLog.outString("    %d..%d: %d alliance, %d horde", from, to, alliance[i], horde[i]);
        }
    }
    sLog.outString("Per race:");
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        if (perRace[race])
        {
            sLog.outString("    %s: %d", ChatHelper::formatRace(race).c_str(), perRace[race]);
        }
    }
    sLog.outString("Per class:");
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        if (perClass[cls])
        {
            sLog.outString("    %s: %d", ChatHelper::formatClass(cls).c_str(), perClass[cls]);
        }
    }
    sLog.outString("Per role:");
    sLog.outString("    tank: %d", tank);
    sLog.outString("    heal: %d", heal);
    sLog.outString("    dps: %d", dps);
}

double RandomPlayerbotMgr::GetBuyMultiplier(Player* bot)
{
    uint32 id = bot->GetObjectGuid();
    uint32 value = GetEventValue(id, "buymultiplier");
    if (!value)
    {
        value = urand(1, 120);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "buymultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

double RandomPlayerbotMgr::GetSellMultiplier(Player* bot)
{
    uint32 id = bot->GetObjectGuid();
    uint32 value = GetEventValue(id, "sellmultiplier");
    if (!value)
    {
        value = urand(80, 250);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "sellmultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

uint32 RandomPlayerbotMgr::GetLootAmount(Player* bot)
{
    uint32 id = bot->GetObjectGuid();
    return GetEventValue(id, "lootamount");
}

void RandomPlayerbotMgr::SetLootAmount(Player* bot, uint32 value)
{
    uint32 id = bot->GetObjectGuid();
    SetEventValue(id, "lootamount", value, 24 * 3600);
}

uint32 RandomPlayerbotMgr::GetTradeDiscount(Player* bot)
{
    Group* group = bot->GetGroup();
    return GetLootAmount(bot) / (group ? group->GetMembersCount() : 10);
}

enum BotRole
{
    BOT_ROLE_TANK,
    BOT_ROLE_HEALER,
    BOT_ROLE_DPS
};

static BotRole GetBotRole(Player* bot)
{
    uint8 cls = bot->getClass();
    switch (cls)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
            return BOT_ROLE_TANK;
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return BOT_ROLE_HEALER;
        case CLASS_DRUID:
            return (urand(0, 1) == 0) ? BOT_ROLE_TANK : BOT_ROLE_HEALER;
        default:
            return BOT_ROLE_DPS;
    }
}

struct DungeonTeleport
{
    uint32 mapId;
    float x, y, z, o;
};

static DungeonTeleport GetDungeonEntrance(uint8 team, uint8 levelBracket)
{
    if (levelBracket <= 1)
    {
        if (team == 1) // Horde -> Ragefire Chasm
            return {1, 1807.5f, -4405.0f, -18.4f, 0.0f};
        else // Alliance -> Deadmines
            return {0, -11208.0f, 1667.0f, 24.7f, 0.0f};
    }
    else if (levelBracket == 2)
    {
        if (team == 1) // Horde -> Wailing Caverns
            return {1, -744.0f, -1428.0f, 20.0f, 0.0f};
        else // Alliance -> Stockade
            return {0, -8760.0f, 843.0f, 87.0f, 0.0f};
    }
    else if (levelBracket == 3)
    {
        if (team == 1) // Horde -> RFK
            return {1, -4467.0f, -2053.0f, 86.0f, 0.0f};
        else // Alliance -> Gnomeregan
            return {0, -5164.0f, 627.0f, 297.0f, 0.0f};
    }
    else if (levelBracket == 4)
    {
        return {0, 2871.0f, -807.0f, 160.0f, 0.0f}; // Scarlet Monastery
    }
    else if (levelBracket == 5)
    {
        return {1, -6801.0f, -2892.0f, 9.0f, 0.0f}; // Zul'Farrak
    }
    else
    {
        return {0, 3381.0f, -3364.0f, 142.0f, 0.0f}; // Stratholme
    }
}

void RandomPlayerbotMgr::FormRandomGroups()
{
    map<uint8, map<uint8, vector<Player*>>> tanks;
    map<uint8, map<uint8, vector<Player*>>> healers;
    map<uint8, map<uint8, vector<Player*>>> dps;

    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* bot = it->second;
        if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->GetGroup() || bot->GetGroupInvite() || bot->IsBeingTeleported())
            continue;

        uint8 team = (bot->GetTeam() == HORDE) ? 1 : 0;
        uint8 levelBracket = bot->getLevel() / 10;
        if (levelBracket < 1) continue;

        BotRole role = GetBotRole(bot);
        if (role == BOT_ROLE_TANK)
            tanks[team][levelBracket].push_back(bot);
        else if (role == BOT_ROLE_HEALER)
            healers[team][levelBracket].push_back(bot);
        else
            dps[team][levelBracket].push_back(bot);
    }

    for (uint8 team = 0; team <= 1; ++team)
    {
        for (uint8 bracket = 1; bracket <= 6; ++bracket)
        {
            vector<Player*>& tList = tanks[team][bracket];
            vector<Player*>& hList = healers[team][bracket];
            vector<Player*>& dList = dps[team][bracket];

            while (!tList.empty() && !hList.empty() && dList.size() >= 3)
            {
                Player* tank = tList.back(); tList.pop_back();
                Player* healer = hList.back(); hList.pop_back();
                Player* dps1 = dList.back(); dList.pop_back();
                Player* dps2 = dList.back(); dList.pop_back();
                Player* dps3 = dList.back(); dList.pop_back();

                if (!tank || !tank->GetSession()) continue;

                Player* members[4] = { healer, dps1, dps2, dps3 };
                for (int m = 0; m < 4; ++m)
                {
                    if (members[m] && members[m]->GetSession())
                    {
                        WorldPacket p;
                        p << members[m]->GetName();
                        uint32 roles_mask = 0;
                        p << roles_mask;
                        tank->GetSession()->HandleGroupInviteOpcode(p);
                    }
                }

                DungeonTeleport dest = GetDungeonEntrance(team, bracket);
                tank->TeleportTo(dest.mapId, dest.x, dest.y, dest.z, dest.o);
                healer->TeleportTo(dest.mapId, dest.x + urand(1, 3), dest.y + urand(1, 3), dest.z, dest.o);
                dps1->TeleportTo(dest.mapId, dest.x - urand(1, 3), dest.y + urand(1, 3), dest.z, dest.o);
                dps2->TeleportTo(dest.mapId, dest.x + urand(1, 3), dest.y - urand(1, 3), dest.z, dest.o);
                dps3->TeleportTo(dest.mapId, dest.x - urand(1, 3), dest.y - urand(1, 3), dest.z, dest.o);

                sLog.outString("FormRandomGroups: Created 5-man Dungeon Party (Tank: %s, Healer: %s) for Bracket %u, Teleported to Dungeon Entrance",
                               tank->GetName(), healer->GetName(), bracket);
            }
        }
    }
}


void RandomPlayerbotMgr::TeleportBotsToPlayers()
{
    if (players.empty())
        return;

    int targetBotsPerHuman = 10;

    for (vector<Player*>::iterator pit = players.begin(); pit != players.end(); ++pit)
    {
        Player* human = *pit;
        if (!human || !human->IsInWorld() || human->IsBeingTeleported())
            continue;

        uint32 mapId = human->GetMapId();
        float px = human->GetPositionX();
        float py = human->GetPositionY();
        float pz = human->GetPositionZ();
        uint8 humanLevel = human->getLevel();
        Team humanTeam = human->GetTeam();
        uint8 humanRace = human->getRace();

        int nearbyBots = 0;
        set<uint8> existingClasses;

        for (PlayerBotMap::const_iterator bit = GetPlayerBotsBegin(); bit != GetPlayerBotsEnd(); ++bit)
        {
            Player* bot = bit->second;
            if (bot && bot->IsInWorld() && bot->GetMapId() == mapId && bot->GetDistance(human) < 150.0f)
            {
                if (bot->GetTeam() == humanTeam && std::abs((int)bot->getLevel() - (int)humanLevel) <= 5)
                {
                    nearbyBots++;
                    existingClasses.insert(bot->getClass());
                }
            }
        }

        int botsNeeded = targetBotsPerHuman - nearbyBots;
        if (botsNeeded <= 0)
            continue;

        // Two passes: Pass 1 prioritizes same race & missing class; Pass 2 fills remaining slots
        for (int pass = 0; pass < 2 && botsNeeded > 0; ++pass)
        {
            for (PlayerBotMap::const_iterator bit = GetPlayerBotsBegin(); bit != GetPlayerBotsEnd(); ++bit)
            {
                if (botsNeeded <= 0)
                    break;

                Player* bot = bit->second;
                if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->GetGroup() || bot->GetGroupInvite() || bot->IsBeingTeleported() || bot->IsInCombat())
                    continue;

                if (bot->GetTeam() != humanTeam)
                    continue;

                if (pass == 0 && bot->getRace() != humanRace)
                    continue;

                bool nearAnotherHuman = false;
                for (vector<Player*>::iterator oit = players.begin(); oit != players.end(); ++oit)
                {
                    Player* otherHuman = *oit;
                    if (otherHuman && otherHuman != human && bot->GetMapId() == otherHuman->GetMapId() && bot->GetDistance(otherHuman) < 150.0f)
                    {
                        nearAnotherHuman = true;
                        break;
                    }
                }
                if (nearAnotherHuman)
                    continue;

                uint8 botClass = bot->getClass();
                if (pass == 0 && botsNeeded > 1 && existingClasses.count(botClass) > 0 && existingClasses.size() < 6)
                {
                    continue;
                }

                int levelDiff = std::abs((int)bot->getLevel() - (int)humanLevel);
                if (levelDiff > 5)
                {
                    int delta = (int)urand(0, 4) - 2;
                    uint32 targetLevel = (uint32)std::max(1, std::min(60, (int)humanLevel + delta));

                    bot->SetLevel(targetLevel);
                    PlayerbotFactory factory(bot, targetLevel);
                    factory.Refresh();
                    factory.AutoLearnAll();
                    sLog.outBasic("TeleportBotsToPlayers: Recycled bot %s level to %u to match human player %s (Lvl %u)",
                        bot->GetName(), targetLevel, human->GetName(), humanLevel);
                }

                float angle = (float)urand(0, 360) * 3.14159265f / 180.0f;
                float dist = (float)urand(20, 45);
                float tx = px + dist * cos(angle);
                float ty = py + dist * sin(angle);
                float tz = pz;

                Map* map = sMapMgr.FindMap(mapId);
                if (map && map->GetTerrain())
                {
                    tz = map->GetTerrain()->GetHeightStatic(tx, ty, 0.5f + pz, true, MAX_HEIGHT);
                    if (tz > INVALID_HEIGHT)
                    {
                        bot->GetMotionMaster()->Clear();
                        bot->TeleportTo(mapId, tx, ty, tz, 0);
                        botsNeeded--;
                        existingClasses.insert(botClass);
                        sLog.outBasic("TeleportBotsToPlayers: Spawned bot %s (%s Lvl %u) near human player %s (Lvl %u)",
                            bot->GetName(), bot->getRace() == humanRace ? "Same-Race" : "Same-Faction", bot->getLevel(), human->GetName(), humanLevel);
                    }
                }
            }
        }
    }
}

void RandomPlayerbotMgr::RelocateIdleBots()
{
    for (PlayerBotMap::const_iterator bit = GetPlayerBotsBegin(); bit != GetPlayerBotsEnd(); ++bit)
    {
        Player* bot = bit->second;
        if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->GetGroup() || bot->GetGroupInvite() || bot->IsBeingTeleported() || bot->IsInCombat())
            continue;


        uint32 zoneLevel = GetZoneLevel(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
        bool isLevelMismatch = (zoneLevel > 0 && (bot->getLevel() > zoneLevel + 12 || bot->getLevel() + 12 < zoneLevel));

        if (isLevelMismatch)
        {
            sLog.outBasic("RelocateIdleBots: Relocating bot %s (Lvl %u) out of mismatch zone (Zone Lvl %u)",
                bot->GetName(), bot->getLevel(), zoneLevel);
            RandomTeleportForLevel(bot);
        }
    }
}

void RandomPlayerbotMgr::ProcessGuilds()
{
    vector<Player*> allianceBots;
    vector<Player*> hordeBots;

    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* bot = it->second;
        if (!bot || !bot->IsInWorld() || bot->GetGuildId() || bot->getLevel() < 20)
            continue;

        if (bot->GetTeam() == HORDE)
            hordeBots.push_back(bot);
        else
            allianceBots.push_back(bot);
    }

    static const char* allianceGuildNames[] = {
        "Stormwind Guard", "Knights of Silver Hand", "Ironforge Honor",
        "Darnassus Vanguard", "Champions of Alliance"
    };

    static const char* hordeGuildNames[] = {
        "Orgrimmar Vanguard", "Darkspear Tribe", "Thunder Bluff Braves",
        "Forsaken Crusade", "Champions of Horde"
    };

    if (allianceBots.size() >= 3)
    {
        Player* leader = allianceBots[0];
        const char* gName = allianceGuildNames[urand(0, 4)];
        if (!sGuildMgr.GetGuildByName(gName))
        {
            Guild* guild = new Guild;
            if (guild->Create(leader, gName))
            {
                sGuildMgr.AddGuild(guild);
                sLog.outString("ProcessGuilds: Bot %s created Alliance guild '%s'", leader->GetName(), gName);
                for (size_t i = 1; i < allianceBots.size(); ++i)
                {
                    guild->AddMember(allianceBots[i]->GetObjectGuid(), GR_INITIATE);
                }
            }
            else
            {
                delete guild;
            }
        }
    }

    if (hordeBots.size() >= 3)
    {
        Player* leader = hordeBots[0];
        const char* gName = hordeGuildNames[urand(0, 4)];
        if (!sGuildMgr.GetGuildByName(gName))
        {
            Guild* guild = new Guild;
            if (guild->Create(leader, gName))
            {
                sGuildMgr.AddGuild(guild);
                sLog.outString("ProcessGuilds: Bot %s created Horde guild '%s'", leader->GetName(), gName);
                for (size_t i = 1; i < hordeBots.size(); ++i)
                {
                    guild->AddMember(hordeBots[i]->GetObjectGuid(), GR_INITIATE);
                }
            }
            else
            {
                delete guild;
            }
        }
    }
}

void RandomPlayerbotMgr::FormRaidGroups()
{
    map<uint8, vector<Player*>> raidTanks;
    map<uint8, vector<Player*>> raidHealers;
    map<uint8, vector<Player*>> raidDps;

    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* bot = it->second;
        if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->GetGroup() || bot->GetGroupInvite() || bot->IsBeingTeleported() || bot->getLevel() < 60)
            continue;

        uint8 team = (bot->GetTeam() == HORDE) ? 1 : 0;
        BotRole role = GetBotRole(bot);
        if (role == BOT_ROLE_TANK)
            raidTanks[team].push_back(bot);
        else if (role == BOT_ROLE_HEALER)
            raidHealers[team].push_back(bot);
        else
            raidDps[team].push_back(bot);
    }

    struct RaidTeleport
    {
        uint32 mapId;
        float x, y, z, o;
    };

    static const RaidTeleport raidEntrances[] = {
        {0, 7575.0f, -1178.0f, 474.0f, 0.0f}, // Molten Core
        {1, -4675.0f, -3720.0f, 45.0f, 0.0f}, // Onyxia's Lair
        {0, 7520.0f, -1215.0f, 480.0f, 0.0f}  // Blackwing Lair
    };

    for (uint8 team = 0; team <= 1; ++team)
    {
        vector<Player*>& tList = raidTanks[team];
        vector<Player*>& hList = raidHealers[team];
        vector<Player*>& dList = raidDps[team];

        if (!tList.empty() && hList.size() >= 2 && dList.size() >= 5)
        {
            vector<Player*> raidRoster;
            while (!tList.empty() && raidRoster.size() < 4) { raidRoster.push_back(tList.back()); tList.pop_back(); }
            while (!hList.empty() && raidRoster.size() < 12) { raidRoster.push_back(hList.back()); hList.pop_back(); }
            while (!dList.empty() && raidRoster.size() < 40) { raidRoster.push_back(dList.back()); dList.pop_back(); }

            if (raidRoster.size() < 10) continue;

            Player* leader = raidRoster[0];
            if (!leader || !leader->GetSession()) continue;

            for (size_t i = 1; i < raidRoster.size(); ++i)
            {
                Player* member = raidRoster[i];
                if (member && member->GetSession())
                {
                    WorldPacket p;
                    p << member->GetName();
                    uint32 roles_mask = 0;
                    p << roles_mask;
                    leader->GetSession()->HandleGroupInviteOpcode(p);
                }
            }

            Group* group = leader->GetGroup();
            if (group && !group->isRaidGroup())
            {
                group->ConvertToRaid();
            }

            RaidTeleport dest = raidEntrances[urand(0, 2)];
            for (size_t i = 0; i < raidRoster.size(); ++i)
            {
                if (raidRoster[i])
                {
                    raidRoster[i]->TeleportTo(dest.mapId, dest.x + (i % 5) * 2.0f, dest.y + (i / 5) * 2.0f, dest.z, dest.o);
                }
            }

            sLog.outString("FormRaidGroups: Created 40-man Raid Group (%u members) led by %s, Teleported to Raid Entrance",
                           (uint32)raidRoster.size(), leader->GetName());
        }
    }
}

struct CityInvasionTarget
{
    const char* cityName;
    uint32 mapId;
    float x;
    float y;
    float z;
    float o;
    bool targetIsHorde;
};

void RandomPlayerbotMgr::FormCityInvasionRaids()
{
    if (urand(1, 3) != 1)
        return;

    static CityInvasionTarget invasionTargets[] = {
        {"Orgrimmar", 1, -254.0f, -4950.0f, 23.0f, 1.57f, true},
        {"Undercity", 0, 1586.0f, 240.0f, -52.0f, 0.0f, true},
        {"Thunder Bluff", 1, -1280.0f, 120.0f, 130.0f, 0.0f, true},
        {"Stormwind", 0, -9040.0f, 410.0f, 93.0f, 0.0f, false},
        {"Ironforge", 0, -5030.0f, -820.0f, 495.0f, 0.0f, false},
        {"Darnassus", 1, 9950.0f, 2300.0f, 1330.0f, 0.0f, false}
    };

    vector<Player*> allyHighLevel;
    vector<Player*> hordeHighLevel;

    for (vector<Player*>::iterator i = players.begin(); i != players.end(); ++i)
    {
        Player* bot = *i;
        if (!bot || !bot->IsInWorld() || bot->GetGroup() || bot->getLevel() < 55)
            continue;

        if (bot->GetTeam() == ALLIANCE)
            allyHighLevel.push_back(bot);
        else
            hordeHighLevel.push_back(bot);
    }

    if (allyHighLevel.size() >= 10)
    {
        CityInvasionTarget target = invasionTargets[urand(0, 2)];
        Player* leader = allyHighLevel[0];
        uint32 raidSize = std::min((size_t)40, allyHighLevel.size());

        for (size_t i = 1; i < raidSize; ++i)
        {
            Player* member = allyHighLevel[i];
            if (leader->GetSession() && member)
            {
                WorldPacket p(CMSG_GROUP_INVITE, 10);
                p << member->GetName();
                uint32 roles_mask = 0;
                p << roles_mask;
                leader->GetSession()->HandleGroupInviteOpcode(p);
            }
        }

        Group* group = leader->GetGroup();
        if (group && !group->isRaidGroup())
            group->ConvertToRaid();

        for (size_t i = 0; i < raidSize; ++i)
        {
            Player* member = allyHighLevel[i];
            member->SetPvP(true);
            member->TeleportTo(target.mapId, target.x + (i % 5) * 3.0f, target.y + (i / 5) * 3.0f, target.z, target.o);
        }

        char msg[256];
        snprintf(msg, sizeof(msg), "|cffff0000[WORLD RAID ANNOUNCEMENT]|r Alliance City Raid of %u level 60 bots is invading %s!", (uint32)raidSize, target.cityName);
        sWorld.SendWorldText(LANG_SYSTEMMESSAGE, msg);
        sLog.outString("FormCityInvasionRaids: Alliance Bot Raid (%u members) invading %s!", (uint32)raidSize, target.cityName);
    }

    if (hordeHighLevel.size() >= 10)
    {
        CityInvasionTarget target = invasionTargets[urand(3, 5)];
        Player* leader = hordeHighLevel[0];
        uint32 raidSize = std::min((size_t)40, hordeHighLevel.size());

        for (size_t i = 1; i < raidSize; ++i)
        {
            Player* member = hordeHighLevel[i];
            if (leader->GetSession() && member)
            {
                WorldPacket p(CMSG_GROUP_INVITE, 10);
                p << member->GetName();
                uint32 roles_mask = 0;
                p << roles_mask;
                leader->GetSession()->HandleGroupInviteOpcode(p);
            }
        }

        Group* group = leader->GetGroup();
        if (group && !group->isRaidGroup())
            group->ConvertToRaid();

        for (size_t i = 0; i < raidSize; ++i)
        {
            Player* member = hordeHighLevel[i];
            member->SetPvP(true);
            member->TeleportTo(target.mapId, target.x + (i % 5) * 3.0f, target.y + (i / 5) * 3.0f, target.z, target.o);
        }

        char msg[256];
        snprintf(msg, sizeof(msg), "|cffff0000[WORLD RAID ANNOUNCEMENT]|r Horde City Raid of %u level 60 bots is invading %s!", (uint32)raidSize, target.cityName);
        sWorld.SendWorldText(LANG_SYSTEMMESSAGE, msg);
        sLog.outString("FormCityInvasionRaids: Horde Bot Raid (%u members) invading %s!", (uint32)raidSize, target.cityName);
    }
}





