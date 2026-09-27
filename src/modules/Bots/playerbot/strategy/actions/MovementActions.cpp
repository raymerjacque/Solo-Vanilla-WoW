#include "botpch.h"
#include "../../playerbot.h"
#include "../values/LastMovementValue.h"
#include "MovementActions.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "../../FleeManager.h"
#include "../../LootObjectStack.h"
#include "../../PlayerbotAIConfig.h"

using namespace ai;

bool MovementAction::MoveNear(uint32 mapId, float x, float y, float z, float distance)
{
    float angle = GetFollowAngle();
    return MoveTo(mapId, x + cos(angle) * distance, y + sin(angle) * distance, z);
}

bool MovementAction::MoveNear(WorldObject* target, float distance)
{
    if (!target)
    {
        return false;
    }

    distance += target->GetObjectBoundingRadius();

    float followAngle = GetFollowAngle();
    for (float angle = followAngle - M_PI; angle <= followAngle + M_PI; angle += M_PI / 4)
    {
        bool moved = MoveTo(target->GetMapId(),
            target->GetPositionX() + cos(angle) * distance,
            target->GetPositionY()+ sin(angle) * distance,
            target->GetPositionZ());
        if (moved)
        {
            return true;
        }
    }
    return false;
}

bool MovementAction::MoveTo(uint32 mapId, float x, float y, float z)
{
    bot->UpdateGroundPositionZ(x, y, z);
    if (!IsMovingAllowed(mapId, x, y, z))
    {
        return false;
    }

    float distance = bot->GetDistance(x, y, z);
    if (distance > sPlayerbotAIConfig.contactDistance)
    {
        WaitForReach(distance);

        if (bot->IsSitState())
        {
            bot->SetStandState(UNIT_STAND_STATE_STAND);
        }

        if (bot->IsNonMeleeSpellCasted(true))
        {
            bot->CastStop();
            ai->InterruptSpell();
        }

        MotionMaster &mm = *bot->GetMotionMaster();
        mm.Clear();

        float botZ = bot->GetPositionZ();
            mm.MovePoint(mapId, x, y, z);
    }

    AI_VALUE(LastMovement&, "last movement").Set(x, y, z, bot->GetOrientation());
    return true;
}

bool MovementAction::MoveTo(Unit* target, float distance)
{
    if (!IsMovingAllowed(target))
    {
        return false;
    }

    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();
    float bz = bot->GetPositionZ();

    float tx = target->GetPositionX();
    float ty = target->GetPositionY();
    float tz = target->GetPositionZ();

    float distanceToTarget = bot->GetDistance(target);
    float angle = bot->GetAngle(target);
    float needToGo = distanceToTarget - distance;

    float maxDistance = sPlayerbotAIConfig.spellDistance;
    if (needToGo > 0 && needToGo > maxDistance)
    {
        needToGo = maxDistance;
    }
    else if (needToGo < 0 && needToGo < -maxDistance)
    {
        needToGo = -maxDistance;
    }

    float dx = cos(angle) * needToGo + bx;
    float dy = sin(angle) * needToGo + by;

    return MoveTo(target->GetMapId(), dx, dy, tz);
}

float MovementAction::GetFollowAngle()
{
    Player* master = GetMaster();
    Group* group = master ? master->GetGroup() : bot->GetGroup();
    uint32 lowGuid = bot->GetObjectGuid().GetCounter();
    float guidJitter = ((float)(lowGuid % 60) - 30.0f) * (M_PI / 180.0f);

    if (!group)
    {
        return (float)(lowGuid % 360) * (M_PI / 180.0f);
    }

    uint32 totalMembers = group->GetMembersCount();
    if (totalMembers <= 1)
        totalMembers = 2;

    int index = 1;
    for (GroupReference *ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        if (master && ref->getSource() == master)
        {
            continue;
        }

        if (ref->getSource() == bot)
        {
            return (2 * M_PI / (totalMembers - 1)) * index + guidJitter;
        }

        index++;
    }
    return (float)(lowGuid % 360) * (M_PI / 180.0f);
}

bool MovementAction::IsMovingAllowed(Unit* target)
{
    if (!target)
    {
        return false;
    }

    if (bot->GetMapId() != target->GetMapId())
    {
        return false;
    }

    float distance = bot->GetDistance(target);
    if (distance > sPlayerbotAIConfig.reactDistance)
    {
        return false;
    }

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed(uint32 mapId, float x, float y, float z)
{
    float distance = bot->GetDistance(x, y, z);
    if (distance > sPlayerbotAIConfig.reactDistance)
    {
        return false;
    }

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed()
{
    if (bot->IsFrozen() || bot->IsPolymorphed() ||
            (bot->IsDead() && !bot->GetCorpse()) ||
            bot->IsBeingTeleported() ||
            bot->IsInRoots() ||
            bot->HasAuraType(SPELL_AURA_MOD_CONFUSE) || bot->IsCharmed() ||
            bot->HasAuraType(SPELL_AURA_MOD_STUN) || bot->IsTaxiFlying())
        return false;

    MotionMaster &mm = *bot->GetMotionMaster();
    return mm.GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE;
}

bool MovementAction::Follow(Unit* target, float distance)
{
    return Follow(target, distance, GetFollowAngle());
}

bool MovementAction::Follow(Unit* target, float distance, float angle)
{
    MotionMaster &mm = *bot->GetMotionMaster();

    if (!target)
    {
        return false;
    }

    if (bot->GetDistance2d(target->GetPositionX(), target->GetPositionY()) <= sPlayerbotAIConfig.sightDistance &&
            abs(bot->GetPositionZ() - target->GetPositionZ()) >= sPlayerbotAIConfig.spellDistance)
    {
        mm.Clear();
        float x = bot->GetPositionX(), y = bot->GetPositionY(), z = target->GetPositionZ();
        if (target->GetMapId() && bot->GetMapId() != target->GetMapId())
        {
            bot->TeleportTo(target->GetMapId(), x, y, z, bot->GetOrientation());
        }
        else
        {
            bot->SetPosition(x, y, z, bot->GetOrientation(), true);
        }
        AI_VALUE(LastMovement&, "last movement").Set(target);
        return true;
    }

    if (!IsMovingAllowed(target))
    {
        return false;
    }

    if (target->IsFriendlyTo(bot) && bot->IsMounted() && AI_VALUE(list<ObjectGuid>, "possible targets").empty())
    {
        distance += angle;
    }

    uint32 lowGuid = bot->GetObjectGuid().GetCounter();
    float botDistOffset = (float)(lowGuid % 5) * 0.7f + 1.0f;
    float botDistance = distance + botDistOffset;

    if (bot->GetDistance(target) <= botDistance * 0.7f)
    {
        return false;
    }

    if (bot->IsSitState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
    }

    if (bot->IsNonMeleeSpellCasted(true))
    {
        bot->CastStop();
        ai->InterruptSpell();
    }

    mm.MoveFollow(target, botDistance, angle);

    AI_VALUE(LastMovement&, "last movement").Set(target);
    return true;
}

void MovementAction::WaitForReach(float distance)
{
    float delay = 1000.0f * distance / bot->GetSpeed(MOVE_RUN) + sPlayerbotAIConfig.reactDelay;

    if (delay > sPlayerbotAIConfig.maxWaitForMove)
    {
        delay = sPlayerbotAIConfig.maxWaitForMove;
    }

    Unit* target = *ai->GetAiObjectContext()->GetValue<Unit*>("current target");
    Unit* player = *ai->GetAiObjectContext()->GetValue<Unit*>("enemy player target");
    if ((player || target) && delay > sPlayerbotAIConfig.globalCoolDown)
    {
        delay = sPlayerbotAIConfig.globalCoolDown;
    }

    ai->SetNextCheckDelay((uint32)delay);
}

bool MovementAction::Flee(Unit *target)
{
    Player* master = GetMaster();
    if (!target)
    {
        target = master;
    }

    if (!target)
    {
        return false;
    }

    if (!sPlayerbotAIConfig.fleeingEnabled)
    {
        return false;
    }

    if (!IsMovingAllowed())
    {
        return false;
    }

    FleeManager manager(bot, sPlayerbotAIConfig.fleeDistance, GetFollowAngle());

    float rx, ry, rz;
    if (!manager.CalculateDestination(&rx, &ry, &rz))
    {
        return false;
    }

    return MoveTo(target->GetMapId(), rx, ry, rz);
}

bool FleeAction::Execute(Event event)
{
    return Flee(AI_VALUE(Unit*, "current target"));
}

bool FleeAction::isUseful()
{
    return AI_VALUE(uint8, "attacker count") > 0 &&
            AI_VALUE2(float, "distance", "current target") <= sPlayerbotAIConfig.tooCloseDistance;
}

bool RunAwayAction::Execute(Event event)
{
    return Flee(AI_VALUE(Unit*, "master target"));
}

bool MoveRandomAction::Execute(Event event)
{
    WorldObject* target = NULL;

    if (!(rand() % 3))
    {
        list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
        for (list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); i++)
        {
            target = ai->GetUnit(*i);

            if (target && bot->GetDistance(target) > sPlayerbotAIConfig.tooCloseDistance)
            {
                break;
            }
        }
    }

    if (!target || !(rand() % 3))
    {
        list<ObjectGuid> gos = AI_VALUE(list<ObjectGuid>, "nearest game objects");
        for (list<ObjectGuid>::iterator i = gos.begin(); i != gos.end(); i++)
        {
            GameObject* go = ai->GetGameObject(*i);
            if (go && bot->GetDistance(go) > sPlayerbotAIConfig.tooCloseDistance)
            {
                LootObject loot(bot, *i);
                if (!loot.IsEmpty() && !loot.IsLootPossible(bot))
                {
                    continue;
                }
                target = go;
                break;
            }
        }
    }

    float distance = sPlayerbotAIConfig.tooCloseDistance + sPlayerbotAIConfig.grindDistance * urand(3, 10) / 10.0f;

    if (target)
    {
        return MoveNear(target);
    }

    for (int i = 0; i < 10; ++i)
    {
        float angle = (float)urand(0, 360) * M_PI / 180.0f;
        float dist = 20.0f + urand(0, (uint32)distance);
        float x = bot->GetPositionX() + cos(angle) * dist;
        float y = bot->GetPositionY() + sin(angle) * dist;
        float z = bot->GetPositionZ();
        bot->UpdateGroundPositionZ(x, y, z);

        bool moved = MoveNear(bot->GetMapId(), x, y, z);
        if (moved)
        {
            return true;
        }
    }

    return false;
}

bool MoveToLootAction::Execute(Event event)
{
    LootObject loot = AI_VALUE(LootObject, "loot target");
    if (!loot.IsLootPossible(bot))
    {
        return false;
    }

    return MoveNear(loot.GetWorldObject(bot));
}

bool MoveOutOfEnemyContactAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }

    return MoveNear(target, sPlayerbotAIConfig.meleeDistance);
}

bool MoveOutOfEnemyContactAction::isUseful()
{
    return AI_VALUE2(float, "distance", "current target") <= sPlayerbotAIConfig.contactDistance;
}

bool SetFacingTargetAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }

    bot->SetFacingTo(bot->GetAngle(target));
    ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    return true;
}

bool SetFacingTargetAction::isUseful()
{
    return !AI_VALUE2(bool, "facing", "current target");
}
