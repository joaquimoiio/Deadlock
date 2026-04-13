#include "imgui_menu.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

ImGuiMenu::ImGuiMenu() 
    : m_showMenu(true), m_currentTab(0),
      // Aimbot defaults
      m_aimbotEnabled(true), m_headshotProb(0.25f), m_smoothSpeed(5.0f),
      m_targetSelect(0), m_aimKey(VK_LBUTTON), m_aimAtHead(true),
      m_headshotOnAcquire(true), m_fovRadius(100.0f),
      // Anti-Flag defaults
      m_adaptiveHSEnabled(true), m_maxConsecutiveHS(3), m_minTimeBetweenHS(0.5f),
      m_missChance(0.05f), m_reactionTimeMin(0.08f), m_reactionTimeMax(0.25f),
      m_flickEnabled(true), m_flickAmount(5.0f), m_maxHSRatePerMinute(4),
      m_cooldownAfterHighHS(5.0f), m_burstFireEnabled(true),
      // ESP defaults
      m_espEnabled(true), m_espBoxes(true), m_espLines(true),
      m_espHealth(true), m_espNames(true), m_espDistance(true),
      m_espHeadDot(true), m_maxESPDistance(5000.0f),
      // Radar defaults
      m_radarEnabled(true), m_radarSize(200.0f), m_radarRange(3000.0f), m_radarPosition(0) {
    
    m_enemyColor[0] = 1.0f; m_enemyColor[1] = 0.0f; m_enemyColor[2] = 0.0f; m_enemyColor[3] = 1.0f;
    m_teammateColor[0] = 0.0f; m_teammateColor[1] = 1.0f; m_teammateColor[2] = 0.0f; m_teammateColor[3] = 1.0f;
    m_boxColor[0] = 1.0f; m_boxColor[1] = 1.0f; m_boxColor[2] = 1.0f; m_boxColor[3] = 1.0f;
}

ImGuiMenu::~ImGuiMenu() {}

void ImGuiMenu::Render() {
    if (!m_showMenu) return;
    
    ImGui::SetNextWindowSize(ImVec2(600, 650), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("DeadUnlock Control Panel", &m_showMenu, ImGuiWindowFlags_NoCollapse)) {
        
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("🎯 Aimbot")) {
                DrawAimbotTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("🛡️ Anti-Flag")) {
                DrawAntiFlagTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("👁️ ESP")) {
                DrawESPTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("📡 Radar")) {
                DrawRadarTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        DrawStatusBar();
        
        // Save/Load buttons
        ImGui::Separator();
        if (ImGui::Button("Save Config", ImVec2(100, 0))) {
            SaveToFile("config/settings.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Config", ImVec2(100, 0))) {
            LoadFromFile("config/settings.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Anti-Flag", ImVec2(120, 0))) {
            // Reset aggression would be called from main
        }
    }
    ImGui::End();
}

void ImGuiMenu::DrawAimbotTab() {
    ImGui::Checkbox("Enable Aimbot", &m_aimbotEnabled);
    ImGui::Separator();
    
    ImGui::Text("Target Selection");
    ImGui::RadioButton("FOV", &m_targetSelect, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Distance", &m_targetSelect, 1);
    
    if (m_targetSelect == 0) {
        ImGui::SliderFloat("FOV Radius", &m_fovRadius, 50.0f, 300.0f, "%.0f pixels");
    }
    
    ImGui::Separator();
    ImGui::SliderFloat("Smooth Speed", &m_smoothSpeed, 1.0f, 15.0f, "%.1f deg/frame");
    ImGui::SliderFloat("Base Headshot %", &m_headshotProb, 0.0f, 1.0f, "%.0f%%");
    ImGui::Checkbox("Force headshot on acquire", &m_headshotOnAcquire);
    ImGui::Checkbox("Always aim at head", &m_aimAtHead);
    
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️ Anti-Flag will dynamically adjust HS%");
}

void ImGuiMenu::DrawAntiFlagTab() {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🛡️ HUMANIZATION SYSTEM");
    ImGui::Separator();
    
    ImGui::Checkbox("Adaptive Headshot", &m_adaptiveHSEnabled);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically reduces HS% if you're getting too many headshots");
    }
    
    ImGui::SliderInt("Max Consecutive HS", &m_maxConsecutiveHS, 1, 10);
    ImGui::SliderFloat("Min Time Between HS (s)", &m_minTimeBetweenHS, 0.1f, 2.0f);
    ImGui::SliderFloat("Miss Chance", &m_missChance, 0.0f, 0.25f, "%.1f%%");
    
    ImGui::Separator();
    ImGui::Text("Reaction Time");
    ImGui::SliderFloat("Min (ms)", &m_reactionTimeMin, 0.05f, 0.3f, "%.0f ms");
    ImGui::SliderFloat("Max (ms)", &m_reactionTimeMax, 0.1f, 0.5f, "%.0f ms");
    
    ImGui::Separator();
    ImGui::Checkbox("Flick Simulation", &m_flickEnabled);
    ImGui::SliderFloat("Flick Amount (px)", &m_flickAmount, 1.0f, 20.0f);
    
    ImGui::Separator();
    ImGui::Text("Rate Limiting");
    ImGui::SliderInt("Max HS Per Minute", &m_maxHSRatePerMinute, 1, 15);
    ImGui::SliderFloat("Cooldown After High HS (s)", &m_cooldownAfterHighHS, 2.0f, 10.0f);
    ImGui::Checkbox("Burst Fire Simulation", &m_burstFireEnabled);
    
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), 
        "Current: %d HS in last minute | %d consecutive", 
        m_maxHSRatePerMinute, m_maxConsecutiveHS);
}

void ImGuiMenu::DrawESPTab() {
    ImGui::Checkbox("Enable ESP", &m_espEnabled);
    ImGui::Separator();
    
    ImGui::Checkbox("Boxes (2D)", &m_espBoxes);
    ImGui::SameLine();
    ImGui::Checkbox("Health Bars", &m_espHealth);
    ImGui::Checkbox("Names", &m_espNames);
    ImGui::SameLine();
    ImGui::Checkbox("Distance", &m_espDistance);
    ImGui::Checkbox("Lines to crosshair", &m_espLines);
    ImGui::SameLine();
    ImGui::Checkbox("Head Dot", &m_espHeadDot);
    
    ImGui::Separator();
    ImGui::Text("Colors");
    ImGui::ColorEdit4("Enemy Color", m_enemyColor);
    ImGui::ColorEdit4("Teammate Color", m_teammateColor);
    ImGui::ColorEdit4("Box Outline Color", m_boxColor);
    
    ImGui::Separator();
    ImGui::SliderFloat("Max Render Distance", &m_maxESPDistance, 1000.0f, 10000.0f, "%.0f units");
}

void ImGuiMenu::DrawRadarTab() {
    ImGui::Checkbox("Enable Radar", &m_radarEnabled);
    ImGui::Separator();
    
    ImGui::SliderFloat("Radar Size", &m_radarSize, 100.0f, 400.0f, "%.0f px");
    ImGui::SliderFloat("Radar Range", &m_radarRange, 1000.0f, 5000.0f, "%.0f units");
    
    const char* positions[] = { "Top-Right", "Top-Left", "Bottom-Right", "Bottom-Left" };
    ImGui::Combo("Position", &m_radarPosition, positions, 4);
}

void ImGuiMenu::DrawStatusBar() {
    ImGui::Separator();
    
    ImGui::Text("Status: ");
    ImGui::SameLine();
    
    if (m_aimbotEnabled) ImGui::TextColored(ImVec4(0,1,0,1), "Aimbot ON ");
    else ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "Aimbot OFF ");
    
    ImGui::SameLine();
    if (m_espEnabled) ImGui::TextColored(ImVec4(0,1,0,1), "| ESP ON ");
    else ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "| ESP OFF ");
    
    ImGui::SameLine();
    if (m_radarEnabled) ImGui::TextColored(ImVec4(0,1,0,1), "| Radar ON");
    else ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "| Radar OFF");
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
}

void ImGuiMenu::SaveToFile(const std::string& path) {
    nlohmann::json j;
    
    j["aimbot"]["enabled"] = m_aimbotEnabled;
    j["aimbot"]["headshot_prob"] = m_headshotProb;
    j["aimbot"]["smooth_speed"] = m_smoothSpeed;
    j["aimbot"]["target_select"] = m_targetSelect;
    j["aimbot"]["aim_key"] = m_aimKey;
    j["aimbot"]["aim_at_head"] = m_aimAtHead;
    j["aimbot"]["headshot_on_acquire"] = m_headshotOnAcquire;
    j["aimbot"]["fov_radius"] = m_fovRadius;
    
    j["anti_flag"]["adaptive_hs"] = m_adaptiveHSEnabled;
    j["anti_flag"]["max_consecutive_hs"] = m_maxConsecutiveHS;
    j["anti_flag"]["min_time_between_hs"] = m_minTimeBetweenHS;
    j["anti_flag"]["miss_chance"] = m_missChance;
    j["anti_flag"]["reaction_time_min"] = m_reactionTimeMin;
    j["anti_flag"]["reaction_time_max"] = m_reactionTimeMax;
    j["anti_flag"]["flick_enabled"] = m_flickEnabled;
    j["anti_flag"]["flick_amount"] = m_flickAmount;
    j["anti_flag"]["max_hs_per_minute"] = m_maxHSRatePerMinute;
    j["anti_flag"]["cooldown_after_high_hs"] = m_cooldownAfterHighHS;
    j["anti_flag"]["burst_fire"] = m_burstFireEnabled;
    
    j["esp"]["enabled"] = m_espEnabled;
    j["esp"]["boxes"] = m_espBoxes;
    j["esp"]["lines"] = m_espLines;
    j["esp"]["health"] = m_espHealth;
    j["esp"]["names"] = m_espNames;
    j["esp"]["distance"] = m_espDistance;
    j["esp"]["head_dot"] = m_espHeadDot;
    j["esp"]["max_distance"] = m_maxESPDistance;
    
    j["radar"]["enabled"] = m_radarEnabled;
    j["radar"]["size"] = m_radarSize;
    j["radar"]["range"] = m_radarRange;
    j["radar"]["position"] = m_radarPosition;
    
    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

void ImGuiMenu::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    
    nlohmann::json j;
    file >> j;
    
    if (j.contains("aimbot")) {
        m_aimbotEnabled = j["aimbot"].value("enabled", m_aimbotEnabled);
        m_headshotProb = j["aimbot"].value("headshot_prob", m_headshotProb);
        m_smoothSpeed = j["aimbot"].value("smooth_speed", m_smoothSpeed);
        m_targetSelect = j["aimbot"].value("target_select", m_targetSelect);
        m_aimKey = j["aimbot"].value("aim_key", m_aimKey);
        m_aimAtHead = j["aimbot"].value("aim_at_head", m_aimAtHead);
        m_headshotOnAcquire = j["aimbot"].value("headshot_on_acquire", m_headshotOnAcquire);
        m_fovRadius = j["aimbot"].value("fov_radius", m_fovRadius);
    }
    
    if (j.contains("anti_flag")) {
        m_adaptiveHSEnabled = j["anti_flag"].value("adaptive_hs", m_adaptiveHSEnabled);
        m_maxConsecutiveHS = j["anti_flag"].value("max_consecutive_hs", m_maxConsecutiveHS);
        m_minTimeBetweenHS = j["anti_flag"].value("min_time_between_hs", m_minTimeBetweenHS);
        m_missChance = j["anti_flag"].value("miss_chance", m_missChance);
        m_reactionTimeMin = j["anti_flag"].value("reaction_time_min", m_reactionTimeMin);
        m_reactionTimeMax = j["anti_flag"].value("reaction_time_max", m_reactionTimeMax);
        m_flickEnabled = j["anti_flag"].value("flick_enabled", m_flickEnabled);
        m_flickAmount = j["anti_flag"].value("flick_amount", m_flickAmount);
        m_maxHSRatePerMinute = j["anti_flag"].value("max_hs_per_minute", m_maxHSRatePerMinute);
        m_cooldownAfterHighHS = j["anti_flag"].value("cooldown_after_high_hs", m_cooldownAfterHighHS);
        m_burstFireEnabled = j["anti_flag"].value("burst_fire", m_burstFireEnabled);
    }
    
    if (j.contains("esp")) {
        m_espEnabled = j["esp"].value("enabled", m_espEnabled);
        m_espBoxes = j["esp"].value("boxes", m_espBoxes);
        m_espLines = j["esp"].value("lines", m_espLines);
        m_espHealth = j["esp"].value("health", m_espHealth);
        m_espNames = j["esp"].value("names", m_espNames);
        m_espDistance = j["esp"].value("distance", m_espDistance);
        m_espHeadDot = j["esp"].value("head_dot", m_espHeadDot);
        m_maxESPDistance = j["esp"].value("max_distance", m_maxESPDistance);
    }
    
    if (j.contains("radar")) {
        m_radarEnabled = j["radar"].value("enabled", m_radarEnabled);
        m_radarSize = j["radar"].value("size", m_radarSize);
        m_radarRange = j["radar"].value("range", m_radarRange);
        m_radarPosition = j["radar"].value("position", m_radarPosition);
    }
}