#include "botpch.h"
#include "../../playerbot.h"
#include "ClassEpicQuestAction.h"

using namespace ai;

bool ClassEpicQuestAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsDead() || bot->getLevel() < 60)
        return false;

    uint8 botClass = bot->getClass();

    // 1. WARLOCK DREADSTEED QUESTLINE
    if (botClass == CLASS_WARLOCK)
    {
        // Spell 23161: Summon Dreadsteed
        if (!bot->HasSpell(23161))
        {
            bot->learnSpell(23161, false);
            bot->learnSpell(5784, false); // Felsteed
            ai->TellMasterNoFacing("I have unlocked my Warlock Dreadsteed epic mount!");
            return true;
        }
    }
    // 2. PALADIN CHARGER QUESTLINE
    else if (botClass == CLASS_PALADIN)
    {
        // Spell 23214: Summon Charger
        if (!bot->HasSpell(23214))
        {
            bot->learnSpell(23214, false);
            bot->learnSpell(13819, false); // Warhorse
            ai->TellMasterNoFacing("I have completed the Holy Charger ritual and unlocked my epic mount!");
            return true;
        }
    }
    // 3. HUNTER RHOK'DELAR EPIC BOW QUEST
    else if (botClass == CLASS_HUNTER)
    {
        // Item 18713: Rhok'delar, Longbow of the Ancient Sinew
        // Item 18715: Lok'delar, Stave of the Ancient Sinew
        if (!bot->HasItemCount(18713, 1, true))
        {
            ItemPrototype const* protoBow = ObjectMgr::GetItemPrototype(18713);
            ItemPrototype const* protoStaff = ObjectMgr::GetItemPrototype(18715);
            if (protoBow && protoStaff)
            {
                Item* bow = Item::CreateItem(18713, 1, bot);
                Item* staff = Item::CreateItem(18715, 1, bot);
                if (bow && staff)
                {
                    bow->SaveToDB();
                    staff->SaveToDB();
                    bot->AddMItem(bow);
                    bot->AddMItem(staff);
                    ai->TellMasterNoFacing("I have slain the Ancient Demons and crafted Rhok'delar & Lok'delar!");
                    return true;
                }
            }
        }
    }
    // 4. PRIEST ANATHEMA / BENEDICTION QUEST
    else if (botClass == CLASS_PRIEST)
    {
        // Item 18608: Benediction / 18609: Anathema
        if (!bot->HasItemCount(18608, 1, true) && !bot->HasItemCount(18609, 1, true))
        {
            ItemPrototype const* protoStaff = ObjectMgr::GetItemPrototype(18608);
            if (protoStaff)
            {
                Item* staff = Item::CreateItem(18608, 1, bot);
                if (staff)
                {
                    staff->SaveToDB();
                    bot->AddMItem(staff);
                    ai->TellMasterNoFacing("I have completed the Balance of Light and Dark quest and forged Benediction!");
                    return true;
                }
            }
        }
    }

    return false;
}
