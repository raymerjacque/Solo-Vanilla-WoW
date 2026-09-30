#ifndef MANGOS_NPCBOTDEFINES_H
#define MANGOS_NPCBOTDEFINES_H

#include "Common.h"
#include <string>
#include <vector>

enum NpcBotBehavior
{
    NPCBOT_BEHAVIOR_STAND = 0,
    NPCBOT_BEHAVIOR_SIT   = 1,
    NPCBOT_BEHAVIOR_PATROL= 2,
    NPCBOT_BEHAVIOR_DUEL  = 3,
    NPCBOT_BEHAVIOR_SOCIAL= 4
};

enum NpcBotFaction
{
    NPCBOT_FACTION_ALLIANCE = 0,
    NPCBOT_FACTION_HORDE    = 1,
    NPCBOT_FACTION_NEUTRAL  = 2
};

struct NpcBotSpot
{
    uint32 spotId;
    uint32 mapId;
    float x;
    float y;
    float z;
    float o;
    NpcBotBehavior behavior;
    uint8 race;
    uint8 class_;
    uint8 gender;
    uint8 level;
    std::string name;
    uint32 duelPartnerSpotId;
    std::vector<std::pair<float, float>> patrolWaypoints;
};

struct NpcBotHub
{
    uint32 hubId;
    std::string name;
    uint32 zoneId;
    uint32 areaId;
    NpcBotFaction faction;
    std::vector<NpcBotSpot> spots;
    bool isActive;
    uint32 activePlayerCount;
};

#endif // MANGOS_NPCBOTDEFINES_H
