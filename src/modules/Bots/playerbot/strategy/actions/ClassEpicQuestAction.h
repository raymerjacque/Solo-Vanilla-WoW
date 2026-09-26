#ifndef _PLAYERBOT_CLASSEPICQUESTACTION_H
#define _PLAYERBOT_CLASSEPICQUESTACTION_H

#include "../Action.h"

namespace ai
{
    class ClassEpicQuestAction : public Action
    {
    public:
        ClassEpicQuestAction(PlayerbotAI* ai) : Action(ai, "class epic quest") {}
        virtual bool Execute(Event event);
    };
}

#endif
