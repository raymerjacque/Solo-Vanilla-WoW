#include "botpch.h"
#include "../../playerbot.h"
#include "ReviveFromCorpseAction.h"
#include "../../PlayerbotFactory.h"
#include "../../PlayerbotAIConfig.h"

using namespace ai;

bool ReviveFromCorpseAction::Execute(Event event)
{
    if (bot->IsAlive())
    {
        return false;
    }

    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
    {
        bot->BuildPlayerRepop();
        corpse = bot->GetCorpse();
    }

    time_t now = time(0);
    time_t deathTime = corpse ? corpse->GetGhostTime() : now;

    if (now - deathTime < 30)
    {
        return false;
    }

    bot->ResurrectPlayer(0.5f, false);
    bot->SpawnCorpseBones();
    bot->SaveToDB();

    Player* master = ai->GetMaster();
    if (master && master->IsInWorld() && master != bot)
    {
        bot->TeleportTo(master->GetMapId(), master->GetPositionX(), master->GetPositionY(), master->GetPositionZ(), master->GetOrientation());
    }

    context->GetValue<Unit*>("current target")->Set(NULL);
    bot->SetSelectionGuid(ObjectGuid());
    return true;
}

bool SpiritHealerAction::Execute(Event event)
{
    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
    {
        return false;
    }

    list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
    for (list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (unit && unit->IsSpiritHealer())
        {
            PlayerbotChatHandler ch(bot);
            if (! ch.revive(*bot))
            {
                ai->TellMaster(".. could not be revived ..");
                return false;
            }
            context->GetValue<Unit*>("current target")->Set(NULL);
            bot->SetSelectionGuid(ObjectGuid());
            return true;
        }
    }

    ai->TellMaster("Cannot find any spirit healer nearby");
    return false;
}
