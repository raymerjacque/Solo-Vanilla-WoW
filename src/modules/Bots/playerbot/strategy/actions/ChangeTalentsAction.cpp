#include "botpch.h"
#include "../../playerbot.h"
#include "ChangeTalentsAction.h"
#include "../../PlayerbotFactory.h"

using namespace ai;

bool ChangeTalentsAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->GetFreeTalentPoints())
        return false;

    PlayerbotFactory factory(bot, bot->getLevel());
    factory.InitTalents();
    return true;
}

