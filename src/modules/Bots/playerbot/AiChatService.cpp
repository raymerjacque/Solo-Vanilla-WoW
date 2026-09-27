#include "AiChatService.h"
#include "playerbot.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "GuildMgr.h"
#include "Guild.h"
#include "Group.h"
#include "DBCStores.h"
#include "Log.h"
#include <curl/curl.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <algorithm>

static const char* GetRaceName(uint8 race)
{
    switch (race)
    {
        case RACE_HUMAN: return "Human";
        case RACE_ORC: return "Orc";
        case RACE_DWARF: return "Dwarf";
        case RACE_NIGHTELF: return "Night Elf";
        case RACE_UNDEAD: return "Undead";
        case RACE_TAUREN: return "Tauren";
        case RACE_GNOME: return "Gnome";
        case RACE_TROLL: return "Troll";
        default: return "Unknown";
    }
}

static const char* GetClassName(uint8 cls)
{
    switch (cls)
    {
        case CLASS_WARRIOR: return "Warrior";
        case CLASS_PALADIN: return "Paladin";
        case CLASS_HUNTER: return "Hunter";
        case CLASS_ROGUE: return "Rogue";
        case CLASS_PRIEST: return "Priest";
        case CLASS_SHAMAN: return "Shaman";
        case CLASS_MAGE: return "Mage";
        case CLASS_WARLOCK: return "Warlock";
        case CLASS_DRUID: return "Druid";
        default: return "Unknown";
    }
}

static std::string GetBotSpecName(Player* bot)
{
    uint8 cls = bot->getClass();
    static const char* specNames[12][3] = {
        {"", "", ""},
        {"Arms", "Fury", "Protection"},              // Warrior = 1
        {"Holy", "Protection", "Retribution"},        // Paladin = 2
        {"Beast Mastery", "Marksmanship", "Survival"}, // Hunter = 3
        {"Assassination", "Combat", "Subtlety"},      // Rogue = 4
        {"Discipline", "Holy", "Shadow"},              // Priest = 5
        {"", "", ""},
        {"Elemental", "Enhancement", "Restoration"},    // Shaman = 7
        {"Arcane", "Fire", "Frost"},                  // Mage = 8
        {"Affliction", "Demonology", "Destruction"},    // Warlock = 9
        {"", "", ""},
        {"Balance", "Feral", "Restoration"}            // Druid = 11
    };

    if (cls < 12 && specNames[cls][0][0] != '\0')
    {
        uint32 specIdx = (bot->GetGUIDLow() % 3);
        return std::string(specNames[cls][specIdx]) + " " + GetClassName(cls);
    }
    return GetClassName(cls);
}

static std::string GetBotProfessions(Player* bot)
{
    static const std::vector<std::pair<uint32, std::string>> profs = {
        {SKILL_MINING, "Mining"},
        {SKILL_HERBALISM, "Herbalism"},
        {SKILL_SKINNING, "Skinning"},
        {SKILL_BLACKSMITHING, "Blacksmithing"},
        {SKILL_LEATHERWORKING, "Leatherworking"},
        {SKILL_ALCHEMY, "Alchemy"},
        {SKILL_ENGINEERING, "Engineering"},
        {SKILL_TAILORING, "Tailoring"},
        {SKILL_ENCHANTING, "Enchanting"},
        {SKILL_FISHING, "Fishing"},
        {SKILL_COOKING, "Cooking"},
        {SKILL_FIRST_AID, "First Aid"}
    };

    std::ostringstream ss;
    bool first = true;
    for (const auto& pair : profs)
    {
        if (bot->HasSkill(pair.first))
        {
            if (!first) ss << ", ";
            uint32 val = bot->GetSkillValue(pair.first);
            uint32 maxVal = bot->GetMaxSkillValue(pair.first);
            ss << pair.second << " (" << val << "/" << maxVal << ")";
            first = false;
        }
    }
    return first ? "None" : ss.str();
}

static std::string GetBotCurrentActivity(Player* bot)
{
    if (bot->IsInCombat())
    {
        std::string victimName = bot->getVictim() ? bot->getVictim()->GetName() : "enemies";
        return std::string("In combat fighting ") + victimName;
    }

    if (bot->IsMounted())
        return "Mounted and traveling";

    if (!bot->IsStandState())
        return "Resting / sitting in tavern or town";

    if (bot->GetGroup())
    {
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (ai && ai->GetMaster())
            return std::string("Following party leader ") + ai->GetMaster()->GetName();
        return "Adventuring with party";
    }

    return "Solo adventuring";
}

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string AiChatService::EscapeJson(const std::string& input)
{
    std::ostringstream ss;
    for (char c : input)
    {
        switch (c)
        {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f')
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                else
                    ss << c;
                break;
        }
    }
    return ss.str();
}

std::string AiChatService::ExtractContent(const std::string& json)
{
    size_t pos = json.find("\"content\":");
    if (pos == std::string::npos) return "";
    pos += 10;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
    if (pos >= json.size() || json[pos] != '"') return "";
    pos++; // skip starting quote

    std::string result;
    bool escaped = false;
    for (; pos < json.size(); pos++)
    {
        char c = json[pos];
        if (escaped)
        {
            if (c == 'n') result += '\n';
            else if (c == 't') result += '\t';
            else if (c == 'r') result += '\r';
            else result += c;
            escaped = false;
        }
        else if (c == '\\')
        {
            escaped = true;
        }
        else if (c == '"')
        {
            break;
        }
        else
        {
            result += c;
        }
    }

    // Strip <think>...</think> if present
    size_t thinkStart = result.find("<think>");
    if (thinkStart != std::string::npos)
    {
        size_t thinkEnd = result.find("</think>");
        if (thinkEnd != std::string::npos)
            result = result.substr(thinkEnd + 8);
        else
            result = result.substr(thinkStart + 7);
    }

    // Strip starting/ending quotes if wrapped
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"')
    {
        result = result.substr(1, result.size() - 2);
    }

    // Trim leading and trailing whitespace
    size_t first = result.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = result.find_last_not_of(" \t\n\r");
    return result.substr(first, (last - first + 1));
}

std::string AiChatService::BuildSystemPrompt(Player* bot, Player* owner)
{
    if (!bot || !owner) return "";

    std::string botName = bot->GetName();
    std::string botRace = GetRaceName(bot->getRace());
    std::string botClass = GetClassName(bot->getClass());
    std::string botSpec = GetBotSpecName(bot);
    uint32 botLevel = bot->getLevel();
    std::string botFaction = (bot->GetTeam() == ALLIANCE) ? "Alliance" : "Horde";

    std::string zoneName = "Azeroth";
    AreaTableEntry const* area = sAreaStore.LookupEntry(bot->GetZoneId());
    if (area && area->area_name[0])
        zoneName = area->area_name[0];

    std::string areaName = "";
    AreaTableEntry const* subArea = sAreaStore.LookupEntry(bot->GetAreaId());
    if (subArea && subArea->area_name[0])
        areaName = subArea->area_name[0];

    std::string guildName = "None";
    if (Guild* guild = sGuildMgr.GetGuildById(bot->GetGuildId()))
        guildName = guild->GetName();

    std::string professions = GetBotProfessions(bot);
    std::string activity = GetBotCurrentActivity(bot);

    std::string groupStatus = "Solo";
    std::string masterName = "None";
    if (bot->GetGroup())
    {
        groupStatus = "In a party";
        if (bot->GetPlayerbotAI() && bot->GetPlayerbotAI()->GetMaster())
            masterName = bot->GetPlayerbotAI()->GetMaster()->GetName();
    }

    std::vector<std::string> activeQuests;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (questId)
        {
            Quest const* qInfo = sObjectMgr.GetQuestTemplate(questId);
            if (qInfo)
                activeQuests.push_back(qInfo->GetTitle());
        }
    }
    std::string questList = "None";
    if (!activeQuests.empty())
    {
        std::ostringstream qss;
        for (size_t i = 0; i < std::min<size_t>(activeQuests.size(), 5); ++i)
        {
            if (i > 0) qss << ", ";
            qss << activeQuests[i];
        }
        questList = qss.str();
    }

    std::string playerName = owner->GetName();
    std::string playerRace = GetRaceName(owner->getRace());
    std::string playerClass = GetClassName(owner->getClass());
    uint32 playerLevel = owner->getLevel();

    std::ostringstream ss;
    ss << "You are an AI playerbot named " << botName << " in World of Warcraft Vanilla (1.12.1).\n"
       << "Your Exact Character Profile:\n"
       << "- Name: " << botName << "\n"
       << "- Level: " << botLevel << "\n"
       << "- Race: " << botRace << "\n"
       << "- Class & Spec: " << botSpec << "\n"
       << "- Faction: " << botFaction << "\n"
       << "- Guild: " << guildName << "\n"
       << "- Location: " << zoneName << (areaName.empty() ? "" : " (" + areaName + ")") << "\n"
       << "- Professions: " << professions << "\n"
       << "- Current Activity: " << activity << "\n"
       << "- Group Status: " << groupStatus << (masterName == "None" ? "" : " (Leader: " + masterName + ")") << "\n"
       << "- Active Quests: " << questList << "\n\n"
       << "You are whispering in chat with human player " << playerName
       << " (Level " << playerLevel << " " << playerRace << " " << playerClass << ").\n"
       << "Rules:\n"
       << "1. Respond like a friendly real gamer in WoW chat.\n"
       << "2. Use your exact character info above (e.g. if asked your level, say " << botLevel << "; if asked your location, say " << zoneName << "; if asked your spec/professions, use exact details above).\n"
       << "3. Keep your response short, natural, and gamer-like (1 to 2 short sentences, max 20 words).\n"
       << "4. NEVER include any quotes, markdown, asterisks, thinking tags (<think>), or stage directions.";

    return ss.str();
}

std::string AiChatService::BuildGroupSystemPrompt(Player* bot, const std::string& groupKey)
{
    if (!bot) return "";

    std::string botName = bot->GetName();
    std::string botRace = GetRaceName(bot->getRace());
    std::string botClass = GetClassName(bot->getClass());
    std::string botSpec = GetBotSpecName(bot);
    uint32 botLevel = bot->getLevel();

    std::string zoneName = "Azeroth";
    AreaTableEntry const* area = sAreaStore.LookupEntry(bot->GetZoneId());
    if (area && area->area_name[0])
        zoneName = area->area_name[0];

    std::string professions = GetBotProfessions(bot);
    std::string activity = GetBotCurrentActivity(bot);

    std::ostringstream ss;
    ss << "You are an AI playerbot named " << botName << " in World of Warcraft Vanilla (1.12.1).\n"
       << "Your Profile: Level " << botLevel << " " << botRace << " " << botSpec << " in " << zoneName << ".\n"
       << "Professions: " << professions << ". Current Activity: " << activity << ".\n"
       << "You are hanging out with a group of fellow players/bots in public chat.\n"
       << "Rules:\n"
       << "1. Continue the casual group chat naturally as " << botName << ".\n"
       << "2. Keep your line very short (1 sentence, max 12 words), casual, and gamer-like.\n"
       << "3. Do NOT repeat previous lines. Add new comments about quests, dungeons, gear, resting, or WoW lore.\n"
       << "4. NEVER use quotes around your line, no markdown, no asterisks, no stage directions.";

    return ss.str();
}

bool AiChatService::ProcessPlayerChat(Player* bot, Player* owner, const std::string& message)
{
    if (!bot || !owner || message.empty())
        return false;

    ObjectGuid botGuid = bot->GetObjectGuid();
    ObjectGuid ownerGuid = owner->GetObjectGuid();
    std::string systemPrompt = BuildSystemPrompt(bot, owner);

    std::thread([this, botGuid, ownerGuid, systemPrompt, message]()
    {
        PerformApiRequest(botGuid, ownerGuid, systemPrompt, message, AI_CHAT_WHISPER);
    }).detach();

    return true;
}

void AiChatService::ProcessNearbyPlayerSay(Player* player, const std::string& message)
{
    if (!player || message.empty() || !player->IsInWorld())
        return;

    std::string groupKey = "Area_" + std::to_string(player->GetZoneId()) + "_" + std::to_string(player->GetAreaId());

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& history = m_groupHistory[groupKey];
        history.push_back({player->GetName(), "user", message});
        if (history.size() > 10)
        {
            history.erase(history.begin(), history.begin() + (history.size() - 10));
        }
    }

    // Find nearest bot within 15 yards to reply in public SAY
    Player* responderBot = nullptr;
    float minDist = 15.0f;

    sObjectAccessor.DoForAllPlayers([&](Player* b) {
        if (!b || b == player || !b->IsInWorld() || !b->GetPlayerbotAI())
            return;

        float dist = player->GetDistance(b);
        if (dist <= minDist)
        {
            minDist = dist;
            responderBot = b;
        }
    });

    if (responderBot)
    {
        std::string systemPrompt = BuildGroupSystemPrompt(responderBot, groupKey);
        ObjectGuid botGuid = responderBot->GetObjectGuid();
        ObjectGuid playerGuid = player->GetObjectGuid();

        std::thread([this, botGuid, playerGuid, systemPrompt, message, groupKey]()
        {
            PerformApiRequest(botGuid, playerGuid, systemPrompt, message, AI_CHAT_SAY, groupKey);
        }).detach();
    }
}

void AiChatService::ProcessGroupChatTick()
{
    time_t now = time(0);
    if (now - m_lastGroupScanTime < 15)
        return;

    m_lastGroupScanTime = now;

    // Cluster online random bots by area
    std::map<std::string, std::vector<Player*>> areaBotClusters;

    sObjectAccessor.DoForAllPlayers([&](Player* b) {
        if (!b || !b->IsInWorld() || !b->GetPlayerbotAI())
            return;

        std::string groupKey = "Area_" + std::to_string(b->GetZoneId()) + "_" + std::to_string(b->GetAreaId());
        areaBotClusters[groupKey].push_back(b);
    });

    for (auto& pair : areaBotClusters)
    {
        std::string groupKey = pair.first;
        auto& bots = pair.second;

        if (bots.size() < 2)
            continue;

        time_t lastTalk = m_groupLastTalkTime[groupKey];
        uint32 delay = urand(20, 45);
        if (now - lastTalk < delay)
            continue;

        m_groupLastTalkTime[groupKey] = now;

        // Select a random bot to speak next in the group
        uint32 speakerIdx = urand(0, bots.size() - 1);
        Player* speakerBot = bots[speakerIdx];

        std::string systemPrompt = BuildGroupSystemPrompt(speakerBot, groupKey);
        ObjectGuid botGuid = speakerBot->GetObjectGuid();

        std::thread([this, botGuid, systemPrompt, groupKey]()
        {
            PerformApiRequest(botGuid, ObjectGuid(), systemPrompt, "Say the next line in the casual group conversation.", AI_CHAT_SAY, groupKey);
        }).detach();
    }
}

void AiChatService::PerformApiRequest(ObjectGuid botGuid, ObjectGuid ownerGuid, const std::string& systemPrompt, const std::string& userMessage, AiChatChannel channel, const std::string& groupKey)
{
    std::string key = (channel == AI_CHAT_WHISPER) ? (botGuid.GetString() + "_" + ownerGuid.GetString()) : groupKey;

    std::vector<ChatMessageTurn> historyCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (channel == AI_CHAT_WHISPER)
            historyCopy = m_conversationHistory[key];
        else
            historyCopy = m_groupHistory[groupKey];
    }

    std::ostringstream jsonStream;
    jsonStream << "{\n"
               << "  \"model\": \"electra-nano\",\n"
               << "  \"messages\": [\n"
               << "    {\"role\": \"system\", \"content\": \"" << EscapeJson(systemPrompt) << "\"}";

    for (const auto& turn : historyCopy)
    {
        std::string content = turn.senderName.empty() ? turn.content : (turn.senderName + ": " + turn.content);
        jsonStream << ",\n    {\"role\": \"" << turn.role << "\", \"content\": \"" << EscapeJson(content) << "\"}";
    }

    jsonStream << ",\n    {\"role\": \"user\", \"content\": \"" << EscapeJson(userMessage) << "\"}\n"
               << "  ],\n"
               << "  \"temperature\": 0.8,\n"
               << "  \"max_tokens\": 60\n"
               << "}";

    std::string payload = jsonStream.str();
    std::string responseBuffer;

    CURL* curl = curl_easy_init();
    bool success = false;
    std::string replyText;

    if (curl)
    {
        std::string apiKey = sPlayerbotAIConfig.aiApiKey;
        if (apiKey.empty()) apiKey = "YOUR_API_KEY_HERE";
        std::string authHeader = "Authorization: Bearer " + apiKey;

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.makululinux.us/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res == CURLE_OK)
        {
            replyText = ExtractContent(responseBuffer);
            if (!replyText.empty())
            {
                success = true;
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (success)
    {
        m_pendingReplies.push_back({botGuid, ownerGuid, replyText, false, channel, groupKey});
    }
    else
    {
        m_pendingReplies.push_back({botGuid, ownerGuid, "", true, channel, groupKey});
    }
}

void AiChatService::Update()
{
    std::vector<PendingAiReply> repliesToProcess;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_pendingReplies.empty())
        {
            repliesToProcess.push_back(m_pendingReplies.front());
            m_pendingReplies.pop_front();
        }
    }

    for (const auto& item : repliesToProcess)
    {
        Player* bot = sObjectMgr.GetPlayer(item.botGuid);
        if (!bot || !bot->IsInWorld())
            continue;

        if (item.channel == AI_CHAT_WHISPER)
        {
            Player* owner = sObjectMgr.GetPlayer(item.ownerGuid);
            if (!owner || !owner->IsInWorld())
                continue;

            if (!item.isFallback && !item.replyText.empty())
            {
                bot->Whisper(item.replyText, LANG_UNIVERSAL, owner->GetObjectGuid());

                std::lock_guard<std::mutex> lock(m_mutex);
                std::string key = item.botGuid.GetString() + "_" + item.ownerGuid.GetString();
                auto& history = m_conversationHistory[key];
                history.push_back({bot->GetName(), "assistant", item.replyText});
            }
            else
            {
                std::string zoneName = "Azeroth";
                AreaTableEntry const* area = sAreaStore.LookupEntry(bot->GetZoneId());
                if (area && area->area_name[0]) zoneName = area->area_name[0];

                std::ostringstream fss;
                fss << "I'm a level " << bot->getLevel() << " " << GetRaceName(bot->getRace()) << " " << GetClassName(bot->getClass())
                    << " currently in " << zoneName << ". Lead the way!";

                bot->Whisper(fss.str(), LANG_UNIVERSAL, owner->GetObjectGuid());
            }
        }
        else if (item.channel == AI_CHAT_SAY)
        {
            if (!item.isFallback && !item.replyText.empty())
            {
                bot->Say(item.replyText, LANG_UNIVERSAL);

                std::lock_guard<std::mutex> lock(m_mutex);
                auto& history = m_groupHistory[item.groupKey];
                history.push_back({bot->GetName(), "assistant", item.replyText});
                if (history.size() > 10)
                {
                    history.erase(history.begin(), history.begin() + (history.size() - 10));
                }
            }
        }
    }
}
