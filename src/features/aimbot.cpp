#include "aimbot.h"
#include <windows.h>
#include <cmath>

Aimbot::Aimbot(Memory& memory, OffsetManager& offsets)
    : m_memory(memory), m_offsets(offsets), m_running(false), m_paused(false),
      m_lockedTarget(-1), m_forceHeadUntil(0), m_lastLockLost(0),
      m_lastHeadshotCacheTime(0), m_headshotCache(false),
      m_rng(std::chrono::steady_clock::now().time_since_epoch().count()), m_dist(0.0f, 1.0f) {}

Aimbot::~Aimbot() {
    Stop();
}

void Aimbot::SetConfig(const AimbotConfig& config) {
    m_config = config;
}

void Aimbot::Start() {
    if (m_running) return;
    m_running = true;
    m_paused = false;
    m_thread = std::thread(&Aimbot::Run, this);
}

void Aimbot::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Aimbot::Pause() {
    m_paused = true;
}

void Aimbot::Resume() {
    m_paused = false;
}

Vector3 Aimbot::GetCameraPosition() {
    uintptr_t camera = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.CameraManager() + m_offsets.CameraPtrOffset()
    );
    
    return Vector3(
        m_memory.Read<float>(camera + m_offsets.CameraPosX()),
        m_memory.Read<float>(camera + m_offsets.CameraPosY()),
        m_memory.Read<float>(camera + m_offsets.CameraPosZ())
    );
}

std::pair<float, float> Aimbot::GetCurrentAngles() {
    uintptr_t camera = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.CameraManager() + m_offsets.CameraPtrOffset()
    );
    
    return {
        m_memory.Read<float>(camera + m_offsets.CameraYaw()),
        m_memory.Read<float>(camera + m_offsets.CameraPitch())
    };
}

void Aimbot::SetAngles(float yaw, float pitch, float aimAngle) {
    uintptr_t camera = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.CameraManager() + m_offsets.CameraPtrOffset()
    );
    
    m_memory.Write<float>(camera + m_offsets.CameraYaw(), yaw);
    m_memory.Write<float>(camera + m_offsets.CameraPitch(), pitch - aimAngle);
}

float Aimbot::GetRecoilPunch() {
    PlayerData local = GetPlayer(0);
    if (!local.isValid) return 0.0f;
    
    uintptr_t cameraServices = m_memory.Read<uintptr_t>(
        local.pawn + m_offsets.GetStatic("camera_services")
    );
    
    return m_memory.Read<float>(cameraServices + m_offsets.GetStatic("punch_angle"));
}

Vector3 Aimbot::GetBonePosition(uintptr_t boneArray, int boneIndex) {
    uintptr_t boneAddress = boneArray + (boneIndex * m_offsets.BoneStep());
    return Vector3(
        m_memory.Read<float>(boneAddress),
        m_memory.Read<float>(boneAddress + 4),
        m_memory.Read<float>(boneAddress + 8)
    );
}

PlayerData Aimbot::GetPlayer(int index) {
    PlayerData data{};
    data.isValid = false;
    
    uintptr_t entityList = m_memory.Read<uintptr_t>(
        m_memory.GetClientBase() + m_offsets.EntityList()
    );
    
    uintptr_t listEntry = m_memory.Read<uintptr_t>(
        entityList + 0x8 * ((index & 0x7FFF) >> 9) + 0x10
    );
    
    if (!listEntry) return data;
    
    data.controller = m_memory.Read<uintptr_t>(listEntry + 120 * (index & 0x1FF));
    
    if (index == 0) {
        data.controller = m_memory.Read<uintptr_t>(
            m_memory.GetClientBase() + m_offsets.LocalPlayerController()
        );
    }
    
    if (!data.controller) return data;
    
    uintptr_t pawnHandle = m_memory.Read<uintptr_t>(data.controller + 0x874);
    uintptr_t pawnListEntry = m_memory.Read<uintptr_t>(
        entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10
    );
    data.pawn = m_memory.Read<uintptr_t>(pawnListEntry + 0x78 * (pawnHandle & 0x1FF));
    
    if (!data.pawn) return data;
    
    data.team = m_memory.Read<int>(data.controller + m_offsets.TeamOffset());
    data.health = m_memory.Read<int>(data.pawn + m_offsets.HealthOffset());
    data.heroId = m_memory.Read<int>(data.controller + m_offsets.HeroIdOffset());
    
    uintptr_t gameSceneNode = m_memory.Read<uintptr_t>(data.pawn + m_offsets.GameSceneNode());
    uintptr_t posAddr = gameSceneNode + m_offsets.NodePosition();
    
    data.position = Vector3(
        m_memory.Read<float>(posAddr),
        m_memory.Read<float>(posAddr + 4),
        m_memory.Read<float>(posAddr + 8) + 70.0f
    );
    
    data.boneArray = m_memory.Read<uintptr_t>(gameSceneNode + m_offsets.BoneArray());
    data.isValid = true;
    
    return data;
}

// ==================== ANTI-FLAG METHODS ====================

bool Aimbot::ShouldMissIntentionally() {
    if (!m_config.enabled) return false;
    
    float now = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    if (now - m_antiFlag.lastShotTime < 0.1f) {
        return false;
    }
    
    float missChance = m_config.missChance;
    if (m_antiFlag.isInCooldown) {
        missChance *= 2.0f;
    }
    
    return m_dist(m_rng) < missChance;
}

Vector2 Aimbot::ApplyFlick(const Vector2& targetPos) {
    if (!m_config.flickEnabled) return targetPos;
    
    if (m_dist(m_rng) > 0.3f) return targetPos;
    
    std::uniform_real_distribution<float> flickDist(-m_config.flickAmount, m_config.flickAmount);
    float flickX = flickDist(m_rng);
    float flickY = flickDist(m_rng);
    
    return Vector2(targetPos.x + flickX, targetPos.y + flickY);
}

float Aimbot::GetHumanReactionDelay() {
    if (!m_config.enabled) return 0.0f;
    
    std::uniform_real_distribution<float> dist(
        m_config.reactionTimeMin,
        m_config.reactionTimeMax
    );
    return dist(m_rng);
}

bool Aimbot::ShouldBurstFire() {
    if (!m_config.burstFireEnabled) return false;
    
    float now = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    if (now - m_antiFlag.lastBurstTime > 1.0f) {
        m_antiFlag.burstCount = 0;
    }
    
    if (m_antiFlag.burstCount >= 3) {
        m_antiFlag.burstCount = 0;
        m_antiFlag.lastBurstTime = now;
        return true;
    }
    
    return false;
}

float Aimbot::GetBurstDelay() {
    std::uniform_real_distribution<float> dist(
        m_config.burstFireDelayMin,
        m_config.burstFireDelayMax
    );
    return dist(m_rng);
}

void Aimbot::UpdateAntiFlagStats(bool wasHeadshot) {
    float now = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    m_antiFlag.lastShotTime = now;
    
    if (wasHeadshot) {
        m_antiFlag.consecutiveHeadshots++;
        m_antiFlag.lastHeadshotTime = now;
        m_antiFlag.headshotsLastMinute++;
        m_antiFlag.headshotTimestamps.push_back(now);
        m_antiFlag.burstCount++;
        
        // Remove timestamps older than 60 seconds
        m_antiFlag.headshotTimestamps.erase(
            std::remove_if(m_antiFlag.headshotTimestamps.begin(),
                          m_antiFlag.headshotTimestamps.end(),
                          [now](float t) { return now - t > 60.0f; }),
            m_antiFlag.headshotTimestamps.end()
        );
        
        m_antiFlag.headshotsLastMinute = m_antiFlag.headshotTimestamps.size();
        
        // Check if exceeding max rate
        if (m_antiFlag.headshotsLastMinute > m_config.maxHSRatePerMinute) {
            m_antiFlag.isInCooldown = true;
            m_antiFlag.cooldownUntil = now + m_config.cooldownAfterHighHS;
            m_antiFlag.currentHSReduction = 0.3f;
        }
        
        // Check consecutive headshots
        if (m_antiFlag.consecutiveHeadshots >= m_config.maxConsecutiveHS) {
            m_antiFlag.consecutiveHeadshots = 0;
            m_antiFlag.isInCooldown = true;
            m_antiFlag.cooldownUntil = now + 2.0f;
        }
    } else {
        m_antiFlag.consecutiveHeadshots = 0;
    }
    
    // Check cooldown expiration
    if (m_antiFlag.isInCooldown && now >= m_antiFlag.cooldownUntil) {
        m_antiFlag.isInCooldown = false;
        m_antiFlag.currentHSReduction = 1.0f;
        m_antiFlag.consecutiveHeadshots = 0;
    }
}

float Aimbot::GetAdaptiveHSProbability() {
    float baseProb = m_config.headshotProbability;
    
    if (m_config.adaptiveHSEnabled) {
        baseProb *= m_antiFlag.currentHSReduction;
    }
    
    if (m_antiFlag.isInCooldown) {
        baseProb *= 0.2f;
    }
    
    float now = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    if (now - m_antiFlag.lastHeadshotTime < m_config.minTimeBetweenHS) {
        return 0.0f;
    }
    
    return std::clamp(baseProb, 0.0f, 1.0f);
}

void Aimbot::ResetAggression() {
    m_antiFlag.consecutiveHeadshots = 0;
    m_antiFlag.headshotsLastMinute = 0;
    m_antiFlag.headshotTimestamps.clear();
    m_antiFlag.currentHSReduction = 1.0f;
    m_antiFlag.isInCooldown = false;
    m_antiFlag.burstCount = 0;
}

bool Aimbot::ShouldAimForHead() {
    auto now = std::chrono::steady_clock::now();
    float nowSec = std::chrono::duration<float>(now.time_since_epoch()).count();
    
    if (m_config.headshotOnAcquire && nowSec < m_forceHeadUntil) {
        return true;
    }
    
    float adaptiveProb = GetAdaptiveHSProbability();
    
    if (nowSec - m_lastHeadshotCacheTime >= 0.4f) {
        m_headshotCache = m_dist(m_rng) < adaptiveProb;
        m_lastHeadshotCacheTime = nowSec;
    }
    
    return m_headshotCache;
}

// ==================== MAIN LOOP ====================

void Aimbot::Run() {
    while (m_running) {
        if (m_paused || !m_config.enabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        bool aimKeyPressed = (GetAsyncKeyState(m_config.aimKey) & 0x8000) != 0;
        if (!aimKeyPressed) {
            m_lockedTarget = -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        
        PlayerData local = GetPlayer(0);
        if (!local.isValid || local.health <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        
        Vector3 cameraPos = GetCameraPosition();
        auto [currentYaw, currentPitch] = GetCurrentAngles();
        float recoilPunch = GetRecoilPunch();
        
        // Find best target
        if (m_lockedTarget == -1) {
            int bestTarget = -1;
            float bestScore = 0;
            
            for (int i = 1; i <= 64; i++) {
                PlayerData target = GetPlayer(i);
                if (!target.isValid || target.team == local.team || target.health <= 0) {
                    continue;
                }
                
                if (m_config.targetSelectType == 1) {
                    float dist = cameraPos.DistanceTo(target.position);
                    if (bestTarget == -1 || dist < bestScore) {
                        bestScore = dist;
                        bestTarget = i;
                    }
                } else {
                    auto [yaw, pitch] = Geometry::CalculateAngles(cameraPos, target.position);
                    float dyaw = std::abs(yaw - currentYaw);
                    if (dyaw > 180) dyaw = 360 - dyaw;
                    float dpitch = std::abs(-pitch - currentPitch);
                    float score = dyaw + dpitch;
                    if (score < m_config.fovRadius && (bestTarget == -1 || score < bestScore)) {
                        bestScore = score;
                        bestTarget = i;
                    }
                }
            }
            
            if (bestTarget != -1) {
                m_lockedTarget = bestTarget;
                float nowSec = std::chrono::duration<float>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count();
                if (nowSec - m_lastLockLost > 2.0f) {
                    m_forceHeadUntil = nowSec + 0.4f;
                }
            }
        }
        
        if (m_lockedTarget == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        
        PlayerData target = GetPlayer(m_lockedTarget);
        if (!target.isValid || target.team == local.team || target.health <= 0) {
            m_lockedTarget = -1;
            m_lastLockLost = std::chrono::duration<float>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            continue;
        }
        
        // Anti-Flag: Check for intentional miss
        if (ShouldMissIntentionally()) {
            m_antiFlag.lastMissTime = std::chrono::duration<float>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        
        // Anti-Flag: Burst fire delay
        if (ShouldBurstFire()) {
            float delay = GetBurstDelay();
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(delay * 1000)));
        }
        
        // Get target position (head or body)
        bool aimForHead = ShouldAimForHead();
        Vector3 targetPos;
        
        if (aimForHead && m_config.aimAtHead) {
            targetPos = GetBonePosition(target.boneArray, 7);
            UpdateAntiFlagStats(true);
        } else {
            targetPos = GetBonePosition(target.boneArray, 4);
            UpdateAntiFlagStats(false);
        }
        
        // Human reaction delay
        float reactionDelay = GetHumanReactionDelay();
        if (reactionDelay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(reactionDelay * 1000)));
        }
        
        // Calculate angles
        auto [targetYaw, targetPitch] = Geometry::CalculateAngles(cameraPos, targetPos);
        
        // Apply smoothing
        auto [newYaw, newPitch] = Geometry::SmoothAngles(
            currentYaw, currentPitch, targetYaw, -targetPitch, m_config.smoothSpeed
        );
        
        // Set new angles with recoil compensation
        SetAngles(newYaw, newPitch, recoilPunch);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}