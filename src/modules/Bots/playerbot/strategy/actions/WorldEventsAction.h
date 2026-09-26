#ifndef _PLAYERBOT_WORLDEVENTSACTION_H
#define _PLAYERBOT_WORLDEVENTSACTION_H

#include "../Action.h"

namespace ai
{
    class WorldEventsAction : public Action
    {
    public:
        WorldEventsAction(PlayerbotAI* ai) : Action(ai, "world events") {}
        virtual bool Execute(Event event);
    };
}

#endif
