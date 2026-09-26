#ifndef _PLAYERBOT_ELITEQUESTGROUPACTION_H
#define _PLAYERBOT_ELITEQUESTGROUPACTION_H

#include "../Action.h"

namespace ai
{
    class EliteQuestGroupAction : public Action
    {
    public:
        EliteQuestGroupAction(PlayerbotAI* ai) : Action(ai, "elite quest group") {}
        virtual bool Execute(Event event);
    };
}

#endif
