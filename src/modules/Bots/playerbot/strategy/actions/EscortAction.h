#pragma once

#include "../Action.h"

namespace ai
{
    class EscortAction : public Action {
    public:
        EscortAction(PlayerbotAI* ai) : Action(ai, "escort") {}
        virtual bool Execute(Event event);
    };
}
