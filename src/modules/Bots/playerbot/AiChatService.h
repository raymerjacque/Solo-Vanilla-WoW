#ifndef _AI_CHAT_SERVICE_H
#define _AI_CHAT_SERVICE_H

#include "Common.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <deque>

class Player;

enum AiChatChannel
{
    AI_CHAT_WHISPER = 0,
    AI_CHAT_SAY     = 1,
    AI_CHAT_PARTY   = 2
};

struct ChatMessageTurn
{
    std::string senderName; // Speaker name (e.g. "Grommash" or "User")
    std::string role;       // "user" or "assistant"
    std::string content;
};

struct PendingAiReply
{
    ObjectGuid botGuid;
    ObjectGuid ownerGuid;
    std::string replyText;
    bool isFallback;
    AiChatChannel channel;
    std::string groupKey;
};

class AiChatService
{
public:
    static AiChatService& instance()
    {
        static AiChatService instance;
        return instance;
    }

    // Called when a real player whispers a bot
    bool ProcessPlayerChat(Player* bot, Player* owner, const std::string& message);

    // Called when a real player says something in local /say near bots
    void ProcessNearbyPlayerSay(Player* player, const std::string& message);

    // Called periodically from main bot manager tick to trigger AI group chatter in taverns/towns
    void ProcessGroupChatTick();

    // Main thread update tick to dispatch queued AI replies
    void Update();

private:
    AiChatService();
    ~AiChatService();

    std::string BuildSystemPrompt(Player* bot, Player* owner);
    std::string BuildGroupSystemPrompt(Player* bot, const std::string& groupKey);
    void PerformApiRequest(ObjectGuid botGuid, ObjectGuid ownerGuid, const std::string& systemPrompt, const std::string& userMessage, AiChatChannel channel = AI_CHAT_WHISPER, const std::string& groupKey = "");
    std::string EscapeJson(const std::string& input);
    std::string ExtractContent(const std::string& json);

    std::mutex m_mutex;
    // Whisper conversation history (key: botGuid_ownerGuid)
    std::map<std::string, std::vector<ChatMessageTurn>> m_conversationHistory;
    // Group conversation history (key: groupKey, e.g. "Area_123_456")
    std::map<std::string, std::vector<ChatMessageTurn>> m_groupHistory;
    std::map<std::string, time_t> m_groupLastTalkTime;
    std::deque<PendingAiReply> m_pendingReplies;
    time_t m_lastGroupScanTime = 0;
};

#define sAiChatService AiChatService::instance()

#endif
