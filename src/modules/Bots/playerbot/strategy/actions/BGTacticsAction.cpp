#include "botpch.h"
#include "../../playerbot.h"
#include "BGTacticsAction.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"

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

    // 1. WARSONG GULCH (WSG)
    if (bgType == BATTLEGROUND_WS)
    {
        // Check if carrying Silverwing (23333) or Warsong (23335) Flag
        bool carriesFlag = bot->HasAura(23333) || bot->HasAura(23335);
        if (carriesFlag)
        {
            // Path back to home flag room to score point
            float homeX = (botTeam == ALLIANCE) ? 1010.0f : 920.0f;
            float homeY = (botTeam == ALLIANCE) ? 1400.0f : 1430.0f;
            float homeZ = 345.0f;

            if (bot->GetDistance2d(homeX, homeY) > 5.0f && !bot->IsBeingTeleported())
            {
                bot->GetMotionMaster()->MovePoint(0, homeX, homeY, homeZ);
                return true;
            }
        }
        else
        {
            // 50% attack enemy flag room, 50% mid-field control
            if (bot->GetGUIDLow() % 2 == 0)
            {
                float enemyFlagX = (botTeam == ALLIANCE) ? 920.0f : 1010.0f;
                float enemyFlagY = (botTeam == ALLIANCE) ? 1430.0f : 1400.0f;
                float enemyFlagZ = 345.0f;

                if (bot->GetDistance2d(enemyFlagX, enemyFlagY) > 5.0f && !bot->IsBeingTeleported())
                {
                    bot->GetMotionMaster()->MovePoint(0, enemyFlagX, enemyFlagY, enemyFlagZ);
                    return true;
                }
            }
        }
    }
    // 2. ARATHI BASIN (AB)
    else if (bgType == BATTLEGROUND_AB)
    {
        static const struct ABNode { const char* name; float x, y, z; } abNodes[] = {
            {"Stables", -1198.0f, 1190.0f, 12.0f},
            {"Gold Mine", -880.0f, 830.0f, -9.0f},
            {"Blacksmith", -1025.0f, 960.0f, 6.0f},
            {"Lumber Mill", -1120.0f, 780.0f, 48.0f},
            {"Farm", -870.0f, 570.0f, 10.0f}
        };

        // Pick node based on bot GUID assignment
        uint32 nodeIdx = bot->GetGUIDLow() % 5;
        ABNode const& targetNode = abNodes[nodeIdx];

        if (bot->GetDistance2d(targetNode.x, targetNode.y) > 10.0f && !bot->IsBeingTeleported())
        {
            bot->GetMotionMaster()->MovePoint(0, targetNode.x, targetNode.y, targetNode.z);
            return true;
        }
    }
    // 3. ALTERAC VALLEY (AV)
    else if (bgType == BATTLEGROUND_AV)
    {
        // Alliance attacks Frostwolf Keep (-1300, -500), Horde attacks Dun Baldar (800, -500)
        float targetX = (botTeam == ALLIANCE) ? -1300.0f : 800.0f;
        float targetY = (botTeam == ALLIANCE) ? -500.0f : -500.0f;
        float targetZ = (botTeam == ALLIANCE) ? 90.0f : 50.0f;

        if (bot->GetDistance2d(targetX, targetY) > 15.0f && !bot->IsBeingTeleported())
        {
            bot->GetMotionMaster()->MovePoint(0, targetX, targetY, targetZ);
            return true;
        }
    }

    return false;
}
