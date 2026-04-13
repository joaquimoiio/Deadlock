#include "radar.h"
#include <imgui.h>
#include <cmath>

Radar::Radar(Memory& memory, OffsetManager& offsets)
    : m_memory(memory), m_offsets(offsets) {}

Radar::~Radar() {}

void Radar::SetConfig(const RadarConfig& config) {
    m_config = config;
}

Vector3 Radar::GetCameraPosition() {
    uintptr_t camera = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.CameraManager() + m_offsets.CameraPtrOffset()
    );
    
    return Vector3(
        m_memory.Read<float>(camera + m_offsets.CameraPosX()),
        m_memory.Read<float>(camera + m_offsets.CameraPosY()),
        m_memory.Read<float>(camera + m_offsets.CameraPosZ())
    );
}

float Radar::GetLocalYaw() {
    uintptr_t camera = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.CameraManager() + m_offsets.CameraPtrOffset()
    );
    return m_memory.Read<float>(camera + m_offsets.CameraYaw());
}

Vector2 Radar::WorldToRadar(const Vector3& worldPos, const Vector3& localPos, float localYaw) {
    float dx = worldPos.x - localPos.x;
    float dz = worldPos.z - localPos.z;
    
    float radYaw = localYaw * 3.14159f / 180.0f;
    float rotatedX = dx * cos(radYaw) - dz * sin(radYaw);
    float rotatedZ = dx * sin(radYaw) + dz * cos(radYaw);
    
    float scale = (m_config.size / 2) / m_config.range;
    float radarX = rotatedX * scale;
    float radarZ = rotatedZ * scale;
    
    float maxPos = m_config.size / 2 - 5;
    radarX = std::clamp(radarX, -maxPos, maxPos);
    radarZ = std::clamp(radarZ, -maxPos, maxPos);
    
    return Vector2(radarX, radarZ);
}

Vector2 Radar::GetRadarPosition() {
    float padding = 10.0f;
    
    switch (m_config.position) {
        case 0: return Vector2(m_config.size + padding, padding);
        case 1: return Vector2(padding, padding);
        case 2: return Vector2(m_config.size + padding, m_config.size + padding);
        case 3: return Vector2(padding, m_config.size + padding);
        default: return Vector2(padding, padding);
    }
}

void Radar::UpdateEntities() {
    m_entities.clear();
    
    Vector3 localPos = GetCameraPosition();
    float localYaw = GetLocalYaw();
    
    uintptr_t entityList = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.EntityList()
    );
    
    uintptr_t localController = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.LocalPlayerController()
    );
    int localTeam = m_memory.Read<int>(localController + m_offsets.TeamOffset());
    
    for (int i = 1; i <= 64; i++) {
        uintptr_t listEntry = m_memory.Read<uintptr_t>(
            entityList + 0x8 * ((i & 0x7FFF) >> 9) + 0x10
        );
        
        if (!listEntry) continue;
        
        uintptr_t controller = m_memory.Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
        if (!controller) continue;
        
        int team = m_memory.Read<int>(controller + m_offsets.TeamOffset());
        if (team == localTeam) continue;
        
        uintptr_t pawnHandle = m_memory.Read<uintptr_t>(controller + 0x874);
        uintptr_t pawnListEntry = m_memory.Read<uintptr_t>(
            entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10
        );
        uintptr_t pawn = m_memory.Read<uintptr_t>(pawnListEntry + 0x78 * (pawnHandle & 0x1FF));
        
        if (!pawn) continue;
        
        uintptr_t gameSceneNode = m_memory.Read<uintptr_t>(pawn + m_offsets.GameSceneNode());
        uintptr_t posAddr = gameSceneNode + m_offsets.NodePosition();
        
        Vector3 entPos(
            m_memory.Read<float>(posAddr),
            m_memory.Read<float>(posAddr + 4),
            m_memory.Read<float>(posAddr + 8)
        );
        
        Vector2 radarPos = WorldToRadar(entPos, localPos, localYaw);
        
        RadarEntity entity;
        entity.radarPos = radarPos;
        entity.team = team;
        entity.isEnemy = team != localTeam;
        entity.yaw = 0;
        
        m_entities.push_back(entity);
    }
}

void Radar::Render() {
    if (!m_config.enabled) return;
    
    UpdateEntities();
    
    Vector2 radarScreenPos = GetRadarPosition();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Background
    drawList->AddRectFilled(
        ImVec2(radarScreenPos.x - m_config.size, radarScreenPos.y - m_config.size),
        ImVec2(radarScreenPos.x, radarScreenPos.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(m_config.backgroundColor[0], m_config.backgroundColor[1],
                                               m_config.backgroundColor[2], m_config.backgroundColor[3]))
    );
    
    // Border
    drawList->AddRect(
        ImVec2(radarScreenPos.x - m_config.size, radarScreenPos.y - m_config.size),
        ImVec2(radarScreenPos.x, radarScreenPos.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(m_config.borderColor[0], m_config.borderColor[1],
                                               m_config.borderColor[2], m_config.borderColor[3])),
        0.0f, 0, 1.5f
    );
    
    // Crosshair lines
    drawList->AddLine(
        ImVec2(radarScreenPos.x - m_config.size / 2, radarScreenPos.y - m_config.size),
        ImVec2(radarScreenPos.x - m_config.size / 2, radarScreenPos.y),
        IM_COL32(255, 255, 255, 100)
    );
    drawList->AddLine(
        ImVec2(radarScreenPos.x - m_config.size, radarScreenPos.y - m_config.size / 2),
        ImVec2(radarScreenPos.x, radarScreenPos.y - m_config.size / 2),
        IM_COL32(255, 255, 255, 100)
    );
    
    // Range circles
    drawList->AddCircle(
        ImVec2(radarScreenPos.x - m_config.size / 2, radarScreenPos.y - m_config.size / 2),
        m_config.size / 2,
        IM_COL32(100, 100, 100, 100)
    );
    drawList->AddCircle(
        ImVec2(radarScreenPos.x - m_config.size / 2, radarScreenPos.y - m_config.size / 2),
        m_config.size / 4,
        IM_COL32(100, 100, 100, 100)
    );
    
    // Center (player)
    drawList->AddCircleFilled(
        ImVec2(radarScreenPos.x - m_config.size / 2, radarScreenPos.y - m_config.size / 2),
        5.0f,
        ImGui::ColorConvertFloat4ToU32(ImVec4(m_config.playerColor[0], m_config.playerColor[1],
                                               m_config.playerColor[2], m_config.playerColor[3]))
    );
    
    // Player direction line
    if (m_config.showPlayerDirection) {
        float dirX = cos(GetLocalYaw() * 3.14159f / 180.0f) * 15.0f;
        float dirY = sin(GetLocalYaw() * 3.14159f / 180.0f) * 15.0f;
        drawList->AddLine(
            ImVec2(radarScreenPos.x - m_config.size / 2, radarScreenPos.y - m_config.size / 2),
            ImVec2(radarScreenPos.x - m_config.size / 2 + dirX, radarScreenPos.y - m_config.size / 2 + dirY),
            IM_COL32(255, 255, 255, 200), 2.0f
        );
    }
    
    // Entities
    for (const auto& entity : m_entities) {
        float screenX = radarScreenPos.x - m_config.size / 2 + entity.radarPos.x;
        float screenY = radarScreenPos.y - m_config.size / 2 + entity.radarPos.y;
        
        const float* color = entity.isEnemy ? m_config.enemyColor : m_config.playerColor;
        
        drawList->AddCircleFilled(
            ImVec2(screenX, screenY),
            4.0f,
            ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
        );
    }
}