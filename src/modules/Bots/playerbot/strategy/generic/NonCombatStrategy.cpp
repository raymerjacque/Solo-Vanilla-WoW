#include "botpch.h"
#include "../../playerbot.h"
#include "NonCombatStrategy.h"

using namespace ai;

void NonCombatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "timer",
        NextAction::array(0, new NextAction("check mount state", 1.0f), new NextAction("equip", 1.0f), new NextAction("talents", 1.0f), new NextAction("auto auction", 1.0f), new NextAction("auto auction buy", 1.0f), new NextAction("ambient emote", 1.0f), new NextAction("bg queue", 1.0f), new NextAction("bg tactics", 1.0f), new NextAction("dungeon tactics", 1.0f), new NextAction("class epic quest", 1.0f), new NextAction("outdoor pvp", 1.0f), new NextAction("world events", 1.0f), new NextAction("profession service", 1.0f), new NextAction("elite quest group", 1.0f), new NextAction("auto quest", 1.0f), new NextAction("escort", 1.0f), NULL)));
}






