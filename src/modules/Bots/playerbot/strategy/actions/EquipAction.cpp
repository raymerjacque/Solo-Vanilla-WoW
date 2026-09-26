#include "botpch.h"
#include "../../playerbot.h"
#include "EquipAction.h"

#include "../values/ItemCountValue.h"
#include "../values/ItemUsageValue.h"

using namespace ai;

class EquipUpgradesVisitor : public IterateItemsVisitor
{
public:
    EquipUpgradesVisitor(EquipAction* action, AiObjectContext* context) : IterateItemsVisitor(), action(action), context(context) {}

    virtual bool Visit(Item* item)
    {
        if (!item || !context || !action)
            return true;

        ItemPrototype const* proto = item->GetProto();
        if (!proto)
            return true;

        ostringstream out;
        out << proto->ItemId;
        ItemUsage usage = context->GetValue<ItemUsage>("item usage", out.str())->Get();
        if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE)
        {
            action->EquipItem(*item);
        }
        return true;
    }

private:
    EquipAction* action;
    AiObjectContext* context;
};

bool EquipAction::Execute(Event event)
{
    string text = event.getParam();

    if (text.empty())
    {
        EquipUpgradesVisitor visitor(this, context);
        IterateItems(&visitor);
        return true;
    }


    ItemIds ids = chat->parseItems(text);

    for (ItemIds::iterator i = ids.begin(); i != ids.end(); i++)
    {
        FindItemByIdVisitor visitor(*i);
        EquipItem(&visitor);
    }

    return true;
}


void EquipAction::EquipItem(FindItemVisitor* visitor)
{
    IterateItems(visitor);
    list<Item*> items = visitor->GetResult();
    if (!items.empty()) EquipItem(**items.begin());
}


void EquipAction::EquipItem(Item& item)
{
    uint8 bagIndex = item.GetBagSlot();
    uint8 slot = item.GetSlot();
    uint32 itemId = item.GetProto()->ItemId;

    if (item.GetProto()->InventoryType == INVTYPE_AMMO)
    {
        bot->SetAmmo(itemId);
    }
    else
    {
        WorldPacket* const packet = new WorldPacket(CMSG_AUTOEQUIP_ITEM, 2);
            *packet << bagIndex << slot;
        bot->GetSession()->QueuePacket(packet);
    }

    ostringstream out; out << "equipping " << chat->formatItem(item.GetProto());
    ai->TellMaster(out);
}
