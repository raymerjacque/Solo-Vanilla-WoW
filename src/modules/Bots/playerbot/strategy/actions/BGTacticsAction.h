#ifndef _PLAYERBOT_BGTACTICSACTION_H
#define _PLAYERBOT_BGTACTICSACTION_H

#include "../Action.h"

namespace ai
{
    class BGTacticsAction : public Action
    {
    public:
        BGTacticsAction(PlayerbotAI* ai) : Action(ai, "bg tactics") {}
        virtual bool Execute(Event event);
    };
}

#endif
