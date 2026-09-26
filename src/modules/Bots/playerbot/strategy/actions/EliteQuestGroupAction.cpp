#include "botpch.h"
#include "../../playerbot.h"
#include "EliteQuestGroupAction.h"
#include "Player.h"
#include "Creature.h"
#include "World.h"
#include "ObjectMgr.h"

using namespace ai;

bool EliteQuestGroupAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->IsBeingTeleported())
        return false;

    // Do not interfere if bot is following human master in party during active combat
    if (bot->IsInCombat())
        return false;

    uint32 zoneId = bot->GetZoneId();

    // ------------------------------------------------------------------------
    // 1. JIN'THA'ALOR ELITE TERRACES (Hinterlands Zone 47)
    // ------------------------------------------------------------------------
    if (zoneId == 47 && bot->getLevel() >= 45 && bot->getLevel() <= 52)
    {
        static const float jintalorX = 350.0f;
        static const float jintalorY = -3800.0f;
        static const float jintalorZ = 140.0f;

        float dist = bot->GetDistance(jintalorX, jintalorY, jintalorZ);
        if (dist > 20.0f && dist < 300.0f)
        {
            bot->GetMotionMaster()->MovePoint(0, jintalorX, jintalorY, jintalorZ);
            return true;
        }
    }

    // ------------------------------------------------------------------------
    // 2. STROMGARDE KEEP ELITE CITY (Arathi Highlands Zone 45)
    // ------------------------------------------------------------------------
    if (zoneId == 45 && bot->getLevel() >= 35 && bot->getLevel() <= 42)
    {
        static const float stromgardeX = -2850.0f;
        static const float stromgardeY = -2100.0f;
        static const float stromgardeZ = 50.0f;

        float dist = bot->GetDistance(stromgardeX, stromgardeY, stromgardeZ);
        if (dist > 20.0f && dist < 300.0f)
        {
            bot->GetMotionMaster()->MovePoint(0, stromgardeX, stromgardeY, stromgardeZ);
            return true;
        }
    }

    return false;
}
