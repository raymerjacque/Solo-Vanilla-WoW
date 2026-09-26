#include "botpch.h"
#include "../../playerbot.h"
#include "AutoAuctionAction.h"
#include "Object/AuctionHouseMgr.h"
#include "Item.h"
#include "Bag.h"

using namespace ai;

bool AutoAuctionAction::Execute(Event event)
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

    return PostAuctions(auctioneer);
}

bool AutoAuctionAction::PostAuctions(Creature* auctioneer)
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

    bool postedAny = false;

    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item || item->IsSoulBound() || item->CanBeTraded() == false)
            continue;

        ItemPrototype const* proto = item->GetProto();
        if (!proto)
            continue;

        if (proto->Quality >= ITEM_QUALITY_UNCOMMON || proto->Class == ITEM_CLASS_RECIPE || proto->Class == ITEM_CLASS_TRADE_GOODS)
        {
            uint32 sellPrice = proto->SellPrice > 0 ? proto->SellPrice : 100;
            uint32 bid = sellPrice * item->GetCount() * 2;
            uint32 buyout = sellPrice * item->GetCount() * 4;
            uint32 etime = 4 * MIN_AUCTION_TIME;

            uint32 deposit = AuctionHouseMgr::GetAuctionDeposit(AHentry, etime, item);
            if (bot->GetMoney() < deposit)
                continue;

            bot->ModifyMoney(-int32(deposit));
            auctionHouse->AddAuction(AHentry, item, etime, bid, buyout, deposit, bot);
            postedAny = true;

            sLog.outString("AutoAuctionAction: Bot %s posted auction %s (Entry %u x%u) for %u copper",
                           bot->GetName(), proto->Name1, item->GetEntry(), item->GetCount(), buyout);
            break;
        }
    }

    if (!postedAny)
    {
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        {
            Bag* bag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bagSlot);
            if (!bag) continue;

            for (uint8 slot = 0; slot < bag->GetBagSize(); ++slot)
            {
                Item* item = bot->GetItemByPos(bagSlot, slot);
                if (!item || item->IsSoulBound() || item->CanBeTraded() == false)
                    continue;

                ItemPrototype const* proto = item->GetProto();
                if (!proto) continue;

                if (proto->Quality >= ITEM_QUALITY_UNCOMMON || proto->Class == ITEM_CLASS_RECIPE || proto->Class == ITEM_CLASS_TRADE_GOODS)
                {
                    uint32 sellPrice = proto->SellPrice > 0 ? proto->SellPrice : 100;
                    uint32 bid = sellPrice * item->GetCount() * 2;
                    uint32 buyout = sellPrice * item->GetCount() * 4;
                    uint32 etime = 4 * MIN_AUCTION_TIME;

                    uint32 deposit = AuctionHouseMgr::GetAuctionDeposit(AHentry, etime, item);
                    if (bot->GetMoney() < deposit)
                        continue;

                    bot->ModifyMoney(-int32(deposit));
                    auctionHouse->AddAuction(AHentry, item, etime, bid, buyout, deposit, bot);
                    postedAny = true;

                    sLog.outString("AutoAuctionAction: Bot %s posted auction %s (Entry %u x%u) for %u copper",
                                   bot->GetName(), proto->Name1, item->GetEntry(), item->GetCount(), buyout);
                    break;
                }
            }
            if (postedAny) break;
        }
    }

    return postedAny;
}
