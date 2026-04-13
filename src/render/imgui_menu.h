#pragma once
#include <imgui.h>
#include <string>
#include <functional>

class ImGuiMenu {
private:
    bool m_showMenu;
    int m_currentTab;
    
    // Aimbot settings
    bool m_aimbotEnabled;
    float m_headshotProb;
    float m_smoothSpeed;
    int m_targetSelect;
    int m_aimKey;
    bool m_aimAtHead;
    bool m_headshotOnAcquire;
    float m_fovRadius;
    
    // Anti-Flag settings
    bool m_adaptiveHSEnabled;
    int m_maxConsecutiveHS;
    float m_minTimeBetweenHS;
    float m_missChance;
    float m_reactionTimeMin;
    float m_reactionTimeMax;
    bool m_flickEnabled;
    float m_flickAmount;
    int m_maxHSRatePerMinute;
    float m_cooldownAfterHighHS;
    bool m_burstFireEnabled;
    
    // ESP settings
    bool m_espEnabled;
    bool m_espBoxes;
    bool m_espLines;
    bool m_espHealth;
    bool m_espNames;
    bool m_espDistance;
    bool m_espHeadDot;
    float m_enemyColor[4];
    float m_teammateColor[4];
    float m_boxColor[4];
    float m_maxESPDistance;
    
    // Radar settings
    bool m_radarEnabled;
    float m_radarSize;
    float m_radarRange;
    int m_radarPosition;
    
    void DrawAimbotTab();
    void DrawAntiFlagTab();
    void DrawESPTab();
    void DrawRadarTab();
    void DrawStatusBar();
    
public:
    ImGuiMenu();
    ~ImGuiMenu();
    
    void Render();
    bool IsMenuVisible() const { return m_showMenu; }
    void ToggleMenu() { m_showMenu = !m_showMenu; }
    
    // Aimbot getters
    bool IsAimbotEnabled() const { return m_aimbotEnabled; }
    float GetHeadshotProb() const { return m_headshotProb; }
    float GetSmoothSpeed() const { return m_smoothSpeed; }
    int GetTargetSelect() const { return m_targetSelect; }
    int GetAimKey() const { return m_aimKey; }
    bool GetAimAtHead() const { return m_aimAtHead; }
    bool GetHeadshotOnAcquire() const { return m_headshotOnAcquire; }
    float GetFOVRadius() const { return m_fovRadius; }
    
    // Anti-Flag getters
    bool IsAdaptiveHSEnabled() const { return m_adaptiveHSEnabled; }
    int GetMaxConsecutiveHS() const { return m_maxConsecutiveHS; }
    float GetMinTimeBetweenHS() const { return m_minTimeBetweenHS; }
    float GetMissChance() const { return m_missChance; }
    float GetReactionTimeMin() const { return m_reactionTimeMin; }
    float GetReactionTimeMax() const { return m_reactionTimeMax; }
    bool IsFlickEnabled() const { return m_flickEnabled; }
    float GetFlickAmount() const { return m_flickAmount; }
    int GetMaxHSRatePerMinute() const { return m_maxHSRatePerMinute; }
    float GetCooldownAfterHighHS() const { return m_cooldownAfterHighHS; }
    bool IsBurstFireEnabled() const { return m_burstFireEnabled; }
    
    // ESP getters
    bool IsESPEnabled() const { return m_espEnabled; }
    bool GetESPBones() const { return m_espBoxes; }
    bool GetESPLines() const { return m_espLines; }
    bool GetESPHealth() const { return m_espHealth; }
    bool GetESPNames() const { return m_espNames; }
    bool GetESPDistance() const { return m_espDistance; }
    bool GetESPHeadDot() const { return m_espHeadDot; }
    const float* GetEnemyColor() const { return m_enemyColor; }
    const float* GetTeammateColor() const { return m_teammateColor; }
    float GetMaxESPDistance() const { return m_maxESPDistance; }
    
    // Radar getters
    bool IsRadarEnabled() const { return m_radarEnabled; }
    float GetRadarSize() const { return m_radarSize; }
    float GetRadarRange() const { return m_radarRange; }
    int GetRadarPosition() const { return m_radarPosition; }
    
    void LoadFromFile(const std::string& path);
    void SaveToFile(const std::string& path);
};