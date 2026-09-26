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

struct ChatMessageTurn
{
    std::string role;    // "user" or "assistant"
    std::string content;
};

struct PendingAiReply
{
    ObjectGuid botGuid;
    ObjectGuid ownerGuid;
    std::string replyText;
    bool isFallback;
};

class AiChatService
{
public:
    static AiChatService& instance()
    {
        static AiChatService instance;
        return instance;
    }

    // Called when a real player sends a chat message to a bot
    bool ProcessPlayerChat(Player* bot, Player* owner, const std::string& message);

    // Called on main thread tick to send queued AI replies to players
    void Update();

private:
    AiChatService() = default;
    ~AiChatService() = default;

    std::string BuildSystemPrompt(Player* bot, Player* owner);
    void PerformApiRequest(ObjectGuid botGuid, ObjectGuid ownerGuid, const std::string& systemPrompt, const std::string& userMessage);
    std::string EscapeJson(const std::string& input);
    std::string ExtractContent(const std::string& json);

    std::mutex m_mutex;
    // Map key: string of botGuid.GetString() + "_" + ownerGuid.GetString()
    std::map<std::string, std::vector<ChatMessageTurn>> m_conversationHistory;
    std::deque<PendingAiReply> m_pendingReplies;
};

#define sAiChatService AiChatService::instance()

#endif
