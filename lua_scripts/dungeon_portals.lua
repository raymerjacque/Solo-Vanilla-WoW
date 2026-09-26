-- ===============================================================================
-- ARCHMAGE TELEPORTUS - DUNGEON & CITY PORTAL MASTER (ELUNA LUA SCRIPT)
-- ===============================================================================

local NPC_ENTRY = 90000

local function TeleportParty(player, mapId, x, y, z, o)
    player:CastSpell(player, 18960, true) -- Teleport Visual
    local group = player:GetGroup()
    if group then
        local members = group:GetMembers()
        for _, member in ipairs(members) do
            if member and member:IsInWorld() then
                member:Teleport(mapId, x, y, z, o)
            end
        end
    else
        player:Teleport(mapId, x, y, z, o)
    end
end

local function OnGossipHello(event, player, creature)
    player:GossipClearMenu()
    player:GossipMenuAddItem(0, "[Dungeons] Level 15 - 30 (Low Level)", 1, 100)
    player:GossipMenuAddItem(0, "[Dungeons] Level 30 - 50 (Mid Level)", 1, 200)
    player:GossipMenuAddItem(0, "[Dungeons] Level 50 - 60 (High Level)", 1, 300)
    player:GossipMenuAddItem(0, "[Raids] Level 60 Raid Entrances", 1, 400)
    player:GossipMenuAddItem(0, "[Cities] Capital City Portals", 1, 500)
    player:GossipSendMenu(1, creature)
end

local function OnGossipSelect(event, player, creature, sender, intid)
    if intid == 100 then
        -- Submenu 100: Low Level Dungeons
        player:GossipClearMenu()
        player:GossipMenuAddItem(0, "Deadmines (Westfall)", 1, 101)
        player:GossipMenuAddItem(0, "Stockade (Stormwind)", 1, 102)
        player:GossipMenuAddItem(0, "Wailing Caverns (Barrens)", 1, 103)
        player:GossipMenuAddItem(0, "Shadowfang Keep (Silverpine)", 1, 104)
        player:GossipMenuAddItem(0, "Blackfathom Deeps (Ashenvale)", 1, 105)
        player:GossipMenuAddItem(0, "Razorfen Kraul (Barrens)", 1, 106)
        player:GossipMenuAddItem(0, "<< Back to Main Menu", 1, 999)
        player:GossipSendMenu(1, creature)
    elseif intid == 200 then
        -- Submenu 200: Mid Level Dungeons
        player:GossipClearMenu()
        player:GossipMenuAddItem(0, "Razorfen Downs (Barrens)", 1, 201)
        player:GossipMenuAddItem(0, "Scarlet Monastery (Tirisfal Glades)", 1, 202)
        player:GossipMenuAddItem(0, "Uldaman (Badlands)", 1, 203)
        player:GossipMenuAddItem(0, "Zul'Farrak (Tanaris)", 1, 204)
        player:GossipMenuAddItem(0, "Maraudon (Desolace)", 1, 205)
        player:GossipMenuAddItem(0, "<< Back to Main Menu", 1, 999)
        player:GossipSendMenu(1, creature)
    elseif intid == 300 then
        -- Submenu 300: High Level Dungeons
        player:GossipClearMenu()
        player:GossipMenuAddItem(0, "Sunken Temple (Swamp of Sorrows)", 1, 301)
        player:GossipMenuAddItem(0, "Blackrock Depths (Blackrock Mountain)", 1, 302)
        player:GossipMenuAddItem(0, "Lower / Upper Blackrock Spire (BRM)", 1, 303)
        player:GossipMenuAddItem(0, "Stratholme (Eastern Plaguelands)", 1, 304)
        player:GossipMenuAddItem(0, "Scholomance (Western Plaguelands)", 1, 305)
        player:GossipMenuAddItem(0, "Dire Maul (Feralas)", 1, 306)
        player:GossipMenuAddItem(0, "<< Back to Main Menu", 1, 999)
        player:GossipSendMenu(1, creature)
    elseif intid == 400 then
        -- Submenu 400: Raids
        player:GossipClearMenu()
        player:GossipMenuAddItem(0, "Molten Core (Blackrock Mountain Balcony)", 1, 401)
        player:GossipMenuAddItem(0, "Onyxia's Lair (Dustwallow Marsh)", 1, 402)
        player:GossipMenuAddItem(0, "Blackwing Lair (BRM Spire)", 1, 403)
        player:GossipMenuAddItem(0, "Zul'Gurub (Stranglethorn Vale)", 1, 404)
        player:GossipMenuAddItem(0, "Naxxramas (Light's Hope Chapel, EPL)", 1, 405)
        player:GossipMenuAddItem(0, "<< Back to Main Menu", 1, 999)
        player:GossipSendMenu(1, creature)
    elseif intid == 500 then
        -- Submenu 500: Capital Cities
        player:GossipClearMenu()
        player:GossipMenuAddItem(0, "Stormwind City (Trade District)", 1, 501)
        player:GossipMenuAddItem(0, "Ironforge (Bank Plaza)", 1, 502)
        player:GossipMenuAddItem(0, "Darnassus (Temple of the Moon)", 1, 503)
        player:GossipMenuAddItem(0, "Orgrimmar (Valley of Strength)", 1, 504)
        player:GossipMenuAddItem(0, "Undercity (Trade Quarter)", 1, 505)
        player:GossipMenuAddItem(0, "Thunder Bluff (Main Mesa)", 1, 506)
        player:GossipMenuAddItem(0, "<< Back to Main Menu", 1, 999)
        player:GossipSendMenu(1, creature)
    elseif intid == 999 then
        OnGossipHello(event, player, creature)
    -- Teleports
    elseif intid == 101 then TeleportParty(player, 0, -11208.0, 1672.0, 24.5, 1.5) player:GossipComplete()
    elseif intid == 102 then TeleportParty(player, 0, -8756.0, 835.0, 98.0, 0.0) player:GossipComplete()
    elseif intid == 103 then TeleportParty(player, 1, -745.0, -1136.0, -26.0, 0.0) player:GossipComplete()
    elseif intid == 104 then TeleportParty(player, 0, -234.0, 1563.0, 76.0, 1.2) player:GossipComplete()
    elseif intid == 105 then TeleportParty(player, 1, 4253.0, 712.0, -15.0, 5.5) player:GossipComplete()
    elseif intid == 106 then TeleportParty(player, 1, -4467.0, -1671.0, 82.0, 1.0) player:GossipComplete()
    elseif intid == 201 then TeleportParty(player, 1, -4657.0, -2523.0, 81.0, 4.5) player:GossipComplete()
    elseif intid == 202 then TeleportParty(player, 0, 2871.0, -816.0, 160.0, 0.0) player:GossipComplete()
    elseif intid == 203 then TeleportParty(player, 0, -6067.0, -2955.0, 209.0, 0.0) player:GossipComplete()
    elseif intid == 204 then TeleportParty(player, 1, -6801.0, -2893.0, 9.0, 0.0) player:GossipComplete()
    elseif intid == 205 then TeleportParty(player, 1, -1423.0, 2906.0, 137.0, 3.1) player:GossipComplete()
    elseif intid == 301 then TeleportParty(player, 0, -10175.0, -3995.0, 5.0, 6.0) player:GossipComplete()
    elseif intid == 302 then TeleportParty(player, 0, -7178.0, -921.0, 166.0, 5.0) player:GossipComplete()
    elseif intid == 303 then TeleportParty(player, 0, -7528.0, -1225.0, 285.0, 5.3) player:GossipComplete()
    elseif intid == 304 then TeleportParty(player, 0, 3348.0, -3379.0, 143.0, 6.2) player:GossipComplete()
    elseif intid == 305 then TeleportParty(player, 0, 1270.0, -2554.0, 88.0, 4.7) player:GossipComplete()
    elseif intid == 306 then TeleportParty(player, 1, -3825.0, 1249.0, 160.0, 4.7) player:GossipComplete()
    elseif intid == 401 then TeleportParty(player, 0, -7515.0, -1045.0, 182.0, 0.0) player:GossipComplete()
    elseif intid == 402 then TeleportParty(player, 1, -4708.0, -3727.0, 54.0, 3.8) player:GossipComplete()
    elseif intid == 403 then TeleportParty(player, 0, -7526.0, -1227.0, 285.0, 5.3) player:GossipComplete()
    elseif intid == 404 then TeleportParty(player, 0, -11916.0, -1206.0, 92.0, 4.7) player:GossipComplete()
    elseif intid == 405 then TeleportParty(player, 0, 3159.0, -4042.0, 120.0, 1.6) player:GossipComplete()
    elseif intid == 501 then TeleportParty(player, 0, -8831.0, 626.0, 94.0, 1.0) player:GossipComplete()
    elseif intid == 502 then TeleportParty(player, 0, -4918.0, -940.0, 501.0, 5.4) player:GossipComplete()
    elseif intid == 503 then TeleportParty(player, 1, 9662.0, 2520.0, 1331.0, 0.0) player:GossipComplete()
    elseif intid == 504 then TeleportParty(player, 1, 1601.0, -4379.0, 9.8, 1.5) player:GossipComplete()
    elseif intid == 505 then TeleportParty(player, 0, 1586.0, 239.0, -52.0, 3.0) player:GossipComplete()
    elseif intid == 506 then TeleportParty(player, 1, -1277.0, 118.0, 131.0, 0.5) player:GossipComplete()
    end
end

RegisterCreatureGossipEvent(NPC_ENTRY, 1, OnGossipHello)
RegisterCreatureGossipEvent(NPC_ENTRY, 2, OnGossipSelect)
