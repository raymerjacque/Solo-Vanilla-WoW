#ifndef MANGOS_NPCBOTMGR_H
#define MANGOS_NPCBOTMGR_H

#include "Common.h"
#include "ObjectGuid.h"
#include "NpcBotDefines.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "WorldHandlers/LoginQueryHolder.h"
#include "WorldSession.h"
#include "Database/DatabaseEnv.h"

class Player;
class SqlQueryHolder;

class NpcBotLoginQueryHolder : public LoginQueryHolder
{
private:
    uint32 m_spotId;
public:
    NpcBotLoginQueryHolder(uint32 spotId, uint32 accountId, ObjectGuid guid)
        : LoginQueryHolder(accountId, guid), m_spotId(spotId) {}
    uint32 GetSpotId() const { return m_spotId; }
};

class NpcBotMgr
{
public:
    static NpcBotMgr& instance()
    {
        static NpcBotMgr instance;
        return instance;
    }

    void Initialize();
    void Update(uint32 diff);
    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea);

    bool IsNpcBot(Player* player) const;
    bool IsNpcBot(ObjectGuid guid) const;

    void ActivateHub(uint32 hubId);
    void DeactivateHub(uint32 hubId);

    void HandleNpcBotLoginCallback(QueryResult* dummy, SqlQueryHolder* holder);

private:
    NpcBotMgr();
    ~NpcBotMgr();

    void RegisterHubs();
    void SpawnBotForSpot(const NpcBotHub& hub, const NpcBotSpot& spot);
    void DespawnBotForSpot(uint32 spotId);
    void UpdateBotBehaviors(uint32 diff);
    void OnNpcBotLoaded(Player* bot, const NpcBotSpot& spot);
    const NpcBotSpot* FindSpot(uint32 spotId) const;

    std::unordered_map<uint32, NpcBotHub> m_hubs; // hubId -> Hub
    std::unordered_map<uint32, std::vector<uint32>> m_zoneToHubs; // zoneId -> vector of hubIds
    std::unordered_map<uint32, std::vector<uint32>> m_areaToHubs; // areaId -> vector of hubIds
    std::unordered_set<ObjectGuid> m_npcBotGuids; // set of active NPCBot GUIDs
    std::unordered_map<uint32, ObjectGuid> m_spotToBotGuid; // spotId -> active bot ObjectGuid
    uint32 m_updateTimer;
};

#define sNpcBotMgr NpcBotMgr::instance()

#endif // MANGOS_NPCBOTMGR_H
