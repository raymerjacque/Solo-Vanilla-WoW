#ifndef _PLAYERBOT_OUTDOORPVPACTION_H
#define _PLAYERBOT_OUTDOORPVPACTION_H

#include "../Action.h"

namespace ai
{
    class OutdoorPvPAction : public Action
    {
    public:
        OutdoorPvPAction(PlayerbotAI* ai) : Action(ai, "outdoor pvp") {}
        virtual bool Execute(Event event);
    };
}

#endif
