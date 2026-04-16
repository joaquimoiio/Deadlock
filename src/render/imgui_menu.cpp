#include "imgui_menu.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

// ─────────────────────── file-local helpers ───────────────────────────────────

static void SectionHeader(const char* label)
{
    ImVec2 p  = ImGui::GetCursorScreenPos();
    float  th = ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddRectFilled(
        p, ImVec2(p.x + 3.f, p.y + th),
        IM_COL32(138, 79, 255, 255));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9.f);
    ImGui::TextColored(ImVec4(0.66f, 0.50f, 0.96f, 1.f), "%s", label);
    ImGui::Spacing();
}

static void GreenCheckbox(const char* label, bool* v)
{
    if (*v) ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.27f, 0.88f, 0.46f, 1.f));
    ImGui::Checkbox(label, v);
    if (*v) ImGui::PopStyleColor();
}

static void StatusPill(const char* label, bool active)
{
    ImGui::PushStyleColor(ImGuiCol_Button,
        active ? ImVec4(0.06f, 0.24f, 0.12f, 1.f) : ImVec4(0.12f, 0.12f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        active ? ImVec4(0.06f, 0.24f, 0.12f, 1.f) : ImVec4(0.12f, 0.12f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        active ? ImVec4(0.06f, 0.24f, 0.12f, 1.f) : ImVec4(0.12f, 0.12f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,
        active ? ImVec4(0.25f, 0.88f, 0.45f, 1.f) : ImVec4(0.42f, 0.42f, 0.52f, 1.f));
    ImGui::SmallButton(label);
    ImGui::PopStyleColor(4);
}

// ─────────────────────── constructor / destructor ─────────────────────────────

ImGuiMenu::ImGuiMenu()
    : m_showMenu(true), m_currentTab(0),
      m_aimbotEnabled(true), m_headshotProb(0.25f), m_smoothSpeed(5.0f),
      m_targetSelect(0), m_aimKey(VK_LBUTTON), m_aimAtHead(true),
      m_headshotOnAcquire(true), m_fovRadius(100.0f),
      m_adaptiveHSEnabled(true), m_maxConsecutiveHS(3), m_minTimeBetweenHS(0.5f),
      m_missChance(0.05f), m_reactionTimeMin(0.08f), m_reactionTimeMax(0.25f),
      m_flickEnabled(true), m_flickAmount(5.0f), m_maxHSRatePerMinute(4),
      m_cooldownAfterHighHS(5.0f), m_burstFireEnabled(true),
      m_espEnabled(true), m_espBoxes(true), m_espLines(true),
      m_espHealth(true), m_espNames(true), m_espDistance(true),
      m_espHeadDot(true), m_maxESPDistance(5000.0f),
      m_radarEnabled(true), m_radarSize(200.0f), m_radarRange(3000.0f), m_radarPosition(0)
{
    m_enemyColor[0]    = 1.00f; m_enemyColor[1]    = 0.18f; m_enemyColor[2]    = 0.18f; m_enemyColor[3]    = 1.f;
    m_teammateColor[0] = 0.18f; m_teammateColor[1] = 0.72f; m_teammateColor[2] = 1.00f; m_teammateColor[3] = 1.f;
    m_boxColor[0]      = 0.78f; m_boxColor[1]      = 0.78f; m_boxColor[2]      = 0.78f; m_boxColor[3]      = 1.f;
    ApplyStyle();
}

ImGuiMenu::~ImGuiMenu() {}

// ─────────────────────── style ────────────────────────────────────────────────

void ImGuiMenu::ApplyStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowPadding     = ImVec2(14, 14);
    s.FramePadding      = ImVec2(8,  4);
    s.CellPadding       = ImVec2(6,  4);
    s.ItemSpacing       = ImVec2(8,  7);
    s.ItemInnerSpacing  = ImVec2(6,  4);
    s.ScrollbarSize     = 10;
    s.GrabMinSize       = 10;
    s.WindowBorderSize  = 1;
    s.ChildBorderSize   = 1;
    s.FrameBorderSize   = 0;
    s.WindowRounding    = 8;
    s.ChildRounding     = 6;
    s.FrameRounding     = 5;
    s.PopupRounding     = 6;
    s.ScrollbarRounding = 4;
    s.GrabRounding      = 4;
    s.TabRounding       = 5;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                  = ImVec4(0.88f, 0.87f, 0.93f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.38f, 0.37f, 0.48f, 1.00f);
    c[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.07f, 0.13f, 0.97f);
    c[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.09f, 0.16f, 1.00f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.09f, 0.15f, 0.97f);
    c[ImGuiCol_Border]                = ImVec4(0.22f, 0.20f, 0.33f, 1.00f);
    c[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.13f, 0.12f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.18f, 0.17f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.22f, 0.37f, 1.00f);
    c[ImGuiCol_TitleBg]               = ImVec4(0.07f, 0.06f, 0.10f, 1.00f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.11f, 0.30f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.07f, 0.06f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0.07f, 0.06f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.34f, 0.20f, 0.62f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.44f, 0.28f, 0.76f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.55f, 0.36f, 0.92f, 1.00f);
    c[ImGuiCol_CheckMark]             = ImVec4(0.55f, 0.36f, 0.96f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.50f, 0.28f, 0.90f, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.65f, 0.42f, 1.00f, 1.00f);
    c[ImGuiCol_Button]                = ImVec4(0.17f, 0.16f, 0.27f, 1.00f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.36f, 0.22f, 0.72f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.48f, 0.30f, 0.90f, 1.00f);
    c[ImGuiCol_Header]                = ImVec4(0.36f, 0.20f, 0.68f, 0.40f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.44f, 0.27f, 0.80f, 0.65f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.54f, 0.35f, 0.94f, 1.00f);
    c[ImGuiCol_Separator]             = ImVec4(0.20f, 0.18f, 0.30f, 1.00f);
    c[ImGuiCol_SeparatorHovered]      = ImVec4(0.50f, 0.30f, 0.90f, 0.75f);
    c[ImGuiCol_SeparatorActive]       = ImVec4(0.55f, 0.36f, 0.96f, 1.00f);
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.50f, 0.30f, 0.90f, 0.20f);
    c[ImGuiCol_ResizeGripHovered]     = ImVec4(0.50f, 0.30f, 0.90f, 0.60f);
    c[ImGuiCol_ResizeGripActive]      = ImVec4(0.55f, 0.36f, 0.96f, 1.00f);
    c[ImGuiCol_Tab]                   = ImVec4(0.10f, 0.09f, 0.16f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.36f, 0.22f, 0.70f, 0.80f);
    c[ImGuiCol_TabActive]             = ImVec4(0.28f, 0.16f, 0.58f, 1.00f);
    c[ImGuiCol_TabUnfocused]          = ImVec4(0.08f, 0.07f, 0.13f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.14f, 0.32f, 1.00f);
    c[ImGuiCol_TextSelectedBg]        = ImVec4(0.50f, 0.30f, 0.90f, 0.35f);
    c[ImGuiCol_NavHighlight]          = ImVec4(0.50f, 0.30f, 0.90f, 1.00f);
    c[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    c[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    c[ImGuiCol_ModalDimBg]            = ImVec4(0.08f, 0.07f, 0.13f, 0.89f);
}

// ─────────────────────── render ───────────────────────────────────────────────

void ImGuiMenu::Render()
{
    if (!m_showMenu) return;

    ImGui::SetNextWindowSize(ImVec2(560, 580), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(80, 80),    ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(460, 380), ImVec2(720, 820));

    if (ImGui::Begin("DeadUnlock  v1.0", &m_showMenu,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
    {
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
        {
            if (ImGui::BeginTabItem("  Aimbot  "))    { DrawAimbotTab();   ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("  Anti-Flag  ")) { DrawAntiFlagTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("  ESP  "))        { DrawESPTab();      ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("  Radar  "))      { DrawRadarTab();    ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }

        DrawStatusBar();

        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        if (ImGui::Button("Save Config", ImVec2(110, 26)))
            SaveToFile("config/settings.json");
        ImGui::SameLine();
        if (ImGui::Button("Load Config", ImVec2(110, 26)))
            LoadFromFile("config/settings.json");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.24f, 0.10f, 0.10f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.14f, 0.14f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.18f, 0.18f, 1.f));
        if (ImGui::Button("Reset Anti-Flag", ImVec2(130, 26))) { /* reset */ }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }
    ImGui::End();
}

// ─────────────────────── aimbot tab ───────────────────────────────────────────

void ImGuiMenu::DrawAimbotTab()
{
    ImGui::Spacing();

    ImVec4 stateCol = m_aimbotEnabled
        ? ImVec4(0.27f, 0.88f, 0.46f, 1.f)
        : ImVec4(0.70f, 0.26f, 0.26f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.27f, 0.88f, 0.46f, 1.f));
    ImGui::Checkbox("##ab_en", &m_aimbotEnabled);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(stateCol,
        m_aimbotEnabled ? "Aimbot  —  ACTIVE" : "Aimbot  —  INACTIVE");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    SectionHeader("TARGET");
    ImGui::RadioButton("FOV Circle",    &m_targetSelect, 0);
    ImGui::SameLine(170);
    ImGui::RadioButton("Nearest Enemy", &m_targetSelect, 1);
    if (m_targetSelect == 0) {
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##fov", &m_fovRadius, 50.f, 300.f, "FOV Radius  %.0f px");
        ImGui::PopItemWidth();
    }

    ImGui::Spacing();
    SectionHeader("SMOOTHING");
    ImGui::PushItemWidth(-1);
    ImGui::SliderFloat("##smooth", &m_smoothSpeed, 1.f, 15.f, "Speed  %.1f deg/frame");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    SectionHeader("HEADSHOT");
    ImGui::PushItemWidth(-1);
    ImGui::SliderFloat("##hsprob", &m_headshotProb, 0.f, 1.f, "Probability  %.0f%%");
    ImGui::PopItemWidth();
    ImGui::Spacing();
    GreenCheckbox("Force headshot on new target", &m_headshotOnAcquire);
    GreenCheckbox("Prefer headshot aiming",       &m_aimAtHead);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.64f, 0.22f, 1.f));
    ImGui::TextWrapped("Anti-Flag adjusts HS%% in real-time.");
    ImGui::PopStyleColor();
}

// ─────────────────────── anti-flag tab ────────────────────────────────────────

void ImGuiMenu::DrawAntiFlagTab()
{
    ImGui::Spacing();

    ImVec4 adCol = m_adaptiveHSEnabled
        ? ImVec4(0.27f, 0.88f, 0.46f, 1.f)
        : ImVec4(0.70f, 0.26f, 0.26f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, adCol);
    ImGui::Checkbox("##af_en", &m_adaptiveHSEnabled);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(adCol,
        m_adaptiveHSEnabled ? "Adaptive Headshot  —  ON" : "Adaptive Headshot  —  OFF");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reduces HS%% automatically when suspicious patterns are detected");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    SectionHeader("THRESHOLDS");
    ImGui::PushItemWidth(-1);
    ImGui::SliderInt  ("##maxchs", &m_maxConsecutiveHS,  1,    10,    "Max Consecutive  %d hits");
    ImGui::SliderFloat("##minths", &m_minTimeBetweenHS,  0.1f,  2.f,  "Min Interval  %.2f s");
    ImGui::SliderFloat("##miss",   &m_missChance,        0.f,   0.25f, "Miss Chance  %.0f%%");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    SectionHeader("REACTION TIME");
    float half = (ImGui::GetContentRegionAvail().x - 8) * 0.5f;
    ImGui::PushItemWidth(half);
    ImGui::SliderFloat("##rtmin", &m_reactionTimeMin, 0.05f, 0.3f, "Min  %.0f ms");
    ImGui::SameLine(0, 8);
    ImGui::SetNextItemWidth(half);
    ImGui::SliderFloat("##rtmax", &m_reactionTimeMax, 0.1f,  0.5f, "Max  %.0f ms");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    SectionHeader("HUMANIZATION");
    GreenCheckbox("Flick Simulation", &m_flickEnabled);
    if (m_flickEnabled) {
        ImGui::SameLine(190);
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##flick", &m_flickAmount, 1.f, 20.f, "%.1f px");
    }
    GreenCheckbox("Burst Fire Simulation", &m_burstFireEnabled);

    ImGui::Spacing();
    SectionHeader("RATE LIMITING");
    ImGui::PushItemWidth(half);
    ImGui::SliderInt  ("##maxhsm", &m_maxHSRatePerMinute,  1,    15,   "Max %d HS/min");
    ImGui::SameLine(0, 8);
    ImGui::SetNextItemWidth(half);
    ImGui::SliderFloat("##cool",   &m_cooldownAfterHighHS, 2.f,  10.f, "CD %.1f s");
    ImGui::PopItemWidth();
}

// ─────────────────────── esp tab ──────────────────────────────────────────────

void ImGuiMenu::DrawESPTab()
{
    ImGui::Spacing();

    ImVec4 espCol = m_espEnabled
        ? ImVec4(0.27f, 0.88f, 0.46f, 1.f)
        : ImVec4(0.70f, 0.26f, 0.26f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, espCol);
    ImGui::Checkbox("##esp_en", &m_espEnabled);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(espCol,
        m_espEnabled ? "ESP  —  ACTIVE" : "ESP  —  INACTIVE");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginDisabled(!m_espEnabled);

    SectionHeader("FEATURES");
    float col2x = ImGui::GetContentRegionAvail().x * 0.5f;
    GreenCheckbox("Boxes (2D)",  &m_espBoxes);    ImGui::SameLine(col2x); GreenCheckbox("Health Bars", &m_espHealth);
    GreenCheckbox("Names",       &m_espNames);    ImGui::SameLine(col2x); GreenCheckbox("Distance",    &m_espDistance);
    GreenCheckbox("Lines",       &m_espLines);    ImGui::SameLine(col2x); GreenCheckbox("Head Dot",    &m_espHeadDot);

    ImGui::Spacing();
    SectionHeader("COLORS");
    ImGui::ColorEdit4("Enemy##ec",    m_enemyColor,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine();
    ImGui::ColorEdit4("Teammate##tc", m_teammateColor,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine();
    ImGui::ColorEdit4("Box##bc",      m_boxColor,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

    ImGui::Spacing();
    SectionHeader("RENDER DISTANCE");
    ImGui::PushItemWidth(-1);
    ImGui::SliderFloat("##espdist", &m_maxESPDistance, 500.f, 10000.f, "%.0f units");
    ImGui::PopItemWidth();

    ImGui::EndDisabled();
}

// ─────────────────────── radar tab ────────────────────────────────────────────

void ImGuiMenu::DrawRadarTab()
{
    ImGui::Spacing();

    ImVec4 radCol = m_radarEnabled
        ? ImVec4(0.27f, 0.88f, 0.46f, 1.f)
        : ImVec4(0.70f, 0.26f, 0.26f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, radCol);
    ImGui::Checkbox("##rad_en", &m_radarEnabled);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(radCol,
        m_radarEnabled ? "Radar  —  ACTIVE" : "Radar  —  INACTIVE");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginDisabled(!m_radarEnabled);

    SectionHeader("DISPLAY");
    float half = (ImGui::GetContentRegionAvail().x - 8) * 0.5f;
    ImGui::PushItemWidth(half);
    ImGui::SliderFloat("##radsize",  &m_radarSize,  100.f, 400.f,  "Size  %.0f px");
    ImGui::SameLine(0, 8);
    ImGui::SetNextItemWidth(half);
    ImGui::SliderFloat("##radrange", &m_radarRange, 1000.f, 5000.f, "Range  %.0f u");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    SectionHeader("POSITION");
    const char* positions[] = { "Top-Right", "Top-Left", "Bottom-Right", "Bottom-Left" };
    ImGui::PushItemWidth(-1);
    ImGui::Combo("##radpos", &m_radarPosition, positions, 4);
    ImGui::PopItemWidth();

    ImGui::EndDisabled();
}

// ─────────────────────── status bar ───────────────────────────────────────────

void ImGuiMenu::DrawStatusBar()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    StatusPill(" Aimbot ", m_aimbotEnabled);
    ImGui::SameLine(0, 5);
    StatusPill(" ESP ",    m_espEnabled);
    ImGui::SameLine(0, 5);
    StatusPill(" Radar ",  m_radarEnabled);

    ImGui::SameLine(ImGui::GetWindowWidth() - 88);
    ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.54f, 1.f),
        "%.0f FPS", ImGui::GetIO().Framerate);
    ImGui::Spacing();
}

// ─────────────────────── save / load ──────────────────────────────────────────

void ImGuiMenu::SaveToFile(const std::string& path) {
    nlohmann::json j;

    j["aimbot"]["enabled"]            = m_aimbotEnabled;
    j["aimbot"]["headshot_prob"]      = m_headshotProb;
    j["aimbot"]["smooth_speed"]       = m_smoothSpeed;
    j["aimbot"]["target_select"]      = m_targetSelect;
    j["aimbot"]["aim_key"]            = m_aimKey;
    j["aimbot"]["aim_at_head"]        = m_aimAtHead;
    j["aimbot"]["headshot_on_acquire"]= m_headshotOnAcquire;
    j["aimbot"]["fov_radius"]         = m_fovRadius;

    j["anti_flag"]["adaptive_hs"]           = m_adaptiveHSEnabled;
    j["anti_flag"]["max_consecutive_hs"]    = m_maxConsecutiveHS;
    j["anti_flag"]["min_time_between_hs"]   = m_minTimeBetweenHS;
    j["anti_flag"]["miss_chance"]           = m_missChance;
    j["anti_flag"]["reaction_time_min"]     = m_reactionTimeMin;
    j["anti_flag"]["reaction_time_max"]     = m_reactionTimeMax;
    j["anti_flag"]["flick_enabled"]         = m_flickEnabled;
    j["anti_flag"]["flick_amount"]          = m_flickAmount;
    j["anti_flag"]["max_hs_per_minute"]     = m_maxHSRatePerMinute;
    j["anti_flag"]["cooldown_after_high_hs"]= m_cooldownAfterHighHS;
    j["anti_flag"]["burst_fire"]            = m_burstFireEnabled;

    j["esp"]["enabled"]      = m_espEnabled;
    j["esp"]["boxes"]        = m_espBoxes;
    j["esp"]["lines"]        = m_espLines;
    j["esp"]["health"]       = m_espHealth;
    j["esp"]["names"]        = m_espNames;
    j["esp"]["distance"]     = m_espDistance;
    j["esp"]["head_dot"]     = m_espHeadDot;
    j["esp"]["max_distance"] = m_maxESPDistance;

    j["radar"]["enabled"]  = m_radarEnabled;
    j["radar"]["size"]     = m_radarSize;
    j["radar"]["range"]    = m_radarRange;
    j["radar"]["position"] = m_radarPosition;

    std::ofstream file(path);
    if (file.is_open())
        file << j.dump(4);
}

void ImGuiMenu::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    nlohmann::json j;
    file >> j;

    if (j.contains("aimbot")) {
        m_aimbotEnabled     = j["aimbot"].value("enabled",             m_aimbotEnabled);
        m_headshotProb      = j["aimbot"].value("headshot_prob",       m_headshotProb);
        m_smoothSpeed       = j["aimbot"].value("smooth_speed",        m_smoothSpeed);
        m_targetSelect      = j["aimbot"].value("target_select",       m_targetSelect);
        m_aimKey            = j["aimbot"].value("aim_key",             m_aimKey);
        m_aimAtHead         = j["aimbot"].value("aim_at_head",         m_aimAtHead);
        m_headshotOnAcquire = j["aimbot"].value("headshot_on_acquire", m_headshotOnAcquire);
        m_fovRadius         = j["aimbot"].value("fov_radius",          m_fovRadius);
    }

    if (j.contains("anti_flag")) {
        m_adaptiveHSEnabled   = j["anti_flag"].value("adaptive_hs",            m_adaptiveHSEnabled);
        m_maxConsecutiveHS    = j["anti_flag"].value("max_consecutive_hs",     m_maxConsecutiveHS);
        m_minTimeBetweenHS    = j["anti_flag"].value("min_time_between_hs",    m_minTimeBetweenHS);
        m_missChance          = j["anti_flag"].value("miss_chance",            m_missChance);
        m_reactionTimeMin     = j["anti_flag"].value("reaction_time_min",      m_reactionTimeMin);
        m_reactionTimeMax     = j["anti_flag"].value("reaction_time_max",      m_reactionTimeMax);
        m_flickEnabled        = j["anti_flag"].value("flick_enabled",          m_flickEnabled);
        m_flickAmount         = j["anti_flag"].value("flick_amount",           m_flickAmount);
        m_maxHSRatePerMinute  = j["anti_flag"].value("max_hs_per_minute",      m_maxHSRatePerMinute);
        m_cooldownAfterHighHS = j["anti_flag"].value("cooldown_after_high_hs", m_cooldownAfterHighHS);
        m_burstFireEnabled    = j["anti_flag"].value("burst_fire",             m_burstFireEnabled);
    }

    if (j.contains("esp")) {
        m_espEnabled      = j["esp"].value("enabled",      m_espEnabled);
        m_espBoxes        = j["esp"].value("boxes",        m_espBoxes);
        m_espLines        = j["esp"].value("lines",        m_espLines);
        m_espHealth       = j["esp"].value("health",       m_espHealth);
        m_espNames        = j["esp"].value("names",        m_espNames);
        m_espDistance     = j["esp"].value("distance",     m_espDistance);
        m_espHeadDot      = j["esp"].value("head_dot",     m_espHeadDot);
        m_maxESPDistance  = j["esp"].value("max_distance", m_maxESPDistance);
    }

    if (j.contains("radar")) {
        m_radarEnabled  = j["radar"].value("enabled",  m_radarEnabled);
        m_radarSize     = j["radar"].value("size",     m_radarSize);
        m_radarRange    = j["radar"].value("range",    m_radarRange);
        m_radarPosition = j["radar"].value("position", m_radarPosition);
    }
}
