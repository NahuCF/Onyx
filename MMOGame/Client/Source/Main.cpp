#include "GameClient.h"
#include "Rendering/GameRenderer.h"
#include "Terrain/ClientTerrainSystem.h"
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
        // 3D rendering
        m_Renderer = std::make_unique<GameRenderer>();
        m_TerrainSystem = std::make_unique<ClientTerrainSystem>();
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

        // Calculate FPS (smoothed) and show in window title
        if (dt > 0.0f) {
            m_FPS = m_FPS * 0.95f + (1.0f / dt) * 0.05f;
        }
        m_FPSTitleTimer += dt;
        if (m_FPSTitleTimer >= 0.25f) {
            m_FPSTitleTimer = 0.0f;
            char title[64];
            snprintf(title, sizeof(title), "MMO Client - %.0f FPS", m_FPS);
            auto& window = *Onyx::Application::GetInstance().GetWindow();
            glfwSetWindowTitle(window.GetWindow(), title);
        }

        m_Client.Update(dt);

        // Lazy-init renderer (needs valid GL context)
        if (!m_RendererInitialized) {
            m_Renderer->Init();
            m_RendererInitialized = true;
        }

        // Handle input and render 3D world in game state
        if (m_Client.GetState() == ClientState::IN_GAME) {
            HandleGameInput(dt);

            // Load terrain when zone changes
            uint32_t mapId = m_Client.GetMapId();
            if (mapId != m_CurrentMapId && mapId != 0) {
                m_TerrainSystem->UnloadZone();
                char mapDir[16];
                snprintf(mapDir, sizeof(mapDir), "maps/%03u", mapId);
                m_TerrainSystem->LoadZone(mapId, mapDir);
                m_CurrentMapId = mapId;
            }

            // Render 3D world directly to the backbuffer
            auto& window = *Onyx::Application::GetInstance().GetWindow();
            uint32_t vpW = static_cast<uint32_t>(window.Width());
            uint32_t vpH = static_cast<uint32_t>(window.Height());

            const auto& player = m_Client.GetLocalPlayer();
            float playerHeight = m_TerrainSystem->GetHeightAt(player.position.x, player.position.y);
            glm::vec3 player3D(player.position.x, playerHeight, player.position.y);

            m_Renderer->BeginFrame(player3D, dt, vpW, vpH);
            m_Renderer->RenderTerrain(*m_TerrainSystem);
            m_Renderer->RenderEntities(player, m_Client.GetEntities(),
                                        m_Client.GetPortals(), m_Client.GetProjectiles(),
                                        *m_TerrainSystem);
            m_Renderer->EndFrame();
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

        // Camera rotation (Q/E)
        if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
            m_Renderer->GetCamera().RotateYaw(-45.0f);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E)) {
            m_Renderer->GetCamera().RotateYaw(45.0f);
        }

        // Camera zoom (scroll wheel)
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.0f) {
            m_Renderer->GetCamera().Zoom(-scroll * 5.0f);
        }

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
                std::string raceName = GetRaceName(charInfo.characterRace);
                std::string className;
                switch (charInfo.characterClass) {
                    case CharacterClass::WARRIOR: className = "Warrior"; break;
                    case CharacterClass::WITCH: className = "Witch"; break;
                    default: className = "Unknown"; break;
                }

                std::string label = charInfo.name + " - Level " +
                                    std::to_string(charInfo.level) + " " + raceName + " " + className;

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

            ImGui::Text("Race:");
            if (ImGui::RadioButton("Human", m_NewCharRace == CharacterRace::HUMAN)) {
                m_NewCharRace = CharacterRace::HUMAN;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Orc", m_NewCharRace == CharacterRace::ORC)) {
                m_NewCharRace = CharacterRace::ORC;
            }

            ImGui::Text("Class:");
            if (ImGui::RadioButton("Warrior", m_NewCharClass == CharacterClass::WARRIOR)) {
                m_NewCharClass = CharacterClass::WARRIOR;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Witch", m_NewCharClass == CharacterClass::WITCH)) {
                m_NewCharClass = CharacterClass::WITCH;
            }

            if (ImGui::Button("Create Character", ImVec2(-1, 30))) {
                if (strlen(m_NewCharName) > 0) {
                    m_Client.CreateCharacter(m_NewCharName, m_NewCharRace, m_NewCharClass);
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

    // 3D world rendering now happens in OnUpdate() directly to the backbuffer


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
    CharacterRace m_NewCharRace = CharacterRace::HUMAN;
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
    float m_FPSTitleTimer = 0.0f;

    // 3D Rendering
    std::unique_ptr<GameRenderer> m_Renderer;
    std::unique_ptr<ClientTerrainSystem> m_TerrainSystem;
    uint32_t m_CurrentMapId = 0;
    bool m_RendererInitialized = false;
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
