#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../math/geometry.h"

struct ESPConfig {
    bool enabled = true;
    bool boxes = true;
    bool lines = true;
    bool healthBars = true;
    bool names = true;
    bool distance = true;
    bool skeleton = false;
    bool headDot = true;
    bool showHealthNumbers = true;
    bool showTeamColor = true;
    
    float enemyColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float teammateColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float boxColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float healthColorGood[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float healthColorMedium[4] = {1.0f, 1.0f, 0.0f, 1.0f};
    float healthColorBad[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    
    // Max render distance
    float maxDistance = 5000.0f;
    bool distanceFade = true;
};

class ESP {
private:
    Memory& m_memory;
    OffsetManager& m_offsets;
    ESPConfig m_config;
    float m_viewMatrix[4][4];
    int m_screenWidth;
    int m_screenHeight;
    
    struct Entity {
        Vector3 position;
        Vector3 headPos;
        int health;
        int team;
        std::string name;
        bool isAlive;
        bool isLocal;
        float distance;
    };
    
    std::vector<Entity> m_entities;
    
    void UpdateViewMatrix();
    void UpdateEntities();
    void UpdateEntity(int index, int localTeam);
    void DrawBox(const Vector2& pos, float height, float width, const float* color, float distance);
    void DrawHealthBar(const Vector2& pos, float height, float width, int health);
    void DrawText(const Vector2& pos, const std::string& text, const float* color);
    void DrawLine(const Vector2& from, const Vector2& to, const float* color);
    void DrawHeadDot(const Vector2& pos, const float* color);
    Vector3 GetBonePosition(uintptr_t boneArray, int boneIndex);
    float GetAlphaByDistance(float distance);
    
public:
    ESP(Memory& memory, OffsetManager& offsets);
    ~ESP();
    
    void SetConfig(const ESPConfig& config);
    void SetScreenSize(int width, int height);
    void Render();
};