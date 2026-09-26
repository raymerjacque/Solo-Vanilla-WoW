#pragma once

#include "../Action.h"

namespace ai
{
    class AutoAuctionBuyAction : public Action {
    public:
        AutoAuctionBuyAction(PlayerbotAI* ai) : Action(ai, "auto auction buy") {}
        virtual bool Execute(Event event);
    private:
        bool BuyAuctions(Creature* auctioneer);
    };
}
