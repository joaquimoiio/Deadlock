#include <windows.h>
#include <thread>
#include <chrono>
#include "core/memory.h"
#include "core/offsets.h"
#include "features/aimbot.h"
#include "features/esp.h"
#include "features/radar.h"
#include "render/overlay.h"
#include "render/imgui_menu.h"

// Globals
Memory g_memory;
OffsetManager g_offsets;
Aimbot* g_aimbot = nullptr;
ESP* g_esp = nullptr;
Radar* g_radar = nullptr;
Overlay* g_overlay = nullptr;
ImGuiMenu* g_menu = nullptr;

bool g_aimbotEnabled = true;
bool g_menuVisible = true;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_menu && g_menu->IsMenuVisible()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
    }
    
    switch (msg) {
        case WM_KEYDOWN:
            switch (wParam) {
                case VK_F1:
                    g_aimbotEnabled = !g_aimbotEnabled;
                    if (g_aimbot) {
                        if (g_aimbotEnabled) g_aimbot->Resume();
                        else g_aimbot->Pause();
                    }
                    break;
                case VK_INSERT:
                    if (g_menu) g_menu->ToggleMenu();
                    break;
                case VK_END:
                    PostQuitMessage(0);
                    break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Load offsets
    if (!g_offsets.LoadFromFile("config/offsets.json")) {
        MessageBox(NULL, L"Failed to load offsets! Run dumper first.", L"Error", MB_OK);
        return 1;
    }
    
    // Attach to Deadlock
    if (!g_memory.Attach("deadlock.exe")) {
        MessageBox(NULL, L"Deadlock not running!", L"Error", MB_OK);
        return 1;
    }
    
    // Create overlay
    g_overlay = new Overlay();
    if (!g_overlay->Create(hInstance, WindowProc)) {
        MessageBox(NULL, L"Failed to create overlay!", L"Error", MB_OK);
        return 1;
    }
    
    g_overlay->InitImGui();
    
    // Get screen size for ESP
    int screenWidth, screenHeight;
    g_overlay->GetScreenSize(screenWidth, screenHeight);
    
    // Create features
    g_aimbot = new Aimbot(g_memory, g_offsets);
    g_esp = new ESP(g_memory, g_offsets);
    g_radar = new Radar(g_memory, g_offsets);
    g_menu = new ImGuiMenu();
    
    g_esp->SetScreenSize(screenWidth, screenHeight);
    
    // Load saved config
    g_menu->LoadFromFile("config/settings.json");
    
    // Apply config to aimbot
    AimbotConfig aimbotConfig;
    aimbotConfig.enabled = g_aimbotEnabled;
    aimbotConfig.headshotProbability = g_menu->GetHeadshotProb();
    aimbotConfig.smoothSpeed = g_menu->GetSmoothSpeed();
    aimbotConfig.targetSelectType = g_menu->GetTargetSelect();
    aimbotConfig.aimKey = g_menu->GetAimKey();
    aimbotConfig.aimAtHead = g_menu->GetAimAtHead();
    aimbotConfig.headshotOnAcquire = g_menu->GetHeadshotOnAcquire();
    aimbotConfig.fovRadius = g_menu->GetFOVRadius();
    // Anti-Flag config
    aimbotConfig.adaptiveHSEnabled = g_menu->IsAdaptiveHSEnabled();
    aimbotConfig.maxConsecutiveHS = g_menu->GetMaxConsecutiveHS();
    aimbotConfig.minTimeBetweenHS = g_menu->GetMinTimeBetweenHS();
    aimbotConfig.missChance = g_menu->GetMissChance();
    aimbotConfig.reactionTimeMin = g_menu->GetReactionTimeMin();
    aimbotConfig.reactionTimeMax = g_menu->GetReactionTimeMax();
    aimbotConfig.flickEnabled = g_menu->IsFlickEnabled();
    aimbotConfig.flickAmount = g_menu->GetFlickAmount();
    aimbotConfig.maxHSRatePerMinute = g_menu->GetMaxHSRatePerMinute();
    aimbotConfig.cooldownAfterHighHS = g_menu->GetCooldownAfterHighHS();
    aimbotConfig.burstFireEnabled = g_menu->IsBurstFireEnabled();
    g_aimbot->SetConfig(aimbotConfig);
    
    // Start aimbot
    if (g_aimbotEnabled) {
        g_aimbot->Start();
    }
    
    // Main loop
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        g_overlay->BeginFrame();
        
        // Update config in real-time
        AimbotConfig newConfig;
        newConfig.enabled = g_aimbotEnabled;
        newConfig.headshotProbability = g_menu->GetHeadshotProb();
        newConfig.smoothSpeed = g_menu->GetSmoothSpeed();
        newConfig.targetSelectType = g_menu->GetTargetSelect();
        newConfig.aimAtHead = g_menu->GetAimAtHead();
        newConfig.headshotOnAcquire = g_menu->GetHeadshotOnAcquire();
        newConfig.fovRadius = g_menu->GetFOVRadius();
        newConfig.adaptiveHSEnabled = g_menu->IsAdaptiveHSEnabled();
        newConfig.maxConsecutiveHS = g_menu->GetMaxConsecutiveHS();
        newConfig.minTimeBetweenHS = g_menu->GetMinTimeBetweenHS();
        newConfig.missChance = g_menu->GetMissChance();
        newConfig.reactionTimeMin = g_menu->GetReactionTimeMin();
        newConfig.reactionTimeMax = g_menu->GetReactionTimeMax();
        newConfig.flickEnabled = g_menu->IsFlickEnabled();
        newConfig.flickAmount = g_menu->GetFlickAmount();
        newConfig.maxHSRatePerMinute = g_menu->GetMaxHSRatePerMinute();
        newConfig.cooldownAfterHighHS = g_menu->GetCooldownAfterHighHS();
        newConfig.burstFireEnabled = g_menu->IsBurstFireEnabled();
        g_aimbot->SetConfig(newConfig);
        
        // Update ESP config
        ESPConfig espConfig;
        espConfig.enabled = g_menu->IsESPEnabled();
        espConfig.boxes = g_menu->GetESPBones();
        espConfig.lines = g_menu->GetESPLines();
        espConfig.healthBars = g_menu->GetESPHealth();
        espConfig.names = g_menu->GetESPNames();
        espConfig.distance = g_menu->GetESPDistance();
        espConfig.headDot = g_menu->GetESPHeadDot();
        espConfig.maxDistance = g_menu->GetMaxESPDistance();
        memcpy(espConfig.enemyColor, g_menu->GetEnemyColor(), sizeof(float) * 4);
        memcpy(espConfig.teammateColor, g_menu->GetTeammateColor(), sizeof(float) * 4);
        g_esp->SetConfig(espConfig);
        
        // Update Radar config
        RadarConfig radarConfig;
        radarConfig.enabled = g_menu->IsRadarEnabled();
        radarConfig.size = g_menu->GetRadarSize();
        radarConfig.range = g_menu->GetRadarRange();
        radarConfig.position = g_menu->GetRadarPosition();
        g_radar->SetConfig(radarConfig);
        
        // Render menu
        if (g_menuVisible) {
            g_menu->Render();
        }
        
        // Render ESP and Radar
        g_esp->Render();
        g_radar->Render();
        
        g_overlay->EndFrame();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Cleanup
    if (g_aimbot) {
        g_aimbot->Stop();
        delete g_aimbot;
    }
    delete g_esp;
    delete g_radar;
    delete g_menu;
    delete g_overlay;
    
    return 0;
}