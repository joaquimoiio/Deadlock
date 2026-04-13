#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class OffsetManager {
private:
    std::unordered_map<std::string, uintptr_t> m_dynamicOffsets;
    std::unordered_map<std::string, uintptr_t> m_staticOffsets;
    bool m_loaded;
    
public:
    OffsetManager();
    ~OffsetManager();
    
    bool LoadFromFile(const std::string& path);
    uintptr_t Get(const std::string& name) const;
    uintptr_t GetStatic(const std::string& name) const;
    bool IsLoaded() const { return m_loaded; }
    
    // Convenience getters
    uintptr_t LocalPlayerController() const { return Get("local_player_controller"); }
    uintptr_t ViewMatrix() const { return Get("view_matrix"); }
    uintptr_t EntityList() const { return Get("entity_list"); }
    uintptr_t CameraManager() const { return Get("camera_manager"); }
    
    // Static offsets
    uintptr_t CameraPtrOffset() const { return GetStatic("camera_ptr_offset"); }
    uintptr_t CameraPosX() const { return GetStatic("camera_pos_x"); }
    uintptr_t CameraPosY() const { return GetStatic("camera_pos_y"); }
    uintptr_t CameraPosZ() const { return GetStatic("camera_pos_z"); }
    uintptr_t CameraYaw() const { return GetStatic("camera_yaw"); }
    uintptr_t CameraPitch() const { return GetStatic("camera_pitch"); }
    uintptr_t HeroIdOffset() const { return GetStatic("hero_id_offset"); }
    uintptr_t GameSceneNode() const { return GetStatic("game_scene_node"); }
    uintptr_t NodePosition() const { return GetStatic("node_position"); }
    uintptr_t TeamOffset() const { return GetStatic("team_offset"); }
    uintptr_t HealthOffset() const { return GetStatic("health_offset"); }
    uintptr_t BoneArray() const { return GetStatic("bone_array"); }
    uintptr_t BoneStep() const { return GetStatic("bone_step"); }
};