#include "botpch.h"
#include "../../playerbot.h"
#include "OutdoorPvPAction.h"
#include "Player.h"
#include "GameObject.h"
#include "World.h"
#include "ObjectMgr.h"

using namespace ai;

bool OutdoorPvPAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->IsBeingTeleported())
        return false;

    // Outdoor PvP is targeted for level 55-60 characters
    if (bot->getLevel() < 55)
        return false;

    // Do not interfere if bot is following human master in party elsewhere
    Player* master = ai->GetMaster();
    if (master && master != bot && master->GetMapId() == bot->GetMapId() && bot->GetDistance(master) < 40.0f)
        return false;

    uint32 zoneId = bot->GetZoneId();

    // ------------------------------------------------------------------------
    // 1. EASTERN PLAGUELANDS (Zone 139) - TOWER CONTROL & LORDAERON'S BLESSING
    // ------------------------------------------------------------------------
    if (zoneId == 139)
    {
        static const struct TowerPoint {
            const char* name;
            float x, y, z;
        } epTowers[4] = {
            {"Northpass Tower", 3181.08f, -4379.36f, 138.0f},
            {"Crownguard Tower", 1860.85f, -3731.23f, 138.0f},
            {"Eastwall Tower", 2574.51f, -4794.89f, 138.0f},
            {"Plague-wood Tower", 2962.71f, -3042.31f, 138.0f}
        };

        // Pick a tower to assault or defend (cycle based on bot GUID)
        uint32 towerIdx = (bot->GetGUIDLow() + (time(NULL) / 300)) % 4;
        const TowerPoint& targetTower = epTowers[towerIdx];

        float dist = bot->GetDistance(targetTower.x, targetTower.y, targetTower.z);
        if (dist > 15.0f)
        {
            // Move toward tower
            bot->GetMotionMaster()->MovePoint(0, targetTower.x, targetTower.y, targetTower.z);
            return true;
        }
        else
        {
            // Standing inside tower zone: emote or announce defense
            if (urand(0, 100) < 5)
            {
                bot->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
            }
            return true;
        }
    }

    // ------------------------------------------------------------------------
    // 2. SILITHUS (Zone 1377) - SILITHYST SAND GATHERING & CENARION FAVOR
    // ------------------------------------------------------------------------
    if (zoneId == 1377)
    {
        // SPELL_SILITHYST = 29519 (carrying silithyst sand flag)
        if (bot->HasAura(29519))
        {
            // Move to faction turn-in area in Silithus
            float turnInX = (bot->GetTeam() == ALLIANCE) ? -6830.0f : -6750.0f;
            float turnInY = (bot->GetTeam() == ALLIANCE) ? 780.0f : 950.0f;
            float turnInZ = (bot->GetTeam() == ALLIANCE) ? 42.0f : 45.0f;

            float dist = bot->GetDistance(turnInX, turnInY, turnInZ);
            if (dist > 10.0f)
            {
                bot->GetMotionMaster()->MovePoint(0, turnInX, turnInY, turnInZ);
                return true;
            }
            else
            {
                // Reached turn in spot
                if (urand(0, 100) < 10)
                {
                    bot->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                }
                return true;
            }
        }
        else
        {
            // Look for nearby Silithyst Geyser (GO 181598) or Silithyst Mound (GO 181597)
            // Or head to Crystal Vale (X=-7100.0f, Y=1400.0f)
            float geyserX = -7100.0f + (urand(0, 200) - 100);
            float geyserY = 1400.0f + (urand(0, 200) - 100);
            float geyserZ = 25.0f;

            float dist = bot->GetDistance(geyserX, geyserY, geyserZ);
            if (dist > 20.0f)
            {
                bot->GetMotionMaster()->MovePoint(0, geyserX, geyserY, geyserZ);
                return true;
            }
        }
    }

    return false;
}
