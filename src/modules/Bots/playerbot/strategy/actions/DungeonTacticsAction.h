#ifndef _PLAYERBOT_DUNGEONTACTICSACTION_H
#define _PLAYERBOT_DUNGEONTACTICSACTION_H

#include "../Action.h"

namespace ai
{
    class DungeonTacticsAction : public Action
    {
    public:
        DungeonTacticsAction(PlayerbotAI* ai) : Action(ai, "dungeon tactics") {}
        virtual bool Execute(Event event);
    };
}

#endif
