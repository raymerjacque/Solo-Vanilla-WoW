#include "botpch.h"
#include "../../playerbot.h"
#include "../../AiFactory.h"
#include "DungeonTacticsAction.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;

bool DungeonTacticsAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead())
        return false;

    // Global Dungeon/Group Rule: Tank checks Healer mana before pulling
    Group* group = bot->GetGroup();
    if (group && ai->IsTank(bot) && !bot->IsInCombat())
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

    // Druid Group Role Adaptation Rule: Only enforce in dungeons/instances
    if (group && bot->getClass() == CLASS_DRUID && !bot->IsInCombat() && bot->GetMap()->IsDungeon())
    {
        bool hasOtherTank = false;
        bool hasOtherHealer = false;
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (member && member != bot && member->IsInWorld() && member->IsAlive())
            {
                if (ai->IsTank(member))
                    hasOtherTank = true;
                if (ai->IsHeal(member))
                    hasOtherHealer = true;
            }
        }

        if (!hasOtherTank)
        {
            if (!ai->HasStrategy("bear", BOT_STATE_COMBAT))
                ai->ChangeStrategy("+bear,-cat,-caster,-heal", BOT_STATE_COMBAT);
        }
        else if (!hasOtherHealer)
        {
            if (!ai->HasStrategy("heal", BOT_STATE_COMBAT))
                ai->ChangeStrategy("+heal,-bear,-cat,-caster", BOT_STATE_COMBAT);
        }
        else
        {
            int spec = AiFactory::GetPlayerSpecTab(bot);
            if (spec == 1) // Feral spec
            {
                if (!ai->HasStrategy("cat", BOT_STATE_COMBAT))
                    ai->ChangeStrategy("+cat,-bear,-caster,-heal", BOT_STATE_COMBAT);
            }
            else
            {
                if (!ai->HasStrategy("caster", BOT_STATE_COMBAT))
                    ai->ChangeStrategy("+caster,-bear,-cat,-heal", BOT_STATE_COMBAT);
            }
        }
    }

    // Warlock Soulstone on Healer Rule:
    if (group && bot->getClass() == CLASS_WARLOCK && !bot->IsInCombat() && !bot->IsNonMeleeSpellCasted(false))
    {
        Player* healer = NULL;
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (member && member->IsInWorld() && member->IsAlive() && ai->IsHeal(member))
            {
                if (!member->HasAura(20707) && !member->HasAura(20740))
                {
                    healer = member;
                    break;
                }
            }
        }

        if (healer)
        {
            static uint32 ssItems[] = { 16896, 16895, 16893, 16892, 5232 };
            Item* ssItem = NULL;
            for (int i = 0; i < 5; ++i)
            {
                if (bot->HasItemCount(ssItems[i], 1))
                {
                    ssItem = bot->GetItemByEntry(ssItems[i]);
                    break;
                }
            }

            if (ssItem)
            {
                ItemPrototype const* proto = ssItem->GetProto();
                if (proto && proto->Spells[0].SpellId)
                {
                    bot->CastSpell(healer, proto->Spells[0].SpellId, true);
                    bot->DestroyItemCount(ssItem->GetEntry(), 1, true);
                    string msg = "Placed Soulstone on " + string(healer->GetName()) + "!";
                    bot->Say(msg, LANG_UNIVERSAL);
                    return true;
                }
            }
            else
            {
                uint32 conjureSpell = 0;
                if (bot->getLevel() >= 60 && bot->HasSpell(20757)) conjureSpell = 20757;
                else if (bot->getLevel() >= 50 && bot->HasSpell(20756)) conjureSpell = 20756;
                else if (bot->getLevel() >= 40 && bot->HasSpell(20755)) conjureSpell = 20755;
                else if (bot->getLevel() >= 30 && bot->HasSpell(20752)) conjureSpell = 20752;
                else if (bot->getLevel() >= 18 && bot->HasSpell(693)) conjureSpell = 693;

                if (conjureSpell)
                {
                    bot->CastSpell(bot, conjureSpell, false);
                    return true;
                }
            }
        }
    }

    if (!bot->GetMap()->IsDungeon())
        return false;

    // Mage End-of-Dungeon Portal Rule:
    if (group && bot->getClass() == CLASS_MAGE && bot->getLevel() >= 40 && !bot->IsInCombat())
    {
        bool bossDeadNearby = false;
        ObjectGuid selGuid = bot->GetSelectionGuid();
        if (selGuid)
        {
            Unit* selected = ai->GetUnit(selGuid);
            if (selected && selected->IsDead() && selected->GetTypeId() == TYPEID_UNIT)
            {
                Creature* c = (Creature*)selected;
                if (c->GetCreatureInfo() && c->GetCreatureInfo()->Rank >= 1)
                    bossDeadNearby = true;
            }
        }

        if (!bossDeadNearby)
        {
            list<ObjectGuid> targets = AI_VALUE(list<ObjectGuid>, "possible targets");
            for (list<ObjectGuid>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
            {
                Unit* u = ai->GetUnit(*itr);
                if (u && u->IsDead() && u->GetTypeId() == TYPEID_UNIT)
                {
                    Creature* c = (Creature*)u;
                    if (c->GetCreatureInfo() && c->GetCreatureInfo()->Rank >= 1)
                    {
                        bossDeadNearby = true;
                        break;
                    }
                }
            }
        }

        if (bossDeadNearby)
        {
            // Party Victory Reactions: Cheer, dance, emote, chat
            static const char* victoryMsgs[] = { "gg!", "Great kill everyone!", "nice pull!", "ty for group!", "Awesome run!" };
            int msgIdx = urand(0, 4);
            bot->Say(victoryMsgs[msgIdx], LANG_UNIVERSAL);
            uint32 emoteId = (urand(0, 2) == 0) ? 1 : ((urand(0, 1) == 0) ? 10 : 66); // Cheer (1), Dance (10), Salute (66)
            bot->HandleEmoteCommand(emoteId);

            bool portalActive = false;
            list<ObjectGuid> gos = AI_VALUE(list<ObjectGuid>, "nearest game objects");
            for (list<ObjectGuid>::iterator i = gos.begin(); i != gos.end(); ++i)
            {
                GameObject* go = ai->GetGameObject(*i);
                if (go && go->GetGoType() == GAMEOBJECT_TYPE_SPELLCASTER)
                {
                    portalActive = true;
                    break;
                }
            }

            if (!portalActive && !bot->IsNonMeleeSpellCasted(false))
            {
                uint32 portalSpell = (bot->GetTeam() == ALLIANCE) ? 11416 : 11419;
                if (bot->HasSpell(portalSpell))
                {
                    bot->CastSpell(bot, portalSpell, false);
                    bot->Say("Opening a portal back to the city!", LANG_UNIVERSAL);
                    return true;
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
