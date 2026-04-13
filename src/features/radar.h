#pragma once
#include <vector>
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../math/geometry.h"

struct RadarConfig {
    bool enabled = true;
    float size = 200.0f;
    float range = 3000.0f;
    int position = 0;  // 0=TR, 1=TL, 2=BR, 3=BL
    float backgroundColor[4] = {0.0f, 0.0f, 0.0f, 0.7f};
    float enemyColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float playerColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    bool showPlayerDirection = true;
    bool showEnemyNames = false;
};

class Radar {
private:
    Memory& m_memory;
    OffsetManager& m_offsets;
    RadarConfig m_config;
    
    struct RadarEntity {
        Vector2 radarPos;
        int team;
        bool isEnemy;
        float yaw;
    };
    
    std::vector<RadarEntity> m_entities;
    
    Vector2 WorldToRadar(const Vector3& worldPos, const Vector3& localPos, float localYaw);
    void UpdateEntities();
    Vector2 GetRadarPosition();
    Vector3 GetCameraPosition();
    float GetLocalYaw();
    
public:
    Radar(Memory& memory, OffsetManager& offsets);
    ~Radar();
    
    void SetConfig(const RadarConfig& config);
    void Render();
};