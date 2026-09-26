#pragma once

#include "../Action.h"

namespace ai
{
    class AmbientEmoteAction : public Action {
    public:
        AmbientEmoteAction(PlayerbotAI* ai) : Action(ai, "ambient emote") {}
        virtual bool Execute(Event event);
    };
}
