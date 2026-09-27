#include "botpch.h"
#include "../../playerbot.h"
#include "DungeonTacticsAction.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;

bool DungeonTacticsAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || !bot->GetMap()->IsDungeon())
        return false;

    // Global Dungeon Rule: Tank checks Healer mana before pulling
    Group* group = bot->GetGroup();
    if (group && ai->IsTank() && !bot->IsInCombat())
    {
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (member && member->IsInWorld() && member->IsAlive() && member->GetPowerType() == POWER_MANA)
            {
                uint8 cls = member->getClass();
                if (cls == CLASS_PRIEST || cls == CLASS_PALADIN || cls == CLASS_SHAMAN || cls == CLASS_DRUID)
                {
                    uint32 maxMana = member->GetMaxPower(POWER_MANA);
                    if (maxMana > 0)
                    {
                        uint32 curMana = member->GetPower(POWER_MANA);
                        if ((curMana * 100) / maxMana < 35)
                        {
                            // Tank waits for healer to drink/regen mana before starting next pull
                            bot->GetMotionMaster()->Clear();
                            return true;
                        }
                    }
                }
            }
        }
    }

    uint32 mapId = bot->GetMapId();

    // 1. BLACKROCK DEPTHS (BRD - Map 230)
    if (mapId == 230)
    {
        // A. Lyceum Torch Lighting
        if (bot->GetDistance2d(290.0f, -230.0f) < 60.0f)
        {
            // Find Shadowforge Brazier nearby
            list<GameObject*> braziers;
            MaNGOS::GameObjectEntryInPosRangeCheck check1(*bot, 174745, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 15.0f);
            MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> searcher1(braziers, check1);
            Cell::VisitAllObjects((const WorldObject*)bot, searcher1, 15.0f);

            if (braziers.empty())
            {
                MaNGOS::GameObjectEntryInPosRangeCheck check2(*bot, 174744, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 15.0f);
                MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> searcher2(braziers, check2);
                Cell::VisitAllObjects((const WorldObject*)bot, searcher2, 15.0f);
            }

            if (!braziers.empty() && bot->HasItemCount(11293, 1))
            {
                GameObject* brazier = *braziers.begin();
                if (bot->GetDistance(brazier) > 3.0f && !bot->IsBeingTeleported())
                {
                    bot->GetMotionMaster()->MovePoint(0, brazier->GetPositionX(), brazier->GetPositionY(), brazier->GetPositionZ());
                    return true;
                }
            }
        }
        // B. Grim Guzzler Tavern (around X: 880, Y: -70, Z: -45)
        else if (bot->GetDistance2d(880.0f, -70.0f) < 40.0f)
        {
            ObjectGuid targetGuid = bot->GetSelectionGuid();
            if (targetGuid)
            {
                Unit* target = bot->GetMap()->GetUnit(targetGuid);
                if (target && target->IsNeutralToAll() && !target->IsInCombat())
                {
                    bot->SetSelectionGuid(ObjectGuid());
                    return true;
                }
            }
        }
    }
    // 2. STRATHOLME (Map 329) - 45 Minute Baron Rivendare Speedrun
    else if (mapId == 329)
    {
        if (bot->GetPositionX() < -3500.0f)
        {
            if (bot->GetDistance2d(-4020.0f, 3335.0f) < 100.0f)
            {
                if (bot->GetDistance2d(-4020.0f, 3335.0f) > 5.0f && !bot->IsBeingTeleported())
                {
                    bot->GetMotionMaster()->MovePoint(0, -4020.0f, 3335.0f, 115.0f);
                    return true;
                }
            }
        }
    }
    // 3. SCHOLOMANCE (Map 289) - Key Doors
    else if (mapId == 289)
    {
        list<GameObject*> doors;
        MaNGOS::GameObjectEntryInPosRangeCheck check(*bot, 176662, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 10.0f);
        MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> searcher(doors, check);
        Cell::VisitAllObjects((const WorldObject*)bot, searcher, 10.0f);

        if (!doors.empty())
        {
            GameObject* door = *doors.begin();
            if (door && door->GetGoState() == GO_STATE_READY && bot->GetDistance(door) > 3.0f && !bot->IsBeingTeleported())
            {
                bot->GetMotionMaster()->MovePoint(0, door->GetPositionX(), door->GetPositionY(), door->GetPositionZ());
                return true;
            }
        }
    }
    // 4. UPPER BLACKROCK SPIRE (UBRS - Map 229) - Rend Blackhand Arena Waves
    else if (mapId == 229)
    {
        if (bot->GetDistance2d(165.0f, -430.0f) < 40.0f)
        {
            if (bot->GetDistance2d(165.0f, -430.0f) > 8.0f && !bot->IsInCombat() && !bot->IsBeingTeleported())
            {
                bot->GetMotionMaster()->MovePoint(0, 165.0f, -430.0f, 110.0f);
                return true;
            }
        }
    }

    return false;
}
