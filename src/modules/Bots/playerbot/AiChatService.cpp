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
    uint32 botLevel = bot->getLevel();
    std::string botFaction = (bot->GetTeam() == ALLIANCE) ? "Alliance" : "Horde";

    std::string zoneName = "Azeroth";
    AreaTableEntry const* area = sAreaStore.LookupEntry(bot->GetZoneId());
    if (area && area->area_name[0])
        zoneName = area->area_name[0];

    std::string guildName = "None";
    if (Guild* guild = sGuildMgr.GetGuildById(bot->GetGuildId()))
        guildName = guild->GetName();

    std::string groupStatus = "Solo";
    std::string masterName = "None";
    if (bot->GetGroup())
    {
        groupStatus = "Grouped";
        if (bot->GetPlayerbotAI() && bot->GetPlayerbotAI()->GetMaster())
            masterName = bot->GetPlayerbotAI()->GetMaster()->GetName();
    }

    std::string targetName = "None";
    if (Unit* victim = bot->getVictim())
        targetName = victim->GetName();
    else if (bot->GetSelectionGuid())
    {
        if (Unit* sel = sObjectAccessor.GetUnit(*bot, bot->GetSelectionGuid()))
            targetName = sel->GetName();
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
        for (size_t i = 0; i < activeQuests.size(); ++i)
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
    ss << "You are an AI playerbot named " << botName << " in World of Warcraft Vanilla (1.12.1). "
       << "Character info: Level " << botLevel << " " << botRace << " " << botClass << " (" << botFaction << "). "
       << "Current Location: " << zoneName << ". "
       << "Guild: " << guildName << ". "
       << "Group Status: " << groupStatus << " (Leader: " << masterName << "). "
       << "Target: " << targetName << ". "
       << "Active Quests: " << questList << ". "
       << "You are communicating in chat with human player " << playerName
       << " (Level " << playerLevel << " " << playerRace << " " << playerClass << "). "
       << "Rules: "
       << "1. Respond like a friendly real gamer in WoW chat. "
       << "2. Keep your response short, natural, and gamer-like (1 to 2 short sentences). "
       << "3. NEVER include any markdown, asterisks, thinking tags (<think>), or stage directions. "
       << "4. If asked to follow or group up, reply enthusiastically like 'I'm with you, lead the way!' or 'Let's do this!'";

    return ss.str();
}

bool AiChatService::ProcessPlayerChat(Player* bot, Player* owner, const std::string& message)
{
    if (!bot || !owner || message.empty())
        return false;

    ObjectGuid botGuid = bot->GetObjectGuid();
    ObjectGuid ownerGuid = owner->GetObjectGuid();
    std::string systemPrompt = BuildSystemPrompt(bot, owner);

    // Launch async thread to call external LLM API without blocking game loop
    std::thread([this, botGuid, ownerGuid, systemPrompt, message]()
    {
        PerformApiRequest(botGuid, ownerGuid, systemPrompt, message);
    }).detach();

    return true;
}

void AiChatService::PerformApiRequest(ObjectGuid botGuid, ObjectGuid ownerGuid, const std::string& systemPrompt, const std::string& userMessage)
{
    sLog.outString("AiChatService: Performing API request for bot %s to owner %s message: '%s'", botGuid.GetString().c_str(), ownerGuid.GetString().c_str(), userMessage.c_str());
    std::string key = botGuid.GetString() + "_" + ownerGuid.GetString();

    std::vector<ChatMessageTurn> historyCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        historyCopy = m_conversationHistory[key];
    }

    std::ostringstream jsonStream;
    jsonStream << "{\n"
               << "  \"model\": \"electra-nano\",\n"
               << "  \"messages\": [\n"
               << "    {\"role\": \"system\", \"content\": \"" << EscapeJson(systemPrompt) << "\"}";

    for (const auto& turn : historyCopy)
    {
        jsonStream << ",\n    {\"role\": \"" << turn.role << "\", \"content\": \"" << EscapeJson(turn.content) << "\"}";
    }

    jsonStream << ",\n    {\"role\": \"user\", \"content\": \"" << EscapeJson(userMessage) << "\"}\n"
               << "  ],\n"
               << "  \"temperature\": 0.7,\n"
               << "  \"max_tokens\": 120\n"
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
                sLog.outString("AiChatService: Successfully received reply from API: '%s'", replyText.c_str());
                success = true;
            }
            else
            {
                sLog.outError("AiChatService: Failed to parse content from response: %s", responseBuffer.c_str());
            }
        }
        else
        {
            sLog.outError("AiChatService: CURL failed with code %d (%s)", (int)res, curl_easy_strerror(res));
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (success)
    {
        auto& history = m_conversationHistory[key];
        history.push_back({"user", userMessage});
        history.push_back({"assistant", replyText});
        if (history.size() > 10)
        {
            history.erase(history.begin(), history.begin() + (history.size() - 10));
        }

        m_pendingReplies.push_back({botGuid, ownerGuid, replyText, false});
    }
    else
    {
        m_pendingReplies.push_back({botGuid, ownerGuid, "", true});
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
        Player* owner = sObjectMgr.GetPlayer(item.ownerGuid);

        if (!bot || !owner || !bot->IsInWorld() || !owner->IsInWorld())
            continue;

        PlayerbotAI* botAi = bot->GetPlayerbotAI();
        if (!botAi)
            continue;

        if (!item.isFallback && !item.replyText.empty())
        {
            sLog.outString("AiChatService: Bot %s whispering reply to %s: '%s'", bot->GetName(), owner->GetName(), item.replyText.c_str());
            bot->Whisper(item.replyText, LANG_UNIVERSAL, owner->GetObjectGuid());
        }
        else
        {
            sLog.outString("AiChatService: Bot %s sending fallback reply to %s", bot->GetName(), owner->GetName());
            // Fallback response if API fails
            bot->Whisper("I'm with you, lead the way!", LANG_UNIVERSAL, owner->GetObjectGuid());
        }
    }
}
