#pragma once

#include "../Action.h"
#include "InventoryAction.h"

namespace ai
{
    class AutoAuctionAction : public InventoryAction {
    public:
        AutoAuctionAction(PlayerbotAI* ai) : InventoryAction(ai, "auto auction") {}
        virtual bool Execute(Event event);

    private:
        bool PostAuctions(Creature* auctioneer);
    };
}
