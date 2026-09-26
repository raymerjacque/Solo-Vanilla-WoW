#include "botpch.h"
#include "../../playerbot.h"
#include "EscortAction.h"
#include "MotionGenerators/MotionMaster.h"

using namespace ai;

bool EscortAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead())
        return false;

    list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
    Creature* escortTarget = NULL;

    for (list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); ++i)
    {
        Creature* unit = bot->GetNPCIfCanInteractWith(*i, UNIT_NPC_FLAG_QUESTGIVER);
        if (unit)
        {
            MovementGeneratorType mType = unit->GetMotionMaster()->GetCurrentMovementGeneratorType();
            if (mType == WAYPOINT_MOTION_TYPE || mType == POINT_MOTION_TYPE)
            {
                escortTarget = unit;
                break;
            }
        }
    }

    if (!escortTarget)
        return false;

    if (escortTarget->IsInCombat())
    {
        Unit* attacker = escortTarget->getVictim();
        if (!attacker)
            attacker = escortTarget->getAttackerForHelper();

        if (attacker)
        {
            bot->Attack(attacker, true);
            sLog.outString("EscortAction: Bot %s defending escort NPC %s against %s",
                           bot->GetName(), escortTarget->GetName(), attacker->GetName());
            return true;
        }
    }

    if (bot->GetDistance(escortTarget) > 10.0f && !bot->IsInCombat())
    {
        float x = escortTarget->GetPositionX() - 3.0f;
        float y = escortTarget->GetPositionY() - 3.0f;
        float z = escortTarget->GetPositionZ();
        bot->GetMotionMaster()->MovePoint(0, x, y, z);
        return true;
    }

    return false;
}

