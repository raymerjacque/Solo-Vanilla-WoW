#include "WorldHandlers/LoginQueryHolder.h"
#include "NpcBotMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "MapManager.h"
#include "Log.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseImpl.h"

NpcBotMgr::NpcBotMgr() : m_updateTimer(0)
{
}

NpcBotMgr::~NpcBotMgr()
{
}

bool NpcBotMgr::IsNpcBot(Player* player) const
{
    if (!player)
        return false;
    return IsNpcBot(player->GetObjectGuid());
}

bool NpcBotMgr::IsNpcBot(ObjectGuid guid) const
{
    return m_npcBotGuids.find(guid) != m_npcBotGuids.end();
}

void NpcBotMgr::Initialize()
{
    m_hubs.clear();
    m_zoneToHubs.clear();
    m_areaToHubs.clear();
    m_npcBotGuids.clear();
    m_spotToBotGuid.clear();

    RegisterHubs();

    sLog.outString("NPCBot Manager initialized with %u ambient hubs.", (uint32)m_hubs.size());
}

void NpcBotMgr::OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea)
{
    if (!player || IsNpcBot(player))
        return;

    uint32 oldZone = player->GetZoneId();

    // Activate hubs for new zone / area
    auto zIt = m_zoneToHubs.find(newZone);
    if (zIt != m_zoneToHubs.end())
    {
        for (uint32 hubId : zIt->second)
        {
            auto hIt = m_hubs.find(hubId);
            if (hIt != m_hubs.end())
            {
                hIt->second.activePlayerCount++;
                if (!hIt->second.isActive)
                {
                    ActivateHub(hubId);
                }
            }
        }
    }

    auto aIt = m_areaToHubs.find(newArea);
    if (aIt != m_areaToHubs.end())
    {
        for (uint32 hubId : aIt->second)
        {
            auto hIt = m_hubs.find(hubId);
            if (hIt != m_hubs.end())
            {
                hIt->second.activePlayerCount++;
                if (!hIt->second.isActive)
                {
                    ActivateHub(hubId);
                }
            }
        }
    }

    // Deactivate hubs for old zone if different
    if (oldZone != 0 && oldZone != newZone)
    {
        auto ozIt = m_zoneToHubs.find(oldZone);
        if (ozIt != m_zoneToHubs.end())
        {
            for (uint32 hubId : ozIt->second)
            {
                auto hIt = m_hubs.find(hubId);
                if (hIt != m_hubs.end())
                {
                    if (hIt->second.activePlayerCount > 0)
                        hIt->second.activePlayerCount--;

                    if (hIt->second.activePlayerCount == 0 && hIt->second.isActive)
                    {
                        DeactivateHub(hubId);
                    }
                }
            }
        }
    }
}

void NpcBotMgr::ActivateHub(uint32 hubId)
{
    auto hIt = m_hubs.find(hubId);
    if (hIt == m_hubs.end())
        return;

    NpcBotHub& hub = hIt->second;
    hub.isActive = true;

    sLog.outDetail("Activating NPCBot Hub: %s (Zone: %u)", hub.name.c_str(), hub.zoneId);

    for (const NpcBotSpot& spot : hub.spots)
    {
        SpawnBotForSpot(hub, spot);
    }
}

void NpcBotMgr::DeactivateHub(uint32 hubId)
{
    auto hIt = m_hubs.find(hubId);
    if (hIt == m_hubs.end())
        return;

    NpcBotHub& hub = hIt->second;
    hub.isActive = false;

    sLog.outDetail("Deactivating NPCBot Hub: %s", hub.name.c_str());

    for (const NpcBotSpot& spot : hub.spots)
    {
        DespawnBotForSpot(spot.spotId);
    }
}

const NpcBotSpot* NpcBotMgr::FindSpot(uint32 spotId) const
{
    for (const auto& hPair : m_hubs)
    {
        for (const auto& spot : hPair.second.spots)
        {
            if (spot.spotId == spotId)
                return &spot;
        }
    }
    return nullptr;
}

void NpcBotMgr::SpawnBotForSpot(const NpcBotHub& hub, const NpcBotSpot& spot)
{
    // Query available character guid and account from database
    QueryResult* result = CharacterDatabase.PQuery("SELECT guid, account FROM characters LIMIT 1 OFFSET %u", (spot.spotId * 7) % 250);
    if (!result)
    {
        result = CharacterDatabase.Query("SELECT guid, account FROM characters LIMIT 1");
        if (!result)
            return;
    }

    Field* fields = result->Fetch();
    uint32 lowguid = fields[0].GetUInt32();
    uint32 accountId = fields[1].GetUInt32();
    delete result;

    ObjectGuid botGuid(HIGHGUID_PLAYER, lowguid);

    // If bot is already online in world
    Player* bot = sObjectMgr.GetPlayer(botGuid);
    if (bot && bot->IsInWorld())
    {
        OnNpcBotLoaded(bot, spot);
        return;
    }

    // Otherwise, load character asynchronously from DB using NpcBotLoginQueryHolder
    NpcBotLoginQueryHolder* holder = new NpcBotLoginQueryHolder(spot.spotId, accountId, botGuid);
    if (!holder->Initialize())
    {
        delete holder;
        return;
    }

    CharacterDatabase.DelayQueryHolder(this, &NpcBotMgr::HandleNpcBotLoginCallback, holder);
}

void NpcBotMgr::HandleNpcBotLoginCallback(QueryResult* /*dummy*/, SqlQueryHolder* holder)
{
    NpcBotLoginQueryHolder* nqh = static_cast<NpcBotLoginQueryHolder*>(holder);
    if (!nqh)
        return;

    uint32 spotId = nqh->GetSpotId();
    uint32 accountId = nqh->GetAccountId();
    ObjectGuid botGuid = nqh->GetGuid();

    if (sObjectMgr.GetPlayer(botGuid))
    {
        delete nqh;
        return;
    }

    WorldSession* botSession = new WorldSession(accountId, NULL, SEC_PLAYER, 0, LOCALE_enUS);
    botSession->HandlePlayerLogin(nqh); // HandlePlayerLogin deletes nqh!

    Player* bot = botSession->GetPlayer();
    if (!bot || !bot->IsInWorld())
    {
        return;
    }

    const NpcBotSpot* pSpot = FindSpot(spotId);
    if (pSpot)
    {
        OnNpcBotLoaded(bot, *pSpot);
    }
}

void NpcBotMgr::OnNpcBotLoaded(Player* bot, const NpcBotSpot& spot)
{
    if (!bot || !bot->IsInWorld())
        return;

    if (bot->GetMapId() == spot.mapId)
    {
        bot->GetMap()->Remove(bot, false);
        bot->Relocate(spot.x, spot.y, spot.z, spot.o);
        bot->GetMap()->Add(bot);
    }
    else
    {
        bot->TeleportTo(spot.mapId, spot.x, spot.y, spot.z, spot.o);
    }

    if (spot.behavior == NPCBOT_BEHAVIOR_SIT)
    {
        bot->SetStandState(UNIT_STAND_STATE_SIT);
    }
    else
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
    }

    m_npcBotGuids.insert(bot->GetObjectGuid());
    m_spotToBotGuid[spot.spotId] = bot->GetObjectGuid();

    sLog.outString("NPCBot '%s' (GUID: %u) loaded successfully at spot %u (%s)", bot->GetName(), bot->GetGUIDLow(), spot.spotId, spot.name.c_str());
}

void NpcBotMgr::DespawnBotForSpot(uint32 spotId)
{
    auto sIt = m_spotToBotGuid.find(spotId);
    if (sIt == m_spotToBotGuid.end())
        return;

    ObjectGuid botGuid = sIt->second;
    Player* bot = sObjectMgr.GetPlayer(botGuid);
    if (bot && bot->IsInWorld())
    {
        WorldSession* session = bot->GetSession();
        m_npcBotGuids.erase(botGuid);
        m_spotToBotGuid.erase(sIt);
        if (session)
        {
            session->LogoutPlayer(true);
            delete session;
        }
    }
    else
    {
        m_npcBotGuids.erase(botGuid);
        m_spotToBotGuid.erase(sIt);
    }
}

void NpcBotMgr::Update(uint32 diff)
{
    // Process bot teleport acks
    for (ObjectGuid guid : m_npcBotGuids)
    {
        Player* bot = sObjectMgr.GetPlayer(guid);
        if (bot && bot->IsBeingTeleported())
        {
            bot->GetMotionMaster()->Clear(true);
            if (bot->IsBeingTeleportedNear())
            {
                WorldPacket p = WorldPacket(MSG_MOVE_TELEPORT_ACK, 8 + 4 + 4);
                p << bot->GetObjectGuid();
                p << (uint32)0;
                p << (uint32)time(0);
                bot->GetSession()->HandleMoveTeleportAckOpcode(p);
            }
            else if (bot->IsBeingTeleportedFar())
            {
                bot->GetSession()->HandleMoveWorldportAckOpcode();
            }
        }
    }

    m_updateTimer += diff;
    if (m_updateTimer < 2000) // Update every 2 seconds
        return;

    m_updateTimer = 0;

    // Check online human player zones to dynamically activate matching hubs
    sObjectAccessor.DoForAllPlayers([this](Player* player)
    {
        if (!player || IsNpcBot(player) || !player->IsInWorld())
            return;

        uint32 pZone = player->GetZoneId();
        uint32 pArea = player->GetAreaId();

        for (auto& hPair : m_hubs)
        {
            NpcBotHub& hub = hPair.second;
            if (pZone == hub.zoneId || (hub.areaId != 0 && pArea == hub.areaId))
            {
                if (!hub.isActive)
                {
                    ActivateHub(hub.hubId);
                }
            }
        }
    });

    UpdateBotBehaviors(diff);
}

void NpcBotMgr::UpdateBotBehaviors(uint32 diff)
{
    for (ObjectGuid guid : m_npcBotGuids)
    {
        Player* bot = sObjectMgr.GetPlayer(guid);
        if (!bot || !bot->IsInWorld())
            continue;

        // Maintain sit state for SIT behavior bots or combat emotes for DUEL bots
        for (const auto& sPair : m_spotToBotGuid)
        {
            if (sPair.second == guid)
            {
                const NpcBotSpot* pSpot = FindSpot(sPair.first);
                if (pSpot && pSpot->behavior == NPCBOT_BEHAVIOR_SIT)
                {
                    if (bot->getStandState() != UNIT_STAND_STATE_SIT)
                        bot->SetStandState(UNIT_STAND_STATE_SIT);
                }
                else if (pSpot && pSpot->behavior == NPCBOT_BEHAVIOR_DUEL)
                {
                    if (urand(0, 100) < 25)
                    {
                        uint32 combatEmote = EMOTE_ONESHOT_ATTACK1H;
                        switch (urand(0, 4))
                        {
                            case 0: combatEmote = EMOTE_ONESHOT_ATTACK1H; break;
                            case 1: combatEmote = EMOTE_ONESHOT_PARRYUNARMED; break;
                            case 2: combatEmote = EMOTE_ONESHOT_SPELLCAST; break;
                            case 3: combatEmote = EMOTE_ONESHOT_BATTLEROAR; break;
                            case 4: combatEmote = EMOTE_ONESHOT_SPECIALATTACK1H; break;
                        }
                        bot->HandleEmoteCommand(combatEmote);
                    }
                }
                break;
            }
        }

        // Occasional ambient social emote
        if (urand(0, 100) < 10)
        {
            uint32 emote = EMOTE_ONESHOT_WAVE;
            switch (urand(0, 5))
            {
                case 0: emote = EMOTE_ONESHOT_WAVE; break;
                case 1: emote = EMOTE_ONESHOT_BOW; break;
                case 2: emote = EMOTE_ONESHOT_TALK; break;
                case 3: emote = EMOTE_ONESHOT_CHEER; break;
                case 4: emote = EMOTE_ONESHOT_LAUGH; break;
                case 5: emote = EMOTE_ONESHOT_EAT; break;
            }
            bot->HandleEmoteCommand(emote);
        }
    }
}

void NpcBotMgr::RegisterHubs()
{
    uint32 spotCounter = 1;

    // Helper lambda to register a hub
    auto AddHub = [this, &spotCounter](uint32 id, const std::string& name, uint32 zoneId, uint32 areaId, NpcBotFaction faction, const std::vector<NpcBotSpot>& spots) {
        NpcBotHub hub;
        hub.hubId = id;
        hub.name = name;
        hub.zoneId = zoneId;
        hub.areaId = areaId;
        hub.faction = faction;
        hub.isActive = false;
        hub.activePlayerCount = 0;

        for (auto spot : spots)
        {
            spot.spotId = spotCounter++;
            hub.spots.push_back(spot);
        }

        m_hubs[id] = hub;
        if (zoneId != 0)
            m_zoneToHubs[zoneId].push_back(id);
        if (areaId != 0)
            m_areaToHubs[areaId].push_back(id);
    };

    // 1. STORMWIND CITY (Zone 1519)
    AddHub(1, "Stormwind Trade & Bank", 1519, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -8831.5f, 628.6f, 94.0f, 3.4f, NPCBOT_BEHAVIOR_STAND, RACE_HUMAN, CLASS_PALADIN, GENDER_MALE, 60, "SW Guard Stand"},
        {0, 0, -8838.2f, 630.1f, 94.0f, 0.2f, NPCBOT_BEHAVIOR_STAND, RACE_HUMAN, CLASS_WARRIOR, GENDER_FEMALE, 60, "SW Bank Stand"},
        {0, 0, -8850.1f, 642.3f, 96.2f, 1.5f, NPCBOT_BEHAVIOR_STAND, RACE_DWARF, CLASS_HUNTER, GENDER_MALE, 55, "SW AH Stand"},
        {0, 0, -8860.4f, 670.8f, 96.0f, 4.2f, NPCBOT_BEHAVIOR_STAND, RACE_NIGHTELF, CLASS_ROGUE, GENDER_FEMALE, 60, "SW Square Walk"}
    });

    AddHub(2, "Stormwind Inn (Pig and Whistle)", 1519, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -8792.1f, 762.4f, 15.2f, 2.1f, NPCBOT_BEHAVIOR_SIT, RACE_HUMAN, CLASS_MAGE, GENDER_MALE, 45, "SW Inn Sit 1"},
        {0, 0, -8794.5f, 765.1f, 15.2f, 5.3f, NPCBOT_BEHAVIOR_SIT, RACE_GNOME, CLASS_WARLOCK, GENDER_FEMALE, 30, "SW Inn Sit 2"},
        {0, 0, -8788.3f, 758.9f, 15.2f, 0.8f, NPCBOT_BEHAVIOR_STAND, RACE_HUMAN, CLASS_PRIEST, GENDER_FEMALE, 50, "SW Inn Drinker"}
    });

    AddHub(3, "Stormwind Gates Dueling", 1519, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -9120.5f, 405.2f, 92.5f, 1.2f, NPCBOT_BEHAVIOR_DUEL, RACE_HUMAN, CLASS_WARRIOR, GENDER_MALE, 60, "SW Dueler 1"},
        {0, 0, -9125.1f, 410.8f, 92.5f, 4.3f, NPCBOT_BEHAVIOR_DUEL, RACE_NIGHTELF, CLASS_ROGUE, GENDER_MALE, 60, "SW Dueler 2"}
    });

    // 2. GOLDSHIRE (Zone 12 - Elwynn Forest)
    AddHub(4, "Goldshire Lion's Pride Inn & Square", 12, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -9460.2f, 62.4f, 56.0f, 0.5f, NPCBOT_BEHAVIOR_SIT, RACE_HUMAN, CLASS_PALADIN, GENDER_MALE, 12, "Goldshire Inn Seated"},
        {0, 0, -9463.1f, 65.8f, 56.0f, 3.8f, NPCBOT_BEHAVIOR_SIT, RACE_DWARF, CLASS_WARRIOR, GENDER_MALE, 15, "Goldshire Inn Seated 2"},
        {0, 0, -9472.4f, 40.2f, 56.1f, 2.1f, NPCBOT_BEHAVIOR_STAND, RACE_HUMAN, CLASS_MAGE, GENDER_FEMALE, 10, "Goldshire Anvil Stand"},
        {0, 0, -9485.0f, 82.1f, 56.0f, 5.0f, NPCBOT_BEHAVIOR_DUEL, RACE_HUMAN, CLASS_ROGUE, GENDER_MALE, 20, "Goldshire Dueler 1"},
        {0, 0, -9490.2f, 86.4f, 56.0f, 1.8f, NPCBOT_BEHAVIOR_DUEL, RACE_GNOME, CLASS_MAGE, GENDER_FEMALE, 22, "Goldshire Dueler 2"}
    });

    // 3. LAKESHIRE (Zone 44 - Redridge Mountains)
    AddHub(5, "Lakeshire Town & Inn", 44, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -9250.4f, -2150.1f, 64.0f, 1.4f, NPCBOT_BEHAVIOR_SIT, RACE_HUMAN, CLASS_HUNTER, GENDER_MALE, 22, "Lakeshire Inn Sit"},
        {0, 0, -9238.2f, -2140.5f, 64.1f, 4.2f, NPCBOT_BEHAVIOR_STAND, RACE_HUMAN, CLASS_WARRIOR, GENDER_FEMALE, 25, "Lakeshire Bridge Stand"}
    });

    // 4. DARKSHIRE (Zone 10 - Duskwood)
    AddHub(6, "Darkshire Town Square & Inn", 10, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -10540.2f, -1170.5f, 28.1f, 0.9f, NPCBOT_BEHAVIOR_SIT, RACE_HUMAN, CLASS_WARLOCK, GENDER_MALE, 30, "Darkshire Inn Sit"},
        {0, 0, -10525.8f, -1150.2f, 27.5f, 3.5f, NPCBOT_BEHAVIOR_STAND, RACE_NIGHTELF, CLASS_DRUID, GENDER_MALE, 28, "Darkshire Square Stand"}
    });

    // 5. IRONFORGE (Zone 1537)
    AddHub(7, "Ironforge Bank & Commons", 1537, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -4915.2f, -942.1f, 501.5f, 5.2f, NPCBOT_BEHAVIOR_STAND, RACE_DWARF, CLASS_PALADIN, GENDER_MALE, 60, "IF Bank Stand"},
        {0, 0, -4925.4f, -955.0f, 501.5f, 2.1f, NPCBOT_BEHAVIOR_STAND, RACE_GNOME, CLASS_ROGUE, GENDER_FEMALE, 60, "IF Commons Stand"},
        {0, 0, -4850.1f, -870.4f, 502.1f, 1.8f, NPCBOT_BEHAVIOR_SIT, RACE_DWARF, CLASS_HUNTER, GENDER_MALE, 58, "IF Inn Sit"}
    });

    // 6. KHARANOS (Zone 1 - Dun Morogh)
    AddHub(8, "Kharanos Distillery & Square", 1, 0, NPCBOT_FACTION_ALLIANCE, {
        {0, 0, -5605.1f, -525.4f, 398.2f, 4.1f, NPCBOT_BEHAVIOR_SIT, RACE_DWARF, CLASS_WARRIOR, GENDER_MALE, 10, "Kharanos Inn Sit"},
        {0, 0, -5590.2f, -510.8f, 398.0f, 1.5f, NPCBOT_BEHAVIOR_STAND, RACE_GNOME, CLASS_MAGE, GENDER_FEMALE, 12, "Kharanos Square Stand"}
    });

    // 7. ORGRIMMAR (Zone 1637)
    AddHub(9, "Orgrimmar Bank & Valley of Strength", 1637, 0, NPCBOT_FACTION_HORDE, {
        {0, 0, 1572.4f, -4439.1f, 16.1f, 1.8f, NPCBOT_BEHAVIOR_STAND, RACE_ORC, CLASS_WARRIOR, GENDER_MALE, 60, "Org Bank Stand"},
        {0, 0, 1585.1f, -4450.2f, 16.1f, 4.5f, NPCBOT_BEHAVIOR_STAND, RACE_TROLL, CLASS_HUNTER, GENDER_MALE, 55, "Org AH Stand"},
        {0, 0, 1630.8f, -4380.5f, 20.8f, 3.1f, NPCBOT_BEHAVIOR_SIT, RACE_UNDEAD, CLASS_MAGE, GENDER_FEMALE, 60, "Org Inn Sit"}
    });

    AddHub(10, "Orgrimmar Gates Dueling", 1637, 0, NPCBOT_FACTION_HORDE, {
        {0, 0, 1315.2f, -4370.8f, 26.2f, 0.8f, NPCBOT_BEHAVIOR_DUEL, RACE_ORC, CLASS_SHAMAN, GENDER_MALE, 60, "Org Dueler 1"},
        {0, 0, 1320.8f, -4375.4f, 26.2f, 3.9f, NPCBOT_BEHAVIOR_DUEL, RACE_UNDEAD, CLASS_ROGUE, GENDER_MALE, 60, "Org Dueler 2"}
    });

    // 8. RAZOR HILL (Zone 14 - Durotar)
    AddHub(11, "Razor Hill Square & Inn", 14, 0, NPCBOT_FACTION_HORDE, {
        {0, 0, 312.4f, -4680.1f, 18.2f, 2.5f, NPCBOT_BEHAVIOR_SIT, RACE_ORC, CLASS_WARRIOR, GENDER_MALE, 12, "Razor Hill Inn Sit"},
        {0, 0, 325.8f, -4695.5f, 18.0f, 5.8f, NPCBOT_BEHAVIOR_STAND, RACE_TROLL, CLASS_SHAMAN, GENDER_FEMALE, 10, "Razor Hill Square Stand"}
    });

    // 9. CROSSROADS (Zone 17 - The Barrens)
    AddHub(12, "Crossroads Inn & Tower", 17, 0, NPCBOT_FACTION_HORDE, {
        {0, 0, -448.2f, -2650.4f, 95.8f, 1.1f, NPCBOT_BEHAVIOR_SIT, RACE_TAUREN, CLASS_DRUID, GENDER_MALE, 18, "Crossroads Inn Sit"},
        {0, 0, -460.5f, -2635.1f, 95.8f, 4.3f, NPCBOT_BEHAVIOR_STAND, RACE_ORC, CLASS_HUNTER, GENDER_FEMALE, 22, "Crossroads Stand"}
    });

    // 10. UNDERCITY (Zone 1497)
    AddHub(13, "Undercity Trade Quarter & Bank", 1497, 0, NPCBOT_FACTION_HORDE, {
        {0, 0, 1585.1f, 240.2f, -52.1f, 2.8f, NPCBOT_BEHAVIOR_STAND, RACE_UNDEAD, CLASS_WARLOCK, GENDER_MALE, 60, "UC Bank Stand"},
        {0, 0, 1595.4f, 255.8f, -52.1f, 5.9f, NPCBOT_BEHAVIOR_SIT, RACE_UNDEAD, CLASS_MAGE, GENDER_FEMALE, 55, "UC Inn Sit"}
    });

    // 11. BOOTY BAY (Zone 33 - Stranglethorn Vale)
    AddHub(14, "Booty Bay Salty Sailor Inn & Docks", 33, 0, NPCBOT_FACTION_NEUTRAL, {
        {0, 0, -14350.2f, 520.1f, 8.8f, 1.4f, NPCBOT_BEHAVIOR_SIT, RACE_HUMAN, CLASS_ROGUE, GENDER_MALE, 40, "BB Inn Sit Human"},
        {0, 0, -14355.8f, 525.4f, 8.8f, 4.6f, NPCBOT_BEHAVIOR_SIT, RACE_TROLL, CLASS_SHAMAN, GENDER_MALE, 42, "BB Inn Sit Troll"},
        {0, 0, -14410.5f, 460.2f, 15.2f, 3.1f, NPCBOT_BEHAVIOR_STAND, RACE_GOBLIN, CLASS_WARRIOR, GENDER_MALE, 45, "BB Docks Stand"}
    });

    // 12. GADGETZAN (Zone 440 - Tanaris)
    AddHub(15, "Gadgetzan Arena & Inn", 440, 0, NPCBOT_FACTION_NEUTRAL, {
        {0, 0, -7120.4f, -3820.1f, 8.5f, 0.9f, NPCBOT_BEHAVIOR_SIT, RACE_DWARF, CLASS_HUNTER, GENDER_MALE, 48, "Gadgetzan Inn Sit"},
        {0, 0, -7145.8f, -3790.2f, 8.8f, 4.1f, NPCBOT_BEHAVIOR_DUEL, RACE_ORC, CLASS_WARRIOR, GENDER_MALE, 50, "Gadgetzan Dueler 1"},
        {0, 0, -7150.2f, -3795.8f, 8.8f, 1.2f, NPCBOT_BEHAVIOR_DUEL, RACE_HUMAN, CLASS_PALADIN, GENDER_FEMALE, 50, "Gadgetzan Dueler 2"}
    });
}
