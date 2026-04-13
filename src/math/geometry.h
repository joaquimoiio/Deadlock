#pragma once
#include <cmath>
#include <optional>
#include <utility>

struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    float DistanceTo(const Vector3& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
};

struct Vector2 {
    float x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

class Geometry {
public:
    static std::pair<float, float> CalculateAngles(const Vector3& from, const Vector3& to);
    static std::pair<float, float> SmoothAngles(float currentYaw, float currentPitch, 
                                                  float targetYaw, float targetPitch, 
                                                  float maxChange);
    static std::optional<Vector2> WorldToScreen(float viewMatrix[4][4], const Vector3& pos, int width, int height);
    
private:
    static float NormalizeAngle(float angle);
};