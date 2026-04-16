#include "geometry.h"

float Geometry::NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

std::pair<float, float> Geometry::CalculateAngles(const Vector3& from, const Vector3& to) {
    Vector3 delta = to - from;
    
    float yaw = std::atan2(delta.y, delta.x) * 180.0f / 3.14159265358979323846f;
    float pitch = std::atan2(delta.z, std::hypot(delta.x, delta.y)) * 180.0f / 3.14159265358979323846f;
    
    return {yaw, pitch};
}

std::pair<float, float> Geometry::SmoothAngles(float currentYaw, float currentPitch,
                                                 float targetYaw, float targetPitch,
                                                 float maxChange) {
    float diffYaw = targetYaw - currentYaw;
    float diffPitch = targetPitch - currentPitch;
    
    if (diffYaw > 180.0f) diffYaw -= 360.0f;
    if (diffYaw < -180.0f) diffYaw += 360.0f;
    
    if (diffYaw > maxChange) diffYaw = maxChange;
    if (diffYaw < -maxChange) diffYaw = -maxChange;
    if (diffPitch > maxChange) diffPitch = maxChange;
    if (diffPitch < -maxChange) diffPitch = -maxChange;

    float newPitch = currentPitch + diffPitch;
    if (newPitch > 89.0f) newPitch = 89.0f;
    if (newPitch < -89.0f) newPitch = -89.0f;

    return {currentYaw + diffYaw, newPitch};
}

std::optional<Vector2> Geometry::WorldToScreen(float viewMatrix[4][4], const Vector3& pos, int width, int height) {
    float clipCoords[4] = {
        pos.x * viewMatrix[0][0] + pos.y * viewMatrix[0][1] + pos.z * viewMatrix[0][2] + viewMatrix[0][3],
        pos.x * viewMatrix[1][0] + pos.y * viewMatrix[1][1] + pos.z * viewMatrix[1][2] + viewMatrix[1][3],
        pos.x * viewMatrix[2][0] + pos.y * viewMatrix[2][1] + pos.z * viewMatrix[2][2] + viewMatrix[2][3],
        pos.x * viewMatrix[3][0] + pos.y * viewMatrix[3][1] + pos.z * viewMatrix[3][2] + viewMatrix[3][3]
    };
    
    if (clipCoords[3] < 0.1f) {
        return std::nullopt;
    }
    
    float ndcX = clipCoords[0] / clipCoords[3];
    float ndcY = clipCoords[1] / clipCoords[3];
    
    float screenX = (ndcX + 1.0f) / 2.0f * width;
    float screenY = (1.0f - ndcY) / 2.0f * height;
    
    return Vector2(screenX, screenY);
}