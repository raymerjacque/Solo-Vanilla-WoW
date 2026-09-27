#pragma once

#include "../actions/GenericActions.h"

namespace ai
{
    class CastDemonSkinAction : public CastBuffSpellAction {
    public:
        CastDemonSkinAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "demon skin") {}
    };

    class CastDemonArmorAction : public CastBuffSpellAction
    {
    public:
        CastDemonArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "demon armor") {}
    };

    BEGIN_RANGED_SPELL_ACTION(CastShadowBoltAction, "shadow bolt")
    END_SPELL_ACTION()

    class CastDrainSoulAction : public CastSpellAction
    {
    public:
        CastDrainSoulAction(PlayerbotAI* ai) : CastSpellAction(ai, "drain soul") {}
        virtual bool isUseful()
        {
            return AI_VALUE2(uint8, "item count", "soul shard") < 2;
        }
    };

    class CastDrainManaAction : public CastSpellAction
    {
    public:
        CastDrainManaAction(PlayerbotAI* ai) : CastSpellAction(ai, "drain mana") {}
    };

    class CastDrainLifeAction : public CastSpellAction
    {
    public:
        CastDrainLifeAction(PlayerbotAI* ai) : CastSpellAction(ai, "drain life") {}
    };

    class CastCurseOfAgonyAction : public CastDebuffSpellAction
    {
    public:
        CastCurseOfAgonyAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "curse of agony") {}
    };

    class CastCurseOfWeaknessAction : public CastDebuffSpellAction
    {
    public:
        CastCurseOfWeaknessAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "curse of weakness") {}
    };

    class CastCorruptionAction : public CastDebuffSpellAction
    {
    public:
        CastCorruptionAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "corruption") {}
    };

    class CastCorruptionOnAttackerAction : public CastDebuffSpellOnAttackerAction
    {
    public:
        CastCorruptionOnAttackerAction(PlayerbotAI* ai) : CastDebuffSpellOnAttackerAction(ai, "corruption") {}
    };


    class CastSummonVoidwalkerAction : public CastBuffSpellAction
    {
    public:
        CastSummonVoidwalkerAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon voidwalker") {}
        virtual bool isUseful()
        {
            if (!CastBuffSpellAction::isUseful())
                return false;

            Group* group = bot->GetGroup();
            if (!group || group->GetMembersCount() <= 1)
                return true; // Solo -> Voidwalker is useful!

            Group::MemberSlotList const& slots = group->GetMemberSlots();
            for (Group::member_citerator itr = slots.begin(); itr != slots.end(); ++itr)
            {
                Player* member = sObjectMgr.GetPlayer(itr->guid);
                if (member && member != bot && member->IsInWorld() && member->IsAlive() && ai->IsTank(member))
                    return false; // Group has a tank -> prefer Imp
            }
            return true; // Group has no tank -> Voidwalker useful to tank
        }
    };

    class CastSummonImpAction : public CastBuffSpellAction
    {
    public:
        CastSummonImpAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon imp") {}
        virtual bool isUseful()
        {
            if (!CastBuffSpellAction::isUseful())
                return false;

            if (ai->HasSpell(697)) // Summon Voidwalker
            {
                Group* group = bot->GetGroup();
                if (!group || group->GetMembersCount() <= 1)
                    return false; // Solo -> prefer Voidwalker

                bool hasOtherTank = false;
                Group::MemberSlotList const& slots = group->GetMemberSlots();
                for (Group::member_citerator itr = slots.begin(); itr != slots.end(); ++itr)
                {
                    Player* member = sObjectMgr.GetPlayer(itr->guid);
                    if (member && member != bot && member->IsInWorld() && member->IsAlive() && ai->IsTank(member))
                    {
                        hasOtherTank = true;
                        break;
                    }
                }
                if (!hasOtherTank)
                    return false; // No tank -> prefer Voidwalker
            }
            return true;
        }
    };

    class CastSummonSuccubusAction : public CastBuffSpellAction
    {
    public:
        CastSummonSuccubusAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon succubus") {}
    };

    class CastSummonFelhunterAction : public CastBuffSpellAction
    {
    public:
        CastSummonFelhunterAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon felhunter") {}
    };

    class CastSummonInfernalAction : public CastBuffSpellAction
    {
    public:
        CastSummonInfernalAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon infernal") {}
    };

    class CastCreateHealthstoneAction : public CastBuffSpellAction
    {
    public:
        CastCreateHealthstoneAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "create healthstone") {}
    };

    class CastCreateFirestoneAction : public CastBuffSpellAction
    {
    public:
        CastCreateFirestoneAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "create firestone") {}
    };

    class CastCreateSpellstoneAction : public CastBuffSpellAction
    {
    public:
        CastCreateSpellstoneAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "create spellstone") {}
    };

    class CastBanishAction : public CastBuffSpellAction
    {
    public:
        CastBanishAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "banish on cc") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("cc target", "banish"); }
        virtual bool Execute(Event event) { return ai->CastSpell("banish", GetTarget()); }
    };

    class CastRainOfFireAction : public CastSpellAction
    {
    public:
        CastRainOfFireAction(PlayerbotAI* ai) : CastSpellAction(ai, "rain of fire") {}
    };

    class CastImmolateAction : public CastDebuffSpellAction
    {
    public:
        CastImmolateAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "immolate") {}
    };

    class CastConflagrateAction : public CastSpellAction
    {
    public:
        CastConflagrateAction(PlayerbotAI* ai) : CastSpellAction(ai, "conflagrate") {}
    };

    class CastFearAction : public CastDebuffSpellAction
    {
    public:
        CastFearAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "fear") {}
    };

    class CastFearOnCcAction : public CastBuffSpellAction
    {
    public:
        CastFearOnCcAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "fear on cc") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("cc target", "fear"); }
        virtual bool Execute(Event event) { return ai->CastSpell("fear", GetTarget()); }
    };

    class CastLifeTapAction: public CastSpellAction
    {
    public:
        CastLifeTapAction(PlayerbotAI* ai) : CastSpellAction(ai, "life tap") {}
        virtual string GetTargetName() { return "self target"; }
        virtual bool isUseful() { return AI_VALUE2(uint8, "health", "self target") > sPlayerbotAIConfig.lowHealth; }
    };

}
