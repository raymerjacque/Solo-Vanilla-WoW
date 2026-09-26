#pragma once

#include "../Action.h"

namespace ai
{
    class AutoQuestAction : public Action {
    public:
        AutoQuestAction(PlayerbotAI* ai) : Action(ai, "auto quest") {}
        virtual bool Execute(Event event);
    private:
        bool ProcessQuestGiver(Creature* creature);
    };
}
