#include "botpch.h"
#include "../../playerbot.h"
#include "AmbientEmoteAction.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;

bool AmbientEmoteAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || !bot->GetSession() || bot->IsInCombat() || bot->IsDead() || bot->isMoving())
        return false;

    if (urand(0, 100) > 15)
        return false;

    bool foundPlayer = false;
    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.data.Part.reserved = 0;
    cell.SetNoCreate();

    std::list<Player*> nearPlayers;
    MaNGOS::AnyPlayerInObjectRangeCheck check(bot, 15.0f);
    MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeCheck> searcher(nearPlayers, check);
    TypeContainerVisitor<MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeCheck>, WorldTypeMapContainer> visitor(searcher);
    cell.Visit(p, visitor, *bot->GetMap(), *bot, 15.0f);

    for (std::list<Player*>::iterator itr = nearPlayers.begin(); itr != nearPlayers.end(); ++itr)
    {
        Player* nearPlayer = *itr;
        if (nearPlayer && nearPlayer != bot && nearPlayer->IsInWorld() && bot->IsFriendlyTo(nearPlayer))
        {
            foundPlayer = true;
            break;
        }
    }

    if (!foundPlayer)
        return false;

    uint32 emoteChoice = urand(0, 4);
    switch (emoteChoice)
    {
        case 0:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
            break;
        case 1:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
            break;
        case 2:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
            break;
        case 3:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
            break;
        default:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_APPLAUD);
            break;
    }

    return true;
}
