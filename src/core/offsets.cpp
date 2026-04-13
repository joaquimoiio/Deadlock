#include "offsets.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

OffsetManager::OffsetManager() : m_loaded(false) {}

OffsetManager::~OffsetManager() {}

bool OffsetManager::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open offsets file: " << path << std::endl;
        return false;
    }
    
    nlohmann::json data;
    file >> data;
    
    if (data.contains("dynamic")) {
        for (auto& [key, value] : data["dynamic"].items()) {
            if (value.is_number()) {
                m_dynamicOffsets[key] = value.get<uintptr_t>();
            } else if (value.is_string()) {
                m_dynamicOffsets[key] = std::stoull(value.get<std::string>(), nullptr, 16);
            }
        }
    }
    
    if (data.contains("static")) {
        for (auto& [key, value] : data["static"].items()) {
            if (value.is_number()) {
                m_staticOffsets[key] = value.get<uintptr_t>();
            } else if (value.is_string()) {
                m_staticOffsets[key] = std::stoull(value.get<std::string>(), nullptr, 16);
            }
        }
    }
    
    m_loaded = !m_dynamicOffsets.empty();
    return m_loaded;
}

uintptr_t OffsetManager::Get(const std::string& name) const {
    auto it = m_dynamicOffsets.find(name);
    if (it != m_dynamicOffsets.end()) {
        return it->second;
    }
    return 0;
}

uintptr_t OffsetManager::GetStatic(const std::string& name) const {
    auto it = m_staticOffsets.find(name);
    if (it != m_staticOffsets.end()) {
        return it->second;
    }
    return 0;
}