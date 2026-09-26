#ifndef _PLAYERBOT_PROFESSIONSERVICEACTION_H
#define _PLAYERBOT_PROFESSIONSERVICEACTION_H

#include "../Action.h"

namespace ai
{
    class ProfessionServiceAction : public Action
    {
    public:
        ProfessionServiceAction(PlayerbotAI* ai) : Action(ai, "profession service") {}
        virtual bool Execute(Event event);
    };
}

#endif
