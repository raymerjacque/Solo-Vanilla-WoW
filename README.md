# Solo-Vanilla-WoW Server Engine (MaNGOS Zero 1.12.1 + Playerbot AI)

[![MaNGOS Zero](https://img.shields.io/badge/MaNGOS-Zero%201.12.1-orange.svg)](https://getmangos.eu)
[![Playerbot AI](https://img.shields.io/badge/Playerbot-AI%20Engine-blue.svg)](https://github.com/raymerjacque/Solo-Vanilla-WoW)
[![Status](https://img.shields.io/badge/Roadmap-100%25%20Completed-brightgreen.svg)](to-do_list.txt)

A complete, feature-packed **Single-Player & Companion Server Engine** for World of Warcraft: Vanilla 1.12.1 based on MaNGOS Zero, featuring autonomous AI playerbots, an external LLM AI chatbot engine, dynamic population scaling, class quest automation, capital city dungeon portals, and full 1–60 content execution.

---

## 🌟 Key Highlights & Overview

- **Full Vanilla 1.12.1 Content**: Complete support for all quest lines, 5-man dungeons, 40-man raids (Molten Core, Onyxia, Blackwing Lair), Battlegrounds (WSG, AB, AV), and World Bosses.
- **Dynamic Bot Population Scaling**: 
  - **0 Bots when Idle**: When no human players are online, the server runs 0 bots, conserving host CPU and RAM.
  - **50 Bots on 1st Login**: When a player logs in, 10 companion bots spawn near the player (level-matched, race-matched, role-balanced: 1 Tank, 1 Healer, 3 DPS) while 40 bots populate global world zones.
  - **+10 Bots per additional player**: Scales dynamically up to 300 bots maximum.
  - **Batched Logins**: Logins are batched at 10 bots/tick to eliminate heap spikes and prevent crashes.
- **Interactive LLM AI Chatbot Engine**: Asynchronous OpenAI-compatible API integration (`AiChatService.cpp`) providing human-like whisper dialogue enriched with real-time in-game bot context.
- **Instant Auto-Account Creation**: `realmd` automatically creates new user accounts upon first login attempt without needing external registration tools.
- **Capital City Dungeon Portals**: `Archmage Teleportus` NPCs stationed in all 6 capital cities providing direct teleportation for players and their grouped bots to all major Vanilla dungeons and raids.
- **Crash-Safe Operations**: Includes explicit session checks (`GetSession()`) preventing GM gold/mail crashes and group disconnect crashes.

---

## 🗺️ Master Development Roadmap (100% Completed)

All **25 Development Phases** are fully implemented and integrated. Track progress in [`to-do_list.txt`](to-do_list.txt) or view the interactive [`to-do-list.html`](to-do-list.html).

| Phase | Description | Status | Core Files |
|---|---|---|---|
| **Phase 01** | Movement & Server Sync Fixes | `[x] Complete` | `AiFactory.cpp`, `Unit.cpp`, `MoveRandomAction.cpp` |
| **Phase 02** | 5-Man Autonomous Dungeon Engine | `[x] Complete` | `RandomPlayerbotMgr.cpp`, `TankStrategy.cpp` |
| **Phase 03** | Economy, Vendor Selling & AH Trading | `[x] Complete` | `SellAction.cpp`, `AutoAuctionBuyAction.cpp` |
| **Phase 04** | Advanced Questing & Playmate Clustering | `[x] Complete` | `TravelAction.cpp`, `RandomPlayerbotMgr.cpp` |
| **Phase 05** | Combat Rotations, Auto-Talents & Consumables | `[x] Complete` | `PlayerbotFactory.cpp`, `PlayerbotAI.cpp` |
| **Phase 06** | Bot Guilds, Emotes & BG Queuing | `[x] Complete` | `AmbientEmoteAction.cpp`, `BGQueueAction.cpp` |
| **Phase 07** | 40-Man Raiding & World Boss Engine | `[x] Complete` | `RandomPlayerbotMgr.cpp` |
| **Phase 08** | Auction House Buying & Player Economy | `[x] Complete` | `AutoAuctionBuyAction.cpp` |
| **Phase 09** | Faction City Invasions & Defense | `[x] Complete` | `RandomPlayerbotMgr.cpp` |
| **Phase 10** | Expanded Chatbot & Natural Dialogue | `[x] Complete` | `PlayerbotAI.cpp` |
| **Phase 11** | Player Party Companion & Auto-Follow | `[x] Complete` | `AcceptInvitationAction.h`, `AiFactory.cpp` |
| **Phase 12** | Auto Spells, Pets & Teleport Tethering | `[x] Complete` | `PlayerbotFactory.cpp`, `PlayerbotAI.cpp` |
| **Phase 13** | Bot Uninvite / Party Disband Resume | `[x] Complete` | `PlayerbotAI.cpp`, `AuthSocket.cpp` |
| **Phase 14** | External LLM AI Chatbot Engine | `[x] Complete` | `AiChatService.cpp`, `AiChatService.h` |
| **Phase 15** | Party Synergy & Group Tactics | `[x] Complete` | `PlayerbotAI.cpp`, `LootRollAction.cpp` |
| **Phase 16** | Dynamic Bot Population Scaling | `[x] Complete` | `RandomPlayerbotMgr.cpp` |
| **Phase 17** | Corpse Repop & Stability Fixes | `[x] Complete` | `Player.cpp` |
| **Phase 18** | Vanilla Honor System & Rank 14 Gear | `[x] Complete` | `PlayerbotFactory.cpp`, `GrindTargetValue.cpp` |
| **Phase 19** | Objective BG Tactics (WSG, AB, AV) | `[x] Complete` | `BGTacticsAction.cpp` |
| **Phase 20** | Advanced Dungeon Boss Mechanics | `[x] Complete` | `DungeonTacticsAction.cpp` |
| **Phase 21** | Class Epic Mount & Questlines | `[x] Complete` | `ClassEpicQuestAction.cpp` |
| **Phase 22** | Outdoor PvP (EPL Towers / Silithyst) | `[x] Complete` | `OutdoorPvPAction.cpp` |
| **Phase 23** | World Events (DMF, Gurubashi, Fishing) | `[x] Complete` | `WorldEventsAction.cpp` |
| **Phase 24** | Profession Crafting & Party Enchanting | `[x] Complete` | `ProfessionServiceAction.cpp` |
| **Phase 25** | Open-World Elite Zones & Escorts | `[x] Complete` | `EliteQuestGroupAction.cpp` |

---

## 🤖 Comprehensive Playerbot AI System Breakdown

### 1. Spells, Talents, Mounts & Pets
- **Auto-Learn Spells**: Bots automatically learn all class spells, spell ranks, shapeshift forms, mounts, and pet summon spells (Warlock Imp, Voidwalker, Succubus, Felhunter; Hunter pet taming/skills) directly upon leveling up—no class trainers required.
- **Auto-Talent Allocation**: Talent points are automatically allocated to match class specs (e.g. Protection Warrior, Holy Priest, Destruction Warlock) as bots level.

### 2. Group Distance, Instance & Taxi Tethering
- **Tethering (>75yd)**: If a bot falls behind while following a human player, it automatically teleports back to the player's side.
- **Instance Entrances/Exits**: Grouped bots automatically port inside when the player enters a dungeon, and port out when the player exits.
- **Flightpath Taxis & Portals**: Grouped bots port directly to the player's destination when taking flightpaths or city portals.

### 3. Party Companion & Synergy Tactics
- **Auto-Accept Invite**: Bots invited by human players auto-accept, set `master = player`, and follow in formation.
- **Target Marking**: Leader/Tank bots automatically assign raid icons (**Skull** on main target, **Cross** on secondary).
- **OOM Drinking Breaks**: Low mana bots announce `"OOM! Resting a sec to drink."`, sit to drink, and pause pulls until mana reaches >70%.
- **Smart Loot Rolling**: Bots roll **Need** on stat/class gear upgrades and **Greed** on remaining items.
- **Emergency Saves**: Paladins cast Lay on Hands (<15% HP), Priests cast Power Word: Shield (<40% HP), Warriors Taunt mobs off squishies/healers.

### 4. Economy, Vendors & Auction House
- **Auto-Equip Upgrades**: Evaluates equipment stats and automatically equips gear upgrades.
- **Vendor Selling & Repairing**: Auto-paths to vendors when bags are full or durability is low to repair gear and sell junk items.
- **AH Trading Engine**: Bots list valuable materials/loot on the Auction House and purchase player-listed items, recipes, and consumables.

### 5. Vanilla Honor System & Rank 14 Gear
- **Honorable Kills**: Bots earn HKs and Honor Points in open-world PvP and Battlegrounds while protecting civilian NPCs to avoid Dishonorable Kills (DKs).
- **Rank 1 to 14 Titles**: Level 60 bots display Vanilla PvP ranks (Scout/Private to High Warlord / Grand Marshal).
- **Rank 14 Epic Weapons**: High-ranking bots equip Grand Marshal / High Warlord Epic weapons and PvP Insignia trinkets.

### 6. Battleground & Dungeon Boss Mechanics
- **Warsong Gulch (WSG)**: Flag carriers auto-path back to scoring rooms; 50/50 midfield defense split.
- **Arathi Basin (AB)**: Coordinates capture and defense of Stables, Gold Mine, Blacksmith, Lumber Mill, and Farm nodes.
- **Alterac Valley (AV)**: Coordinates campaign assaults against Lieutenants, Towers, and Generals (Drek'Thar / Vanndar).
- **Blackrock Depths (BRD)**: Collects Shadowforge Torches to light Lyceum Braziers; tavern patron safety checks.
- **Stratholme**: 45-minute Baron speedrun routing clearing Ziggurats and slaughterhouse abominations.
- **Scholomance & UBRS**: Key door opening and Rend Blackhand arena wave positioning.

### 7. Outdoor PvP, World Events & Professions
- **Eastern Plaguelands Towers**: Captures Crown Guard, Northpass, Eastwall, and Plaguewood towers for **Lordaeron's Blessing**.
- **Silithus Sand Gathering**: Gathers Silithyst Sand in Crystal Vale for **Cenarion Favor**.
- **Darkmoon Faire**: Visits Sayge for fortune-teller buffs (+10% damage/stats).
- **Gurubashi Arena**: Battles for the STV Arena Treasure Chest.
- **Profession Services**: Party equipment enchanting and trade skill consumable crafting.

---

## 💬 External LLM AI Chatbot Engine (`AiChatService`)

Bots feature an asynchronous AI Chatbot engine connected to an OpenAI-compatible API endpoint:
- **Asynchronous Execution**: Uses `libcurl` and background worker threads (`AiChatService.cpp`) to send API requests without blocking the game server loop.
- **Rich In-Game System Prompt**: Supplies bot name, class, race, level, faction, zone, guild, group/master, current target, active quest list, and player details.
- **Whisper Inform Feedback**: Sends `CHAT_MSG_WHISPER_INFORM` packets to human players, displaying pink whisper confirmation text in the WoW chat log.
- **Thinking Tag Sanitization**: Automatically strips internal `<think>...</think>` reasoning tags from LLM responses before displaying in-game.

---

## 🌌 Capital City Dungeon Portals (`Archmage Teleportus`)

Custom Eluna script ([`lua_scripts/dungeon_portals.lua`](lua_scripts/dungeon_portals.lua)) spawns `Archmage Teleportus` in all capital cities:
- **Alliance**: Stormwind City (Trade District), Ironforge (Commons), Darnassus (Temple of the Moon).
- **Horde**: Orgrimmar (Valley of Strength), Undercity (Trade Quarter), Thunder Bluff (Lower Rise).
- **Features**:
  - Teleport to Low-Level, Mid-Level, High-Level Dungeons, and Raids.
  - Teleport between Capital Cities.
  - **Group Teleport**: Teleports all active party members and companion bots along with the player.

---

## 🛠️ Building & Installation

### Prerequisites (Ubuntu / Debian Linux)
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libboost-all-dev libssl-dev libmariadb-dev libcurl4-openssl-dev zlib1g-dev
```

### 1. Build Server
```bash
mkdir -p server_build
cd server_build
cmake ../server -DCMAKE_INSTALL_PREFIX=/opt/mangos -DSCRIPT_LIB_ELUNA=1 -DPLAYERBOTS=1
make -j$(nproc)
make install
```

### 2. Configure `aiplayerbot.conf`
Copy `aiplayerbot.conf.dist` to `aiplayerbot.conf` in your server config directory:
```ini
[AiPlayerbot]
AiPlayerbot.Enabled = 1
AiPlayerbot.MinRandomBots = 50
AiPlayerbot.MaxRandomBots = 300
AiPlayerbot.ApiKey = YOUR_API_KEY_HERE
```

### 3. Run Server Daemons
```bash
./realmd &
./mangosd
```

---

## 📜 Credits & Acknowledgments

- **MaNGOS Zero Team**: For the core 1.12.1 World of Warcraft server foundation.
- **Playerbot AI Core (ike3 & blueboy)**: Original Playerbot AI framework.
- **Eluna Engine Team**: Lua scripting engine integration.
- **ScriptDev3 Developers**: C++ database scripting library.

---
*Maintained & developed for the Solo Vanilla WoW Community.*
