#include "GameClient.h"
#include "../../Shared/Source/Types/Types.h"
#include "../../Shared/Source/Spells/SpellDefines.h"
#include <Onyx.h>
#include <Source/Core/EntryPoint.h>
#include <imgui.h>
#include <cmath>
#include <chrono>
#include <thread>

using namespace MMO;

class GameLayer : public Onyx::Layer {
public:
    GameLayer() {
        m_LastFrameTime = std::chrono::steady_clock::now();
        // Initialize item templates for client-side lookups
        ItemTemplateManager::Instance().Initialize();
    }

    void OnUpdate() override {
        // Frame limiter (VSync fallback for WSL2)
        constexpr float TARGET_FPS = 60.0f;
        constexpr float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_LastFrameTime).count();

        if (dt < TARGET_FRAME_TIME) {
            auto sleepTime = std::chrono::duration<float>(TARGET_FRAME_TIME - dt);
            std::this_thread::sleep_for(sleepTime);
            now = std::chrono::steady_clock::now();
            dt = std::chrono::duration<float>(now - m_LastFrameTime).count();
        }
        m_LastFrameTime = now;

        // Calculate FPS (smoothed)
        if (dt > 0.0f) {
            m_FPS = m_FPS * 0.95f + (1.0f / dt) * 0.05f;
        }

        m_Client.Update(dt);

        // Handle input in game state
        if (m_Client.GetState() == ClientState::IN_GAME) {
            HandleGameInput(dt);
        }
    }

    void OnImGui() override {
        switch (m_Client.GetState()) {
            case ClientState::DISCONNECTED:
            case ClientState::CONNECTING_LOGIN:
                RenderConnectScreen();
                break;
            case ClientState::LOGIN_SCREEN:
                RenderLoginScreen();
                break;
            case ClientState::CHARACTER_SELECT:
                RenderCharacterSelect();
                break;
            case ClientState::CONNECTING_WORLD:
                RenderLoadingScreen();
                break;
            case ClientState::IN_GAME:
                RenderGameUI();
                RenderGameWorld();
                break;
        }

        // Show error popup
        if (!m_Client.GetLastError().empty() && !m_ShowingError) {
            m_ShowingError = true;
            m_ErrorMessage = m_Client.GetLastError();
        }
        if (m_ShowingError) {
            RenderErrorPopup();
        }
    }

private:
    void HandleGameInput(float dt) {
        m_InputTimer += dt;
        if (m_InputTimer < 1.0f / 60.0f) return;
        m_InputTimer = 0.0f;

        int8_t moveX = 0, moveY = 0;

        if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) moveY = 1;
        if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) moveY = -1;
        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) moveX = -1;
        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) moveX = 1;

        float rotation = m_Client.GetLocalPlayer().rotation;
        if (moveX != 0 || moveY != 0) {
            rotation = std::atan2(static_cast<float>(moveY), static_cast<float>(moveX));
        }

        
        m_Client.SendInput(moveX, moveY, rotation);
        // Ability keys
        const auto& abilities = m_Client.GetLocalPlayer().abilities;
        if (ImGui::IsKeyPressed(ImGuiKey_1) && abilities.size() > 0) {
            m_Client.CastAbility(abilities[0]);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_2) && abilities.size() > 1) {
            m_Client.CastAbility(abilities[1]);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_3) && abilities.size() > 2) {
            m_Client.CastAbility(abilities[2]);
        }

        // Tab to cycle targets
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            CycleTarget();
        }
    }

    void CycleTarget() {
        const auto& entities = m_Client.GetEntities();
        EntityId currentTarget = m_Client.GetLocalPlayer().targetId;

        // Find next target
        bool foundCurrent = false;
        for (const auto& [id, entity] : entities) {
            if (entity.type != EntityType::MOB) continue;

            if (currentTarget == INVALID_ENTITY_ID) {
                m_Client.SelectTarget(id);
                return;
            }

            if (foundCurrent) {
                m_Client.SelectTarget(id);
                return;
            }

            if (id == currentTarget) {
                foundCurrent = true;
            }
        }

        // Wrap around to first target
        for (const auto& [id, entity] : entities) {
            if (entity.type == EntityType::MOB) {
                m_Client.SelectTarget(id);
                return;
            }
        }
    }

    void RenderConnectScreen() {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 150));

        if (ImGui::Begin("Connect to Server", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::InputText("Host", m_LoginHost, sizeof(m_LoginHost));
            ImGui::InputInt("Port", &m_LoginPort);

            if (ImGui::Button("Connect", ImVec2(-1, 30))) {
                m_Client.ConnectToLoginServer(m_LoginHost, static_cast<uint16_t>(m_LoginPort));
            }
        }
        ImGui::End();
    }

    void RenderLoginScreen() {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 250));

        if (ImGui::Begin("Login", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::InputText("Username", m_Username, sizeof(m_Username));
            ImGui::InputText("Password", m_Password, sizeof(m_Password),
                             ImGuiInputTextFlags_Password);

            if (ImGui::Button("Login", ImVec2(-1, 30))) {
                m_Client.Login(m_Username, m_Password);
            }

            ImGui::Separator();
            ImGui::Text("Create Account:");
            ImGui::InputText("Email", m_Email, sizeof(m_Email));

            if (ImGui::Button("Register", ImVec2(-1, 30))) {
                m_Client.Register(m_Username, m_Password, m_Email);
            }
        }
        ImGui::End();
    }

    void RenderCharacterSelect() {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 400));

        if (ImGui::Begin("Character Select", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("Select a Character:");
            ImGui::Separator();

            const auto& characters = m_Client.GetCharacterList();
            for (const auto& charInfo : characters) {
                std::string className;
                switch (charInfo.characterClass) {
                    case CharacterClass::WARRIOR: className = "Warrior"; break;
                    case CharacterClass::WITCH: className = "Witch"; break;
                    default: className = "Unknown"; break;
                }

                std::string label = charInfo.name + " - Level " +
                                    std::to_string(charInfo.level) + " " + className;

                if (ImGui::Selectable(label.c_str(), m_SelectedCharIndex == charInfo.id)) {
                    m_SelectedCharIndex = charInfo.id;
                }
            }

            ImGui::Separator();

            if (m_SelectedCharIndex != 0) {
                if (ImGui::Button("Enter World", ImVec2(-1, 30))) {
                    m_Client.SelectCharacter(m_SelectedCharIndex);
                }
            }

            ImGui::Separator();
            ImGui::Text("Create New Character:");
            ImGui::InputText("Name", m_NewCharName, sizeof(m_NewCharName));

            if (ImGui::RadioButton("Warrior", m_NewCharClass == CharacterClass::WARRIOR)) {
                m_NewCharClass = CharacterClass::WARRIOR;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Witch", m_NewCharClass == CharacterClass::WITCH)) {
                m_NewCharClass = CharacterClass::WITCH;
            }

            if (ImGui::Button("Create Character", ImVec2(-1, 30))) {
                if (strlen(m_NewCharName) > 0) {
                    m_Client.CreateCharacter(m_NewCharName, m_NewCharClass);
                    memset(m_NewCharName, 0, sizeof(m_NewCharName));
                }
            }
        }
        ImGui::End();
    }

    void RenderLoadingScreen() {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(200, 60));

        if (ImGui::Begin("Loading", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoTitleBar)) {
            ImGui::Text("Connecting to World Server...");
        }
        ImGui::End();
    }

    void RenderGameWorld() {
        // Create a game viewport window
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - 20,
                                        ImGui::GetIO().DisplaySize.y - 200));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

        if (ImGui::Begin("GameWorld", nullptr, flags)) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            Vec2 center(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);

            // Camera follows player
            const auto& player = m_Client.GetLocalPlayer();
            float scale = 20.0f;  // Pixels per unit

            // Draw grid
            ImU32 gridColor = IM_COL32(50, 50, 50, 100);
            for (float x = -20; x <= 20; x += 5) {
                Vec2 start(x - player.position.x, -20 - player.position.y);
                Vec2 end(x - player.position.x, 20 - player.position.y);
                drawList->AddLine(
                    ImVec2(center.x + start.x * scale, center.y - start.y * scale),
                    ImVec2(center.x + end.x * scale, center.y - end.y * scale),
                    gridColor
                );
            }
            for (float y = -20; y <= 20; y += 5) {
                Vec2 start(-20 - player.position.x, y - player.position.y);
                Vec2 end(20 - player.position.x, y - player.position.y);
                drawList->AddLine(
                    ImVec2(center.x + start.x * scale, center.y - start.y * scale),
                    ImVec2(center.x + end.x * scale, center.y - end.y * scale),
                    gridColor
                );
            }

            // Draw portals
            for (const auto& portal : m_Client.GetPortals()) {
                Vec2 relPos = portal.position - player.position;
                ImVec2 screenPos(center.x + relPos.x * scale, center.y - relPos.y * scale);

                float halfW = (portal.size.x / 2.0f) * scale;
                float halfH = (portal.size.y / 2.0f) * scale;

                // Draw portal as a glowing rectangle
                ImU32 portalColor = IM_COL32(100, 200, 255, 150);
                ImU32 portalBorderColor = IM_COL32(50, 150, 255, 255);

                drawList->AddRectFilled(
                    ImVec2(screenPos.x - halfW, screenPos.y - halfH),
                    ImVec2(screenPos.x + halfW, screenPos.y + halfH),
                    portalColor
                );
                drawList->AddRect(
                    ImVec2(screenPos.x - halfW, screenPos.y - halfH),
                    ImVec2(screenPos.x + halfW, screenPos.y + halfH),
                    portalBorderColor, 0.0f, 0, 2.0f
                );

                // Draw destination name above portal
                std::string label = "To: " + portal.destMapName;
                drawList->AddText(
                    ImVec2(screenPos.x - label.length() * 3, screenPos.y - halfH - 15),
                    IM_COL32(200, 230, 255, 255),
                    label.c_str()
                );
            }

            // Draw entities
            for (const auto& [id, entity] : m_Client.GetEntities()) {
                Vec2 relPos = entity.position - player.position;
                ImVec2 screenPos(center.x + relPos.x * scale, center.y - relPos.y * scale);

                ImU32 color;
                float size = 15.0f;
                bool isDead = entity.health <= 0;

                switch (entity.type) {
                    case EntityType::PLAYER:
                        color = IM_COL32(0, 200, 0, 255);  // Green for other players
                        break;
                    case EntityType::MOB:
                        if (isDead) {
                            color = IM_COL32(80, 60, 60, 255);  // Dark gray for dead mobs
                        } else {
                            color = IM_COL32(200, 50, 50, 255);  // Red for living mobs
                        }
                        break;
                    default:
                        color = IM_COL32(200, 200, 200, 255);
                        break;
                }

                // Draw quad
                drawList->AddRectFilled(
                    ImVec2(screenPos.x - size, screenPos.y - size),
                    ImVec2(screenPos.x + size, screenPos.y + size),
                    color
                );

                // Draw health bar above entity (only for living entities)
                if (!isDead) {
                    float healthPercent = entity.maxHealth > 0 ?
                        static_cast<float>(entity.health) / entity.maxHealth : 0.0f;
                    drawList->AddRectFilled(
                        ImVec2(screenPos.x - 20, screenPos.y - size - 10),
                        ImVec2(screenPos.x + 20, screenPos.y - size - 5),
                        IM_COL32(100, 100, 100, 255)
                    );
                    drawList->AddRectFilled(
                        ImVec2(screenPos.x - 20, screenPos.y - size - 10),
                        ImVec2(screenPos.x - 20 + 40 * healthPercent, screenPos.y - size - 5),
                        IM_COL32(0, 200, 0, 255)
                    );
                } else if (entity.type == EntityType::MOB) {
                    // Draw loot sparkle for dead mobs
                    drawList->AddCircle(screenPos, size + 5, IM_COL32(255, 215, 0, 150), 8, 2.0f);
                }

                // Draw name
                drawList->AddText(
                    ImVec2(screenPos.x - entity.name.length() * 3, screenPos.y - size - 25),
                    isDead ? IM_COL32(128, 128, 128, 255) : IM_COL32(255, 255, 255, 255),
                    entity.name.c_str()
                );

                // Draw target indicator
                if (id == player.targetId) {
                    drawList->AddRect(
                        ImVec2(screenPos.x - size - 3, screenPos.y - size - 3),
                        ImVec2(screenPos.x + size + 3, screenPos.y + size + 3),
                        IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f
                    );
                }
            }

            // Draw local player (always at center)
            ImU32 playerColor;
            switch (player.characterClass) {
                case CharacterClass::WARRIOR:
                    playerColor = IM_COL32(100, 150, 255, 255);  // Blue for warrior
                    break;
                case CharacterClass::WITCH:
                    playerColor = IM_COL32(200, 100, 255, 255);  // Purple for witch
                    break;
                default:
                    playerColor = IM_COL32(255, 255, 255, 255);
                    break;
            }
            drawList->AddRectFilled(
                ImVec2(center.x - 15, center.y - 15),
                ImVec2(center.x + 15, center.y + 15),
                playerColor
            );

            // Draw player rotation indicator
            float dirX = std::cos(player.rotation);
            float dirY = std::sin(player.rotation);
            drawList->AddLine(
                ImVec2(center.x, center.y),
                ImVec2(center.x + dirX * 25, center.y - dirY * 25),
                IM_COL32(255, 255, 255, 255), 2.0f
            );

            // Draw projectiles
            for (const auto& proj : m_Client.GetProjectiles()) {
                Vec2 relPos = proj.position - player.position;
                ImVec2 screenPos(center.x + relPos.x * scale, center.y - relPos.y * scale);

                // Color based on ability type
                ImU32 projColor;
                float projSize = 8.0f;
                AbilityData ability = AbilityData::GetAbilityData(proj.abilityId);

                // Get damage type from first projectile effect
                DamageType damageType = DamageType::PHYSICAL;
                for (const auto& effect : ability.effects) {
                    if (effect.type == SpellEffectType::SPAWN_PROJECTILE) {
                        damageType = effect.damageType;
                        break;
                    }
                }

                switch (damageType) {
                    case DamageType::FIRE:
                        projColor = IM_COL32(255, 100, 0, 255);  // Orange for fire
                        break;
                    case DamageType::FROST:
                        projColor = IM_COL32(100, 200, 255, 255);  // Light blue for frost
                        break;
                    case DamageType::HOLY:
                        projColor = IM_COL32(255, 255, 150, 255);  // Yellow for heal
                        break;
                    default:
                        projColor = IM_COL32(255, 255, 255, 255);  // White default
                        break;
                }

                // Draw projectile as a small filled circle
                drawList->AddCircleFilled(screenPos, projSize, projColor);

                // Add a trail effect
                drawList->AddCircleFilled(screenPos, projSize * 0.6f, IM_COL32(255, 255, 255, 200));
            }

            // Click to target, loot, or use portal
            if (ImGui::IsWindowHovered() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                ImVec2 mousePos = ImGui::GetMousePos();
                Vec2 worldClick;
                worldClick.x = (mousePos.x - center.x) / scale + player.position.x;
                worldClick.y = -(mousePos.y - center.y) / scale + player.position.y;

                bool rightClick = ImGui::IsMouseClicked(1);

                // First check if clicking on a portal
                bool clickedPortal = false;
                for (const auto& portal : m_Client.GetPortals()) {
                    // Check if click is within portal bounds
                    float halfW = portal.size.x / 2.0f;
                    float halfH = portal.size.y / 2.0f;
                    if (worldClick.x >= portal.position.x - halfW &&
                        worldClick.x <= portal.position.x + halfW &&
                        worldClick.y >= portal.position.y - halfH &&
                        worldClick.y <= portal.position.y + halfH) {
                        // Check if player is close enough to portal (within 8 units)
                        float distToPortal = Vec2::Distance(player.position, portal.position);
                        if (distToPortal <= 8.0f) {
                            m_Client.UsePortal(portal.id);
                            clickedPortal = true;
                            break;
                        }
                    }
                }

                // If not clicking portal, check for entity targeting/looting
                if (!clickedPortal) {
                    EntityId closestId = INVALID_ENTITY_ID;
                    float closestDist = 2.0f;  // Click radius in world units
                    bool closestIsDead = false;

                    for (const auto& [id, entity] : m_Client.GetEntities()) {
                        float dist = Vec2::Distance(worldClick, entity.position);
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestId = id;
                            closestIsDead = (entity.health <= 0 && entity.type == EntityType::MOB);
                        }
                    }

                    if (closestId != INVALID_ENTITY_ID) {
                        if (closestIsDead && rightClick) {
                            // Right-click on dead mob to loot
                            m_Client.OpenLoot(closestId);
                        } else {
                            // Left-click or right-click on living entity to target
                            m_Client.SelectTarget(closestId);
                        }
                    } else {
                        // Clicked empty space, clear target
                        m_Client.SelectTarget(INVALID_ENTITY_ID);
                    }
                }
            }
        }
        ImGui::End();
    }

    void RenderGameUI() {
        const auto& player = m_Client.GetLocalPlayer();

        // Buff bar (above player frame)
        if (!player.auras.empty()) {
            ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 230));
            ImGui::SetNextWindowSize(ImVec2(250, 45));
            if (ImGui::Begin("BuffBar", nullptr, ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBackground)) {
                for (size_t i = 0; i < player.auras.size() && i < 8; ++i) {
                    const auto& aura = player.auras[i];
                    if (i > 0) ImGui::SameLine();

                    // Color based on buff/debuff
                    ImVec4 color = aura.IsDebuff() ?
                        ImVec4(0.8f, 0.2f, 0.2f, 1.0f) :  // Red for debuff
                        ImVec4(0.2f, 0.6f, 0.2f, 1.0f);   // Green for buff

                    ImGui::PushStyleColor(ImGuiCol_Button, color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);

                    // Get ability name for tooltip
                    AbilityData ability = AbilityData::GetAbilityData(aura.sourceAbility);
                    std::string label = ability.name.empty() ? "?" : ability.name.substr(0, 2);

                    ImGui::Button(label.c_str(), ImVec2(35, 35));

                    // Tooltip with details
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ability.name.c_str());
                        ImGui::Text("%.1fs remaining", aura.duration);
                        if (aura.value != 0) {
                            ImGui::Text("Value: %d", aura.value);
                        }
                        ImGui::EndTooltip();
                    }

                    ImGui::PopStyleColor(2);

                    // Duration bar under the button
                    float progress = aura.maxDuration > 0 ?
                        aura.duration / aura.maxDuration : 0.0f;
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
                }
            }
            ImGui::End();
        }

        // Player frame
        ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 180));
        ImGui::SetNextWindowSize(ImVec2(200, 110));
        if (ImGui::Begin("PlayerFrame", nullptr, ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("%s - Level %u", player.name.c_str(), player.level);

            // Health bar
            float healthPercent = player.maxHealth > 0 ?
                static_cast<float>(player.health) / player.maxHealth : 0.0f;
            ImGui::ProgressBar(healthPercent, ImVec2(-1, 20),
                               (std::to_string(player.health) + "/" +
                                std::to_string(player.maxHealth)).c_str());

            // Mana bar
            float manaPercent = player.maxMana > 0 ?
                static_cast<float>(player.mana) / player.maxMana : 0.0f;
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
            ImGui::ProgressBar(manaPercent, ImVec2(-1, 20),
                               (std::to_string(player.mana) + "/" +
                                std::to_string(player.maxMana)).c_str());
            ImGui::PopStyleColor();

            // XP bar (purple)
            float xpPercent = player.experienceToNext > 0 ?
                static_cast<float>(player.experience) / static_cast<float>(player.experienceToNext) : 0.0f;
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
            std::string xpText = std::to_string(player.experience) + "/" +
                                 std::to_string(player.experienceToNext) + " XP";
            ImGui::ProgressBar(xpPercent, ImVec2(-1, 15), xpText.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::End();

        // Target frame
        if (player.targetId != INVALID_ENTITY_ID) {
            const auto& entities = m_Client.GetEntities();
            auto it = entities.find(player.targetId);
            if (it != entities.end()) {
                const auto& target = it->second;

                ImGui::SetNextWindowPos(ImVec2(220, ImGui::GetIO().DisplaySize.y - 180));
                ImGui::SetNextWindowSize(ImVec2(200, 80));
                if (ImGui::Begin("TargetFrame", nullptr, ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                    ImGui::Text("%s", target.name.c_str());

                    float healthPercent = target.maxHealth > 0 ?
                        static_cast<float>(target.health) / target.maxHealth : 0.0f;
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    ImGui::ProgressBar(healthPercent, ImVec2(-1, 20),
                                       (std::to_string(target.health) + "/" +
                                        std::to_string(target.maxHealth)).c_str());
                    ImGui::PopStyleColor();
                }
                ImGui::End();
            }
        }

        // Action bar
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150,
                                       ImGui::GetIO().DisplaySize.y - 90));
        ImGui::SetNextWindowSize(ImVec2(300, 80));
        if (ImGui::Begin("ActionBar", nullptr, ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            for (size_t i = 0; i < player.abilities.size() && i < 6; ++i) {
                AbilityData ability = AbilityData::GetAbilityData(player.abilities[i]);
                auto it = player.cooldowns.find(player.abilities[i]);
                float cooldown = (it != player.cooldowns.end()) ? std::max(0.0f, it->second) : 0.0f;

                if (i > 0) ImGui::SameLine();

                ImGui::BeginGroup();
                bool onCooldown = cooldown > 0.0f;
                if (onCooldown) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                }

                std::string buttonLabel = std::to_string(i + 1) + "\n" + ability.name;
                if (ImGui::Button(buttonLabel.c_str(), ImVec2(45, 50))) {
                    m_Client.CastAbility(player.abilities[i]);
                }

                if (onCooldown) {
                    ImGui::PopStyleColor();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 55);
                    ImGui::Text("%.1f", cooldown);
                }
                ImGui::EndGroup();
            }
        }
        ImGui::End();

        // Casting bar
        if (player.isCasting) {
            AbilityData ability = AbilityData::GetAbilityData(player.castingAbilityId);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 100,
                                           ImGui::GetIO().DisplaySize.y - 130));
            ImGui::SetNextWindowSize(ImVec2(200, 30));
            if (ImGui::Begin("CastBar", nullptr, ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                ImGui::ProgressBar(player.castProgress, ImVec2(-1, 20), ability.name.c_str());
            }
            ImGui::End();
        }

        // Zone name
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 150, 10));
        ImGui::SetNextWindowSize(ImVec2(140, 30));
        if (ImGui::Begin("ZoneName", nullptr, ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBackground)) {
            ImGui::Text("%s", m_Client.GetZoneName().c_str());
        }
        ImGui::End();

        // Combat log (floating damage numbers)
        RenderCombatLog();

        // Instructions
        ImGui::SetNextWindowPos(ImVec2(10, 100));
        ImGui::SetNextWindowSize(ImVec2(200, 220));
        if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize)) {
            ImGui::Text("FPS: %.0f", m_FPS);
            ImGui::Separator();
            ImGui::Text("WASD - Move");
            ImGui::Text("Tab - Cycle targets");
            ImGui::Text("1/2/3 - Use abilities");
            ImGui::Text("Click - Select target");
            ImGui::Text("Right-click - Loot corpse");
            ImGui::Text("Click portal - Teleport");
            ImGui::Text("I - Inventory");
            ImGui::Text("C - Character/Equipment");
            ImGui::Text("F3 - Debug panel");
        }
        ImGui::End();

        // Debug panel toggle
        if (ImGui::IsKeyPressed(ImGuiKey_F3)) {
            m_ShowDebugPanel = !m_ShowDebugPanel;
        }
        if (m_ShowDebugPanel) {
            RenderDebugPanel();
        }

        // Wallet display
        RenderWallet();

        // Loot window
        if (m_Client.GetLootState().isOpen) {
            RenderLootWindow();
        }

        // Inventory window (toggle with I key)
        if (ImGui::IsKeyPressed(ImGuiKey_I)) {
            m_ShowInventory = !m_ShowInventory;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_C)) {
            m_ShowCharacter = !m_ShowCharacter;
        }
        if (m_ShowInventory) {
            RenderInventoryWindow();
        }
        if (m_ShowCharacter) {
            RenderCharacterWindow();
        }
    }

    void RenderDebugPanel() {
        const auto& player = m_Client.GetLocalPlayer();
        const auto& entities = m_Client.GetEntities();

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(310, 400), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Debug Panel", &m_ShowDebugPanel)) {
            // Local player info
            if (ImGui::CollapsingHeader("Local Player", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Entity ID: %u", player.entityId);
                ImGui::Text("Name: %s", player.name.c_str());
                ImGui::Text("Class: %s", player.characterClass == CharacterClass::WARRIOR ? "Warrior" : "Witch");
                ImGui::Text("Position: (%.2f, %.2f)", player.position.x, player.position.y);
                ImGui::Text("Health: %d / %d", player.health, player.maxHealth);
                ImGui::Text("Mana: %d / %d", player.mana, player.maxMana);
                ImGui::Text("Level: %u", player.level);
                ImGui::Text("XP: %lu / %lu", static_cast<unsigned long>(player.experience),
                            static_cast<unsigned long>(player.experienceToNext));
                ImGui::Text("Target: %u", player.targetId);
                ImGui::Text("Casting: %s", player.isCasting ? "Yes" : "No");
            }

            ImGui::Separator();

            // World entities
            if (ImGui::CollapsingHeader("World Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Total: %zu entities", entities.size());
                ImGui::Separator();

                for (const auto& [id, entity] : entities) {
                    std::string entityType;
                    switch (entity.type) {
                        case EntityType::PLAYER: entityType = "Player"; break;
                        case EntityType::MOB: entityType = "Mob"; break;
                        case EntityType::NPC: entityType = "NPC"; break;
                        default: entityType = "Unknown"; break;
                    }

                    std::string headerLabel = entity.name + " (" + entityType + ")##" + std::to_string(id);

                    bool isSelected = (m_DebugSelectedEntity == id);
                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
                    }

                    if (ImGui::CollapsingHeader(headerLabel.c_str())) {
                        m_DebugSelectedEntity = id;
                        ImGui::Indent();
                        ImGui::Text("Entity ID: %u", entity.id);
                        ImGui::Text("Position: (%.2f, %.2f)", entity.position.x, entity.position.y);
                        ImGui::Text("Rotation: %.2f", entity.rotation);

                        // Health bar
                        float healthPct = entity.maxHealth > 0 ?
                            static_cast<float>(entity.health) / entity.maxHealth : 0.0f;
                        ImGui::Text("Health:");
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        ImGui::ProgressBar(healthPct, ImVec2(-1, 16),
                            (std::to_string(entity.health) + "/" + std::to_string(entity.maxHealth)).c_str());
                        ImGui::PopStyleColor();

                        ImGui::Text("Level: %u", entity.level);

                        std::string moveStateStr;
                        switch (entity.moveState) {
                            case MoveState::IDLE: moveStateStr = "Idle"; break;
                            case MoveState::WALKING: moveStateStr = "Walking"; break;
                            case MoveState::RUNNING: moveStateStr = "Running"; break;
                            default: moveStateStr = "Unknown"; break;
                        }
                        ImGui::Text("Move State: %s", moveStateStr.c_str());

                        if (entity.isCasting) {
                            AbilityData ability = AbilityData::GetAbilityData(entity.castingAbilityId);
                            ImGui::Text("Casting: %s (%.0f%%)", ability.name.c_str(), entity.castProgress * 100);
                        }

                        // Distance from player
                        float dist = Vec2::Distance(player.position, entity.position);
                        ImGui::Text("Distance: %.2f", dist);

                        // Quick actions
                        if (ImGui::Button(("Target##" + std::to_string(id)).c_str())) {
                            m_Client.SelectTarget(id);
                        }

                        ImGui::Unindent();
                    }

                    if (isSelected) {
                        ImGui::PopStyleColor();
                    }
                }
            }
        }
        ImGui::End();
    }

    void RenderCombatLog() {
        const auto& events = m_Client.GetPendingEvents();
        const auto& player = m_Client.GetLocalPlayer();
        float scale = 20.0f;
        ImVec2 windowPos = ImVec2(10, 10);
        ImVec2 windowSize = ImVec2(ImGui::GetIO().DisplaySize.x - 20,
                                   ImGui::GetIO().DisplaySize.y - 200);
        Vec2 center(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);

        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        float currentTime = static_cast<float>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count() / 1000.0f
        );

        for (const auto& event : events) {
            if (event.type != GameEventType::DAMAGE && event.type != GameEventType::HEAL &&
                event.type != GameEventType::XP_GAIN && event.type != GameEventType::LEVEL_UP) {
                continue;
            }

            float age = currentTime - event.timestamp;
            if (age > 2.0f) continue;  // Slightly longer for XP/level up

            Vec2 relPos = event.position - player.position;
            ImVec2 screenPos(center.x + relPos.x * scale, center.y - relPos.y * scale);

            // Animate upward and fade out
            float yOffset = age * 30.0f;
            float alpha = 1.0f - (age / 2.0f);

            std::string text;
            ImU32 color;

            if (event.type == GameEventType::DAMAGE) {
                text = std::to_string(event.value);
                color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.2f, 0.2f, alpha));
            } else if (event.type == GameEventType::HEAL) {
                text = std::to_string(event.value);
                color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 1.0f, 0.2f, alpha));
            } else if (event.type == GameEventType::XP_GAIN) {
                text = "+" + std::to_string(event.value) + " XP";
                color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.8f, 0.4f, 1.0f, alpha));  // Purple
                yOffset += 20.0f;  // Show higher than damage numbers
            } else if (event.type == GameEventType::LEVEL_UP) {
                text = "LEVEL UP!";
                color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.84f, 0.0f, alpha));  // Gold
                yOffset += 40.0f;  // Show even higher
            }

            drawList->AddText(
                ImVec2(screenPos.x - text.length() * 4, screenPos.y - 40 - yOffset),
                color, text.c_str()
            );
        }
    }

    void RenderErrorPopup() {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 100));

        if (ImGui::Begin("Error", &m_ShowingError,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::TextWrapped("%s", m_ErrorMessage.c_str());
            if (ImGui::Button("OK", ImVec2(-1, 30))) {
                m_ShowingError = false;
            }
        }
        ImGui::End();
    }

    void RenderWallet() {
        const auto& player = m_Client.GetLocalPlayer();
        Currency money = Currency::FromCopper(player.money);

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 160,
                                       ImGui::GetIO().DisplaySize.y - 50));
        ImGui::SetNextWindowSize(ImVec2(150, 40));

        if (ImGui::Begin("Wallet", nullptr, ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            // Gold (yellow)
            ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%ug", money.GetGold());
            ImGui::SameLine();
            // Silver (gray)
            ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "%us", money.GetSilver());
            ImGui::SameLine();
            // Copper (bronze/orange)
            ImGui::TextColored(ImVec4(0.72f, 0.45f, 0.2f, 1.0f), "%uc", money.GetCopper());
        }
        ImGui::End();
    }

    void RenderLootWindow() {
        const auto& lootState = m_Client.GetLootState();

        // Calculate window height based on content
        int contentRows = (lootState.money > 0 ? 1 : 0) + static_cast<int>(lootState.items.size());
        float windowHeight = 60.0f + contentRows * 35.0f;
        if (contentRows == 0) windowHeight = 100.0f;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(220, windowHeight));
        ImGui::SetNextWindowFocus();

        bool windowOpen = true;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoCollapse;
        if (ImGui::Begin("Loot", &windowOpen, flags)) {
            // Find corpse name
            std::string corpseName = "Corpse";
            const auto& entities = m_Client.GetEntities();
            auto it = entities.find(lootState.corpseId);
            if (it != entities.end()) {
                corpseName = it->second.name;
            }
            ImGui::Text("%s", corpseName.c_str());
            ImGui::Separator();

            bool hasLoot = false;

            // Display money if any
            if (lootState.money > 0) {
                hasLoot = true;
                Currency money = Currency::FromCopper(lootState.money);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.25f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.35f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.4f, 0.25f, 1.0f));

                // Format money string
                std::string moneyStr;
                if (money.GetGold() > 0) {
                    moneyStr += std::to_string(money.GetGold()) + "g ";
                }
                if (money.GetSilver() > 0 || money.GetGold() > 0) {
                    moneyStr += std::to_string(money.GetSilver()) + "s ";
                }
                moneyStr += std::to_string(money.GetCopper()) + "c";

                if (ImGui::Button(moneyStr.c_str(), ImVec2(-1, 30))) {
                    m_Client.TakeLootMoney();
                }
                ImGui::PopStyleColor(3);
            }

            // Display items
            for (uint8_t i = 0; i < lootState.items.size(); ++i) {
                hasLoot = true;
                const auto& lootItem = lootState.items[i];
                const ItemTemplate* tmpl = ItemTemplateManager::Instance().GetTemplate(lootItem.templateId);

                // Get quality color
                uint32_t qualityColorU32 = GetQualityColor(tmpl ? tmpl->quality : ItemQuality::COMMON);
                float r = ((qualityColorU32 >> 16) & 0xFF) / 255.0f;
                float g = ((qualityColorU32 >> 8) & 0xFF) / 255.0f;
                float b = (qualityColorU32 & 0xFF) / 255.0f;
                ImVec4 qualityColor = ImVec4(r, g, b, 1.0f);

                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r * 0.3f, g * 0.3f, b * 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 0.5f, g * 0.5f, b * 0.5f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.7f, g * 0.7f, b * 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, qualityColor);

                std::string itemName = tmpl ? tmpl->name : "Unknown Item";
                if (lootItem.stackCount > 1) {
                    itemName += " x" + std::to_string(lootItem.stackCount);
                }

                if (ImGui::Button(itemName.c_str(), ImVec2(-1, 30))) {
                    m_Client.TakeLootItem(i);
                }

                ImGui::PopStyleColor(4);
                ImGui::PopID();

                // Tooltip on hover
                if (ImGui::IsItemHovered() && tmpl) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(qualityColor, "%s", tmpl->name.c_str());
                    if (!tmpl->description.empty()) {
                        ImGui::TextWrapped("%s", tmpl->description.c_str());
                    }
                    ImGui::EndTooltip();
                }
            }

            if (!hasLoot) {
                ImGui::TextDisabled("Empty");
            }
        }
        ImGui::End();

        // Close loot window if user clicked X or pressed Escape
        if (!windowOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_Client.CloseLoot();
        }
    }

    void RenderInventoryWindow() {
        const auto& inventory = m_Client.GetInventory();

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 260, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 400));

        if (ImGui::Begin("Inventory (I)", &m_ShowInventory)) {
            // Render 4 columns x 5 rows of inventory slots
            constexpr int COLS = 4;
            constexpr float SLOT_SIZE = 50.0f;
            constexpr float SLOT_PADDING = 5.0f;

            for (uint8_t i = 0; i < INVENTORY_SIZE; ++i) {
                if (i % COLS != 0) ImGui::SameLine();

                ImGui::PushID(i);

                const auto& slot = inventory.slots[i];
                bool hasItem = !slot.IsEmpty();

                // Get item template if we have an item
                const ItemTemplate* tmpl = nullptr;
                if (hasItem) {
                    tmpl = ItemTemplateManager::Instance().GetTemplate(slot.templateId);
                }

                // Slot button with item color
                ImVec4 buttonColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
                if (tmpl) {
                    uint32_t qualityColor = GetQualityColor(tmpl->quality);
                    buttonColor = ImVec4(
                        ((qualityColor >> 16) & 0xFF) / 255.0f * 0.6f,
                        ((qualityColor >> 8) & 0xFF) / 255.0f * 0.6f,
                        (qualityColor & 0xFF) / 255.0f * 0.6f,
                        1.0f
                    );
                }

                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

                std::string label = hasItem && tmpl ? tmpl->name.substr(0, 6) : "";
                if (ImGui::Button(label.c_str(), ImVec2(SLOT_SIZE, SLOT_SIZE))) {
                    m_SelectedInventorySlot = i;
                }

                ImGui::PopStyleColor();

                // Highlight selected slot
                if (m_SelectedInventorySlot == i) {
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddRect(min, max, IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);
                }

                // Tooltip
                if (hasItem && tmpl && ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImColor(GetQualityColor(tmpl->quality)).Value, "%s", tmpl->name.c_str());
                    ImGui::TextWrapped("%s", tmpl->description.c_str());

                    if (tmpl->IsWeapon()) {
                        ImGui::Text("Damage: %.0f - %.0f", tmpl->weaponDamageMin, tmpl->weaponDamageMax);
                        ImGui::Text("Speed: %.1f", tmpl->weaponSpeed);
                    }
                    if (tmpl->armor > 0) {
                        ImGui::Text("Armor: %d", tmpl->armor);
                    }
                    for (uint8_t j = 0; j < tmpl->statCount; ++j) {
                        const char* statName = "";
                        switch (tmpl->stats[j].stat) {
                            case StatType::STRENGTH: statName = "Strength"; break;
                            case StatType::AGILITY: statName = "Agility"; break;
                            case StatType::STAMINA: statName = "Stamina"; break;
                            case StatType::INTELLECT: statName = "Intellect"; break;
                            case StatType::SPIRIT: statName = "Spirit"; break;
                            default: break;
                        }
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "+%d %s", tmpl->stats[j].value, statName);
                    }
                    if (tmpl->requiredLevel > 1) {
                        ImGui::Text("Requires Level %d", tmpl->requiredLevel);
                    }
                    ImGui::EndTooltip();
                }

                // Drag source
                if (hasItem && ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("INVENTORY_SLOT", &i, sizeof(uint8_t));
                    ImGui::Text("%s", tmpl ? tmpl->name.c_str() : "Item");
                    ImGui::EndDragDropSource();
                }

                // Drop target
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("INVENTORY_SLOT")) {
                        uint8_t srcSlot = *(const uint8_t*)payload->Data;
                        if (srcSlot != i) {
                            m_Client.SwapInventorySlots(srcSlot, i);
                        }
                    }
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EQUIPMENT_SLOT")) {
                        EquipmentSlot srcSlot = *(const EquipmentSlot*)payload->Data;
                        m_Client.UnequipItem(srcSlot);
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::PopID();
            }

            // Equip button for selected item
            if (m_SelectedInventorySlot >= 0 && m_SelectedInventorySlot < INVENTORY_SIZE) {
                const auto& slot = inventory.slots[m_SelectedInventorySlot];
                if (!slot.IsEmpty()) {
                    const ItemTemplate* tmpl = ItemTemplateManager::Instance().GetTemplate(slot.templateId);
                    if (tmpl && tmpl->IsEquippable()) {
                        ImGui::Separator();
                        if (ImGui::Button("Equip", ImVec2(-1, 25))) {
                            m_Client.EquipItem(m_SelectedInventorySlot, tmpl->equipSlot);
                            m_SelectedInventorySlot = -1;
                        }
                    }
                }
            }
        }
        ImGui::End();
    }

    void RenderCharacterWindow() {
        const auto& inventory = m_Client.GetInventory();
        const auto& stats = m_Client.GetCharacterStats();
        const auto& player = m_Client.GetLocalPlayer();

        ImGui::SetNextWindowPos(ImVec2(50, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 450));

        if (ImGui::Begin("Character (C)", &m_ShowCharacter)) {
            // Character name and class
            ImGui::Text("%s", player.name.c_str());
            ImGui::Text("Level %d %s", player.level,
                player.characterClass == CharacterClass::WARRIOR ? "Warrior" : "Witch");
            ImGui::Separator();

            // Equipment slots (arranged like a paper doll)
            ImGui::Text("Equipment:");

            const char* slotNames[] = {
                "Head", "Neck", "Shoulders", "Back", "Chest",
                "Wrists", "Hands", "Waist", "Legs", "Feet",
                "Ring 1", "Ring 2", "Weapon", "Offhand"
            };

            constexpr float EQUIP_SLOT_SIZE = 45.0f;

            for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; ++i) {
                EquipmentSlot slot = static_cast<EquipmentSlot>(i);
                const auto& equipSlot = inventory.equipment[i];
                bool hasItem = !equipSlot.IsEmpty();

                const ItemTemplate* tmpl = nullptr;
                if (hasItem) {
                    tmpl = ItemTemplateManager::Instance().GetTemplate(equipSlot.templateId);
                }

                // Render slot
                ImGui::PushID(100 + i);

                ImVec4 buttonColor = ImVec4(0.15f, 0.15f, 0.25f, 1.0f);
                if (tmpl) {
                    uint32_t qualityColor = GetQualityColor(tmpl->quality);
                    buttonColor = ImVec4(
                        ((qualityColor >> 16) & 0xFF) / 255.0f * 0.5f,
                        ((qualityColor >> 8) & 0xFF) / 255.0f * 0.5f,
                        (qualityColor & 0xFF) / 255.0f * 0.5f,
                        1.0f
                    );
                }

                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

                std::string label = hasItem && tmpl ? tmpl->name.substr(0, 5) : slotNames[i];
                if (ImGui::Button(label.c_str(), ImVec2(EQUIP_SLOT_SIZE, EQUIP_SLOT_SIZE))) {
                    if (hasItem) {
                        m_Client.UnequipItem(slot);
                    }
                }

                ImGui::PopStyleColor();

                // Same line for pairs
                if (i % 2 == 0 && i < EQUIPMENT_SLOT_COUNT - 1) {
                    ImGui::SameLine();
                }

                // Tooltip
                if (hasItem && tmpl && ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImColor(GetQualityColor(tmpl->quality)).Value, "%s", tmpl->name.c_str());
                    ImGui::TextWrapped("%s", tmpl->description.c_str());
                    if (tmpl->IsWeapon()) {
                        ImGui::Text("Damage: %.0f - %.0f", tmpl->weaponDamageMin, tmpl->weaponDamageMax);
                    }
                    if (tmpl->armor > 0) {
                        ImGui::Text("Armor: %d", tmpl->armor);
                    }
                    for (uint8_t j = 0; j < tmpl->statCount; ++j) {
                        const char* statName = "";
                        switch (tmpl->stats[j].stat) {
                            case StatType::STRENGTH: statName = "Strength"; break;
                            case StatType::AGILITY: statName = "Agility"; break;
                            case StatType::STAMINA: statName = "Stamina"; break;
                            case StatType::INTELLECT: statName = "Intellect"; break;
                            case StatType::SPIRIT: statName = "Spirit"; break;
                            default: break;
                        }
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "+%d %s", tmpl->stats[j].value, statName);
                    }
                    ImGui::Text("Click to unequip");
                    ImGui::EndTooltip();
                }

                // Drag source
                if (hasItem && ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("EQUIPMENT_SLOT", &slot, sizeof(EquipmentSlot));
                    ImGui::Text("%s", tmpl ? tmpl->name.c_str() : slotNames[i]);
                    ImGui::EndDragDropSource();
                }

                // Drop target - accept inventory items that match this slot
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("INVENTORY_SLOT")) {
                        uint8_t srcSlot = *(const uint8_t*)payload->Data;
                        m_Client.EquipItem(srcSlot, slot);
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::PopID();
            }

            ImGui::Separator();

            // Stats display
            ImGui::Text("Attributes:");
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Strength: %.0f", stats.strength);
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Agility: %.0f", stats.agility);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "Stamina: %.0f", stats.stamina);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Intellect: %.0f", stats.intellect);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Spirit: %.0f", stats.spirit);

            ImGui::Separator();
            ImGui::Text("Combat:");
            ImGui::Text("Attack Power: %.0f", stats.attackPower);
            ImGui::Text("Spell Power: %.0f", stats.spellPower);
            ImGui::Text("Armor: %d", stats.armor);
            ImGui::Text("Damage: %.0f - %.0f", stats.meleeDamageMin, stats.meleeDamageMax);
            ImGui::Text("Attack Speed: %.1f", stats.weaponSpeed);
        }
        ImGui::End();
    }

private:
    GameClient m_Client;

    // Connection
    char m_LoginHost[64] = "127.0.0.1";
    int m_LoginPort = 7000;

    // Login
    char m_Username[32] = "";
    char m_Password[32] = "";
    char m_Email[64] = "";

    // Character creation
    char m_NewCharName[32] = "";
    CharacterClass m_NewCharClass = CharacterClass::WARRIOR;
    CharacterId m_SelectedCharIndex = 0;

    // Error display
    bool m_ShowingError = false;
    std::string m_ErrorMessage;

    // Input
    float m_InputTimer = 0.0f;

    // Timing
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_FPS = 0.0f;

    // Debug panel
    bool m_ShowDebugPanel = false;
    EntityId m_DebugSelectedEntity = INVALID_ENTITY_ID;

    // Inventory/Equipment UI
    bool m_ShowInventory = false;
    bool m_ShowCharacter = false;
    int m_SelectedInventorySlot = -1;
};

class MMOClientApp : public Onyx::Application {
public:
    MMOClientApp(Onyx::ApplicationSpec& spec) : Application(spec) {
        PushLayer(new GameLayer());
    }
};

static Onyx::ApplicationSpec s_AppSpec = { 1280, 720, "MMO Client" };

Onyx::Application* Onyx::CreateApplication() {
    return new MMOClientApp(s_AppSpec);
}
