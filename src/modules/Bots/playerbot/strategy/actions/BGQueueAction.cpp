#include "botpch.h"
#include "../../playerbot.h"
#include "BGQueueAction.h"
#include "BattleGround/BattleGroundMgr.h"

using namespace ai;

bool BGQueueAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat() || bot->IsDead())
        return false;

    if (bot->InBattleGround())
        return false;

    static const BattleGroundTypeId bgTypes[] = { BATTLEGROUND_WS, BATTLEGROUND_AB, BATTLEGROUND_AV };
    for (int t = 0; t < 3; ++t)
    {
        BattleGroundTypeId bgTypeId = bgTypes[t];
        BattleGroundQueueTypeId bgQueueTypeId = sBattleGroundMgr.BGQueueTypeId(bgTypeId);
        if (bot->IsInvitedForBattleGroundQueueType(bgQueueTypeId))
        {
            uint32 mapId = GetBattleGrounMapIdByTypeId(bgTypeId);
            uint8 action = 1;
            WorldPacket p;
            p << mapId << action;
            bot->GetSession()->HandleBattleFieldPortOpcode(p);
            return true;
        }
    }

    if (bot->InBattleGroundQueue())
        return false;

    uint32 level = bot->getLevel();
    if (level < 10)
        return false;

    BattleGroundTypeId bgTypeId = BATTLEGROUND_WS;
    if (level >= 51 && urand(0, 1) == 0)
        bgTypeId = BATTLEGROUND_AV;
    else if (level >= 20 && urand(0, 1) == 0)
        bgTypeId = BATTLEGROUND_AB;

    BattleGroundQueueTypeId bgQueueTypeId = sBattleGroundMgr.BGQueueTypeId(bgTypeId);
    BattleGroundBracketId bgBracketId = bot->GetBattleGroundBracketIdFromLevel(bgTypeId);
    BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];

    bgQueue.AddGroup(bot, NULL, bgTypeId, bgBracketId, false);
    bot->AddBattleGroundQueueId(bgQueueTypeId);
    bot->SetBattleGroundEntryPoint();
    sBattleGroundMgr.ScheduleQueueUpdate(bgQueueTypeId, bgTypeId, bgBracketId);

    sLog.outString("BGQueueAction: Bot %s (Level %u) queued for Battleground %u", bot->GetName(), level, bgTypeId);
    return true;
}
