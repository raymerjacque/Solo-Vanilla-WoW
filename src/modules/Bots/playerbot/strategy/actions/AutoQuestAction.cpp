#include "botpch.h"
#include "../../playerbot.h"
#include "AutoQuestAction.h"
#include "ObjectMgr.h"
#include "GossipDef.h"

using namespace ai;

bool AutoQuestAction::Execute(Event event)
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat() || bot->IsDead())
        return false;

    list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
    Creature* questGiver = NULL;
    for (list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); ++i)
    {
        Creature* unit = bot->GetNPCIfCanInteractWith(*i, UNIT_NPC_FLAG_QUESTGIVER);
        if (unit)
        {
            questGiver = unit;
            break;
        }
    }

    if (!questGiver)
        return false;

    return ProcessQuestGiver(questGiver);
}

bool AutoQuestAction::ProcessQuestGiver(Creature* questGiver)
{
    Player* bot = ai->GetBot();
    if (!bot || !questGiver)
        return false;

    ObjectGuid guid = questGiver->GetObjectGuid();
    if (bot->GetDistance(questGiver) > INTERACTION_DISTANCE)
        return false;

    bot->SetSelectionGuid(guid);
    bot->PrepareQuestMenu(guid);
    QuestMenu& questMenu = bot->PlayerTalkClass->GetQuestMenu();

    bool actionTaken = false;

    for (uint32 i = 0; i < questMenu.MenuItemCount(); ++i)
    {
        QuestMenuItem const& menuItem = questMenu.GetItem(i);
        uint32 questID = menuItem.m_qId;
        Quest const* quest = sObjectMgr.GetQuestTemplate(questID);
        if (!quest)
            continue;

        uint32 status = bot->GetQuestStatus(questID);

        if (status == QUEST_STATUS_COMPLETE)
        {
            WorldPacket p(CMSG_QUESTGIVER_COMPLETE_QUEST);
            p << guid << questID;
            p.rpos(0);
            bot->GetSession()->HandleQuestgiverCompleteQuest(p);

            WorldPacket p2(CMSG_QUESTGIVER_REQUEST_REWARD);
            p2 << guid << questID;
            p2.rpos(0);
            bot->GetSession()->HandleQuestgiverRequestRewardOpcode(p2);

            sLog.outString("AutoQuestAction: Bot %s turned in quest '%s' (Entry %u)",
                           bot->GetName(), quest->GetTitle().c_str(), questID);
            actionTaken = true;
            break;
        }
        else if (bot->CanTakeQuest(quest, false) && bot->SatisfyQuestLog(false))
        {
            WorldPacket p(CMSG_QUESTGIVER_ACCEPT_QUEST);
            uint32 unk1 = 0;
            p << guid << questID << unk1;
            p.rpos(0);
            bot->GetSession()->HandleQuestgiverAcceptQuestOpcode(p);

            sLog.outString("AutoQuestAction: Bot %s accepted quest '%s' (Entry %u)",
                           bot->GetName(), quest->GetTitle().c_str(), questID);
            actionTaken = true;
            break;
        }
    }

    return actionTaken;
}
