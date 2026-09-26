#include "botpch.h"
#include "../../playerbot.h"
#include "WorldEventsAction.h"
#include "Player.h"
#include "GameObject.h"
#include "World.h"
#include "ObjectMgr.h"

using namespace ai;

bool WorldEventsAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->IsBeingTeleported())
        return false;

    // Do not interfere if bot is following human master in party elsewhere
    Player* master = ai->GetMaster();
    if (master && master != bot && master->GetMapId() == bot->GetMapId() && bot->GetDistance(master) < 40.0f)
        return false;

    uint32 zoneId = bot->GetZoneId();

    // ------------------------------------------------------------------------
    // 1. DARKMOON FAIRE (Elwynn Forest Zone 12 or Mulgore Zone 215)
    // ------------------------------------------------------------------------
    if (zoneId == 12 || zoneId == 215)
    {
        // Sayge location in Elwynn or Mulgore
        float dmfX = (zoneId == 12) ? -9500.0f : -2200.0f;
        float dmfY = (zoneId == 12) ? 0.0f : -450.0f;
        float dmfZ = (zoneId == 12) ? 60.0f : -10.0f;

        float dist = bot->GetDistance(dmfX, dmfY, dmfZ);
        if (dist > 15.0f && dist < 300.0f)
        {
            bot->GetMotionMaster()->MovePoint(0, dmfX, dmfY, dmfZ);
            return true;
        }
        else if (dist <= 15.0f)
        {
            if (urand(0, 100) < 5)
            {
                bot->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
            }
            return true;
        }
    }

    // ------------------------------------------------------------------------
    // 2. GURUBASHI ARENA CHEST (Stranglethorn Vale Zone 33)
    // ------------------------------------------------------------------------
    if (zoneId == 33 && bot->getLevel() >= 35)
    {
        static const float arenaX = -13180.0f;
        static const float arenaY = 335.0f;
        static const float arenaZ = 35.0f;

        float dist = bot->GetDistance(arenaX, arenaY, arenaZ);
        if (dist > 15.0f && dist < 400.0f)
        {
            // Path into Gurubashi Arena floor
            bot->GetMotionMaster()->MovePoint(0, arenaX, arenaY, arenaZ);
            return true;
        }
        else if (dist <= 15.0f)
        {
            // Inside Arena floor: combat stance / emote
            if (urand(0, 100) < 10)
            {
                bot->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
            }
            return true;
        }
    }

    return false;
}
