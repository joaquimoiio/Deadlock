#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

class Memory {
private:
    HANDLE m_hProcess;
    DWORD m_pid;
    uintptr_t m_clientBase;
    uintptr_t m_engineBase;
    
public:
    Memory();
    ~Memory();
    
    bool Attach(const std::string& processName = "deadlock.exe");
    uintptr_t GetModuleBase(const std::string& moduleName);
    
    template<typename T>
    T Read(uintptr_t address) {
        T buffer{};
        ReadProcessMemory(m_hProcess, (LPCVOID)address, &buffer, sizeof(T), nullptr);
        return buffer;
    }
    
    template<typename T>
    void Write(uintptr_t address, T value) {
        WriteProcessMemory(m_hProcess, (LPVOID)address, &value, sizeof(T), nullptr);
    }
    
    std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size);
    
    uintptr_t GetClientBase() const { return m_clientBase; }
    bool IsValid() const { return m_hProcess != nullptr; }
    
private:
    DWORD GetProcessId(const std::string& processName);
};