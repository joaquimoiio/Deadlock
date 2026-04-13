#pragma once
#include <atomic>
#include <thread>
#include <optional>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../math/geometry.h"

struct AimbotConfig {
    // Core settings
    bool enabled = true;
    int aimKey = VK_LBUTTON;
    int targetSelectType = 0;  // 0 = FOV, 1 = Distance
    float fovRadius = 100.0f;
    float smoothSpeed = 5.0f;
    bool aimAtHead = true;
    
    // Headshot settings
    float headshotProbability = 0.25f;
    bool headshotOnAcquire = true;
    
    // Anti-Flag / Humanization System
    bool adaptiveHSEnabled = true;
    int maxConsecutiveHS = 3;
    float minTimeBetweenHS = 0.5f;
    float missChance = 0.05f;
    float reactionTimeMin = 0.08f;
    float reactionTimeMax = 0.25f;
    bool flickEnabled = true;
    float flickAmount = 5.0f;
    int maxHSRatePerMinute = 4;
    float cooldownAfterHighHS = 5.0f;
    bool burstFireEnabled = true;
    float burstFireDelayMin = 0.05f;
    float burstFireDelayMax = 0.15f;
};

struct PlayerData {
    uintptr_t controller;
    uintptr_t pawn;
    Vector3 position;
    int health;
    int team;
    int heroId;
    uintptr_t boneArray;
    bool isValid;
    float aimAngle;
};

class Aimbot {
private:
    Memory& m_memory;
    OffsetManager& m_offsets;
    AimbotConfig m_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_paused;
    std::thread m_thread;
    
    int m_lockedTarget;
    float m_forceHeadUntil;
    float m_lastLockLost;
    float m_lastHeadshotCacheTime;
    bool m_headshotCache;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;
    
    // Anti-Flag tracking
    struct AntiFlagData {
        int consecutiveHeadshots = 0;
        float lastHeadshotTime = 0;
        float lastShotTime = 0;
        int headshotsLastMinute = 0;
        std::vector<float> headshotTimestamps;
        float currentHSReduction = 1.0f;
        float cooldownUntil = 0;
        float lastMissTime = 0;
        bool isInCooldown = false;
        int burstCount = 0;
        float lastBurstTime = 0;
    } m_antiFlag;
    
    // Core methods
    PlayerData GetPlayer(int index);
    Vector3 GetBonePosition(uintptr_t boneArray, int boneIndex);
    void SetAngles(float yaw, float pitch, float aimAngle = 0.0f);
    std::pair<float, float> GetCurrentAngles();
    Vector3 GetCameraPosition();
    float GetRecoilPunch();
    
    // Anti-Flag methods
    bool ShouldAimForHead();
    bool ShouldMissIntentionally();
    Vector2 ApplyFlick(const Vector2& targetPos);
    float GetHumanReactionDelay();
    void UpdateAntiFlagStats(bool wasHeadshot);
    void ResetAggression();
    float GetAdaptiveHSProbability();
    bool ShouldBurstFire();
    float GetBurstDelay();
    
public:
    Aimbot(Memory& memory, OffsetManager& offsets);
    ~Aimbot();
    
    void SetConfig(const AimbotConfig& config);
    void Start();
    void Stop();
    void Pause();
    void Resume();
    bool IsRunning() const { return m_running; }
    void ResetAntiFlag() { ResetAggression(); }
    
private:
    void Run();
};