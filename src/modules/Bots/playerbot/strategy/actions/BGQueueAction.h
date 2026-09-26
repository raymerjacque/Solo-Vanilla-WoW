#pragma once

#include "../Action.h"

namespace ai
{
    class BGQueueAction : public Action {
    public:
        BGQueueAction(PlayerbotAI* ai) : Action(ai, "bg queue") {}
        virtual bool Execute(Event event);
    };
}
