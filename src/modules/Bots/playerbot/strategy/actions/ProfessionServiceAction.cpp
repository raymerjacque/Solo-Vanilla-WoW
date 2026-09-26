#include "botpch.h"
#include "../../playerbot.h"
#include "ProfessionServiceAction.h"
#include "Player.h"
#include "Item.h"
#include "World.h"
#include "ObjectMgr.h"

using namespace ai;

bool ProfessionServiceAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->IsBeingTeleported())
        return false;

    // Do not interfere if bot is following human master in party during active combat
    if (bot->IsInCombat())
        return false;

    // Check if bot has any trade skill (Alchemy, Blacksmithing, Enchanting, Leatherworking, Tailoring)
    // Skill IDs: 171 (Alchemy), 164 (Blacksmithing), 333 (Enchanting), 165 (Leatherworking), 197 (Tailoring)
    bool hasEnchanting = bot->HasSkill(333);
    bool hasCrafting = bot->HasSkill(171) || bot->HasSkill(164) || bot->HasSkill(165) || bot->HasSkill(197);

    if (!hasEnchanting && !hasCrafting)
        return false;

    // Party Enchanting Service
    Group* group = bot->GetGroup();
    if (hasEnchanting && group)
    {
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || !member->IsInWorld() || member->GetMapId() != bot->GetMapId())
                continue;

            if (bot->GetDistance(member) < 10.0f && urand(0, 100) < 5)
            {
                // Emote offering enchanting service
                bot->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                return true;
            }
        }
    }

    // Trade skill ambient crafting emote during rest
    if (hasCrafting && urand(0, 100) < 2)
    {
        bot->HandleEmoteCommand(EMOTE_STATE_USESTANDING);
        return true;
    }

    return false;
}
