#include "botpch.h"
#include "../../playerbot.h"
#include "BGTacticsAction.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;

bool BGTacticsAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || !bot->InBattleGround())
        return false;

    BattleGround* bg = bot->GetBattleGround();
    if (!bg || bg->GetStatus() != STATUS_IN_PROGRESS)
        return false;

    BattleGroundTypeId bgType = bg->GetTypeID();
    Team botTeam = bot->GetTeam();

    // Human-like formation spread jitter (no single-line marching)
    uint32 guidSeed = bot->GetGUIDLow();
    float jitterX = ((float)(guidSeed % 11) - 5.0f) * 1.5f;
    float jitterY = ((float)((guidSeed / 11) % 11) - 5.0f) * 1.5f;

    // 1. WARSONG GULCH (WSG - Map 489)
    if (bgType == BATTLEGROUND_WS)
    {
        bool carriesFlag = bot->HasAura(23333) || bot->HasAura(23335);

        // Check for nearby dropped flag or flag stand gameobject within 12m to pick up/return
        list<GameObject*> flags;
        MaNGOS::GameObjectLastSearcher<MaNGOS::WorldObjectLastSearcher> searcher(bot, flags, 12.0f);
        Cell::VisitAllObjects((const WorldObject*)bot, searcher, 12.0f);
        for (list<GameObject*>::iterator it = flags.begin(); it != flags.end(); ++it)
        {
            GameObject* go = *it;
            if (go && (go->GetEntry() == 179785 || go->GetEntry() == 179786 || go->GetEntry() == 179830 || go->GetEntry() == 179831))
            {
                if (bot->GetDistance(go) > 2.0f && !bot->IsBeingTeleported())
                {
                    bot->GetMotionMaster()->MovePoint(0, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
                    return true;
                }
            }
        }

        if (carriesFlag)
        {
            // Path back to home flag room with squad offset
            float homeX = (botTeam == ALLIANCE) ? (1010.0f + jitterX * 0.5f) : (920.0f + jitterX * 0.5f);
            float homeY = (botTeam == ALLIANCE) ? (1400.0f + jitterY * 0.5f) : (1430.0f + jitterY * 0.5f);
            float homeZ = 345.0f;

            if (bot->GetDistance2d(homeX, homeY) > 4.0f && !bot->IsBeingTeleported())
            {
                bot->GetMotionMaster()->MovePoint(0, homeX, homeY, homeZ);
                return true;
            }
        }
        else
        {
            // 50% attack enemy flag room, 50% mid-field control / escort
            if (guidSeed % 2 == 0)
            {
                float enemyFlagX = (botTeam == ALLIANCE) ? (920.0f + jitterX) : (1010.0f + jitterX);
                float enemyFlagY = (botTeam == ALLIANCE) ? (1430.0f + jitterY) : (1400.0f + jitterY);
                float enemyFlagZ = 345.0f;

                if (bot->GetDistance2d(enemyFlagX, enemyFlagY) > 5.0f && !bot->IsBeingTeleported())
                {
                    bot->GetMotionMaster()->MovePoint(0, enemyFlagX, enemyFlagY, enemyFlagZ);
                    return true;
                }
            }
            else
            {
                // Midfield / Escort patrol
                float midX = 965.0f + jitterX * 2.0f;
                float midY = 1415.0f + jitterY * 2.0f;
                float midZ = 340.0f;

                if (bot->GetDistance2d(midX, midY) > 8.0f && !bot->IsBeingTeleported())
                {
                    bot->GetMotionMaster()->MovePoint(0, midX, midY, midZ);
                    return true;
                }
            }
        }
    }
    // 2. ARATHI BASIN (AB - Map 529)
    else if (bgType == BATTLEGROUND_AB)
    {
        static const struct ABNode { const char* name; float x, y, z; } abNodes[] = {
            {"Stables", -1198.0f, 1190.0f, 12.0f},
            {"Gold Mine", -880.0f, 830.0f, -9.0f},
            {"Blacksmith", -1025.0f, 960.0f, 6.0f},
            {"Lumber Mill", -1120.0f, 780.0f, 48.0f},
            {"Farm", -870.0f, 570.0f, 10.0f}
        };

        // Pick dynamic node assignment with position spread
        uint32 nodeIdx = (guidSeed + (uint32)(bot->GetMapId() & 0xFF)) % 5;
        ABNode const& targetNode = abNodes[nodeIdx];

        float destX = targetNode.x + jitterX;
        float destY = targetNode.y + jitterY;
        float destZ = targetNode.z;

        if (bot->GetDistance2d(destX, destY) > 8.0f && !bot->IsBeingTeleported())
        {
            bot->GetMotionMaster()->MovePoint(0, destX, destY, destZ);
            return true;
        }
    }
    // 3. ALTERAC VALLEY (AV - Map 30)
    else if (bgType == BATTLEGROUND_AV)
    {
        // Strategic progression path: Graveyards / Towers / Boss Keep
        float targetX = (botTeam == ALLIANCE) ? (-1300.0f + jitterX * 2.0f) : (800.0f + jitterX * 2.0f);
        float targetY = (botTeam == ALLIANCE) ? (-500.0f + jitterY * 2.0f) : (-500.0f + jitterY * 2.0f);
        float targetZ = (botTeam == ALLIANCE) ? 90.0f : 50.0f;

        if (bot->GetDistance2d(targetX, targetY) > 12.0f && !bot->IsBeingTeleported())
        {
            bot->GetMotionMaster()->MovePoint(0, targetX, targetY, targetZ);
            return true;
        }
    }

    return false;
}
