#include "esp.h"
#include <imgui.h>
#include <string>
#include <cmath>
#include <cstdio>

ESP::ESP(Memory& memory, OffsetManager& offsets)
    : m_memory(memory), m_offsets(offsets), m_screenWidth(1920), m_screenHeight(1080) {
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m_viewMatrix[i][j] = 0;
        }
    }
}

ESP::~ESP() {}

void ESP::SetConfig(const ESPConfig& config) {
    m_config = config;
}

void ESP::SetScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

float ESP::GetAlphaByDistance(float distance) {
    if (!m_config.distanceFade) return 1.0f;
    if (distance > m_config.maxDistance) return 0.0f;
    
    float alpha = 1.0f - (distance / m_config.maxDistance);
    return std::clamp(alpha, 0.3f, 1.0f);
}

void ESP::UpdateViewMatrix() {
    uintptr_t viewMatrixAddr = m_memory.GetClientBase() + m_offsets.ViewMatrix();
    auto raw = m_memory.ReadBytes(viewMatrixAddr, 16 * 4);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m_viewMatrix[i][j] = *reinterpret_cast<float*>(&raw[(i * 4 + j) * 4]);
        }
    }
}

Vector3 ESP::GetBonePosition(uintptr_t boneArray, int boneIndex) {
    uintptr_t boneAddress = boneArray + (boneIndex * m_offsets.BoneStep());
    return Vector3(
        m_memory.Read<float>(boneAddress),
        m_memory.Read<float>(boneAddress + 4),
        m_memory.Read<float>(boneAddress + 8)
    );
}

void ESP::UpdateEntity(int index, int localTeam) {
    uintptr_t entityList = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.EntityList()
    );
    
    uintptr_t listEntry = m_memory.Read<uintptr_t>(
        entityList + 0x8 * ((index & 0x7FFF) >> 9) + 0x10
    );
    
    if (!listEntry) return;
    
    uintptr_t controller = m_memory.Read<uintptr_t>(listEntry + 120 * (index & 0x1FF));
    if (!controller) return;
    
    uintptr_t pawnHandle = m_memory.Read<uintptr_t>(controller + 0x874);
    uintptr_t pawnListEntry = m_memory.Read<uintptr_t>(
        entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10
    );
    uintptr_t pawn = m_memory.Read<uintptr_t>(pawnListEntry + 0x78 * (pawnHandle & 0x1FF));
    
    if (!pawn) return;
    
    int team = m_memory.Read<int>(controller + m_offsets.TeamOffset());
    int health = m_memory.Read<int>(pawn + m_offsets.HealthOffset());
    
    if (health <= 0) return;
    
    uintptr_t gameSceneNode = m_memory.Read<uintptr_t>(pawn + m_offsets.GameSceneNode());
    uintptr_t posAddr = gameSceneNode + m_offsets.NodePosition();
    
    Vector3 position(
        m_memory.Read<float>(posAddr),
        m_memory.Read<float>(posAddr + 4),
        m_memory.Read<float>(posAddr + 8) + 70.0f
    );
    
    // Get head position
    uintptr_t boneArray = m_memory.Read<uintptr_t>(gameSceneNode + m_offsets.BoneArray());
    Vector3 headPos = GetBonePosition(boneArray, 7);
    
    // Get camera position for distance calculation
    uintptr_t camera = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.CameraManager() + m_offsets.CameraPtrOffset()
    );
    Vector3 cameraPos(
        m_memory.Read<float>(camera + m_offsets.CameraPosX()),
        m_memory.Read<float>(camera + m_offsets.CameraPosY()),
        m_memory.Read<float>(camera + m_offsets.CameraPosZ())
    );
    
    Entity e;
    e.position = position;
    e.headPos = headPos;
    e.health = health;
    e.team = team;
    e.isAlive = true;
    e.isLocal = false;
    e.distance = cameraPos.DistanceTo(position);
    e.name = "Player";
    
    m_entities.push_back(e);
}

void ESP::UpdateEntities() {
    m_entities.clear();
    
    uintptr_t localController = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.LocalPlayerController()
    );
    int localTeam = m_memory.Read<int>(localController + m_offsets.TeamOffset());
    
    for (int i = 1; i <= 64; i++) {
        UpdateEntity(i, localTeam);
    }
}

void ESP::DrawBox(const Vector2& pos, float height, float width, const float* color, float distance) {
    float alpha = GetAlphaByDistance(distance);
    ImU32 drawColor = IM_COL32(
        (int)(color[0] * 255),
        (int)(color[1] * 255),
        (int)(color[2] * 255),
        (int)(color[3] * 255 * alpha)
    );
    
    ImGui::GetBackgroundDrawList()->AddRect(
        ImVec2(pos.x - width / 2, pos.y - height),
        ImVec2(pos.x + width / 2, pos.y),
        drawColor,
        2.0f, 0, 1.5f
    );
}

void ESP::DrawHealthBar(const Vector2& pos, float height, float width, int health) {
    float healthPercent = health / 100.0f;
    float barHeight = height * healthPercent;
    float barWidth = 4.0f;
    
    ImU32 color;
    if (healthPercent > 0.6f) {
        color = IM_COL32(0, 255, 0, 255);
    } else if (healthPercent > 0.3f) {
        color = IM_COL32(255, 255, 0, 255);
    } else {
        color = IM_COL32(255, 0, 0, 255);
    }
    
    // Background
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(pos.x - width / 2 - 6, pos.y - height),
        ImVec2(pos.x - width / 2 - 2, pos.y),
        IM_COL32(40, 40, 40, 200)
    );
    
    // Health fill
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(pos.x - width / 2 - 5, pos.y - barHeight),
        ImVec2(pos.x - width / 2 - 3, pos.y),
        color
    );
    
    if (m_config.showHealthNumbers) {
        char healthText[16];
        sprintf_s(healthText, "%d", health);
        ImGui::GetBackgroundDrawList()->AddText(
            ImVec2(pos.x - width / 2 - 20, pos.y - 8),
            IM_COL32(255, 255, 255, 255),
            healthText
        );
    }
}

void ESP::DrawText(const Vector2& pos, const std::string& text, const float* color) {
    ImGui::GetBackgroundDrawList()->AddText(
        ImVec2(pos.x - 20, pos.y - 25),
        ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3])),
        text.c_str()
    );
}

void ESP::DrawLine(const Vector2& from, const Vector2& to, const float* color) {
    ImGui::GetBackgroundDrawList()->AddLine(
        ImVec2(from.x, from.y),
        ImVec2(to.x, to.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
    );
}

void ESP::DrawHeadDot(const Vector2& pos, const float* color) {
    ImGui::GetBackgroundDrawList()->AddCircleFilled(
        ImVec2(pos.x, pos.y),
        4.0f,
        ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
    );
}

void ESP::Render() {
    if (!m_config.enabled) return;
    
    UpdateViewMatrix();
    UpdateEntities();
    
    uintptr_t localController = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.LocalPlayerController()
    );
    int localTeam = m_memory.Read<int>(localController + m_offsets.TeamOffset());
    
    for (const auto& entity : m_entities) {
        if (entity.distance > m_config.maxDistance) continue;
        
        bool isEnemy = entity.team != localTeam;
        const float* color = isEnemy ? m_config.enemyColor : m_config.teammateColor;
        
        auto screenPos = Geometry::WorldToScreen(m_viewMatrix, entity.position, m_screenWidth, m_screenHeight);
        auto headScreen = Geometry::WorldToScreen(m_viewMatrix, entity.headPos, m_screenWidth, m_screenHeight);
        
        if (!screenPos.has_value() || !headScreen.has_value()) continue;
        
        float height = std::abs(screenPos->y - headScreen->y);
        float width = height * 0.5f;
        
        // Line to crosshair
        if (m_config.lines) {
            DrawLine(Vector2(m_screenWidth / 2, m_screenHeight / 2), screenPos.value(), color);
        }
        
        // Box
        if (m_config.boxes) {
            DrawBox(screenPos.value(), height, width, m_config.boxColor, entity.distance);
        }
        
        // Health bar
        if (m_config.healthBars) {
            DrawHealthBar(screenPos.value(), height, width, entity.health);
        }
        
        // Name
        if (m_config.names) {
            DrawText(screenPos.value(), entity.name, color);
        }
        
        // Distance
        if (m_config.distance) {
            char distText[32];
            sprintf_s(distText, "%.0fm", entity.distance / 100.0f);
            DrawText(Vector2(screenPos->x, screenPos->y + 5), distText, color);
        }
        
        // Head dot
        if (m_config.headDot) {
            DrawHeadDot(headScreen.value(), color);
        }
    }
}