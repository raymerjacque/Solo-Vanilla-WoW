#include "botpch.h"
#include "../../playerbot.h"
#include "GuildAcceptAction.h"

using namespace std;
using namespace ai;

bool GuildAcceptAction::Execute(Event event)
{
    if (bot->GetGuildId())
    {
        WorldPacket packet;
        bot->GetSession()->HandleGuildDeclineOpcode(packet);
        return false;
    }

    uint32 guildId = bot->GetGuildIdInvited();
    if (!guildId)
    {
        Player* master = GetMaster();
        if (master) guildId = master->GetGuildId();
    }

    if (guildId)
    {
        WorldPacket packet;
        bot->SetGuildIdInvited(guildId);
        bot->GetSession()->HandleGuildAcceptOpcode(packet);
        return true;
    }

    return false;
}

