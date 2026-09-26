#include "botpch.h"
#include "../../playerbot.h"
#include "AutoAuctionBuyAction.h"
#include "Object/AuctionHouseMgr.h"
#include "ObjectMgr.h"
#include "Item.h"

using namespace ai;

bool AutoAuctionBuyAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat() || bot->IsDead())
        return false;

    list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
    Creature* auctioneer = NULL;
    for (list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); ++i)
    {
        Creature* unit = bot->GetNPCIfCanInteractWith(*i, UNIT_NPC_FLAG_AUCTIONEER);
        if (unit)
        {
            auctioneer = unit;
            break;
        }
    }

    if (!auctioneer)
        return false;

    return BuyAuctions(auctioneer);
}

bool AutoAuctionBuyAction::BuyAuctions(Creature* auctioneer)
{
    Player* bot = ai->GetBot();
    if (!bot || !auctioneer)
        return false;

    AuctionHouseEntry const* AHentry = AuctionHouseMgr::GetAuctionHouseEntry(auctioneer);
    if (!AHentry)
        return false;

    AuctionHouseObject* auctionHouse = sAuctionMgr.GetAuctionsMap(AHentry);
    if (!auctionHouse)
        return false;

    bool boughtAny = false;
    AuctionHouseObject::AuctionEntryMap const& auctions = auctionHouse->GetAuctions();

    for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = auctions.begin(); itr != auctions.end(); ++itr)
    {
        AuctionEntry* auction = itr->second;
        if (!auction || auction->owner == bot->GetGUIDLow())
            continue;

        uint32 price = auction->buyout > 0 ? auction->buyout : auction->startbid;
        if (price == 0 || bot->GetMoney() < price)
            continue;

        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(auction->itemTemplate);
        if (!proto)
            continue;

        if (proto->Quality >= ITEM_QUALITY_UNCOMMON || proto->Class == ITEM_CLASS_CONSUMABLE || proto->Class == ITEM_CLASS_RECIPE)
        {
            bot->ModifyMoney(-int32(price));

            Player* seller = sObjectMgr.GetPlayer(ObjectGuid(HIGHGUID_PLAYER, auction->owner));
            if (seller)
            {
                seller->ModifyMoney(price);
            }

            Item* item = sAuctionMgr.GetAItem(auction->itemGuidLow);
            if (item)
            {
                bot->StoreNewItemInBestSlots(item->GetEntry(), item->GetCount());
            }

            sAuctionMgr.RemoveAItem(auction->itemGuidLow);
            auctionHouse->RemoveAuction(auction->Id);
            boughtAny = true;

            sLog.outString("AutoAuctionBuyAction: Bot %s bought auction %s (Entry %u) for %u copper",
                           bot->GetName(), proto->Name1, auction->itemTemplate, price);
            break;
        }
    }

    return boughtAny;
}
