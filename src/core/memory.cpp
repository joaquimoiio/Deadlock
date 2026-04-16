#include "memory.h"
#include <tlhelp32.h>
#include <psapi.h>

Memory::Memory() : m_hProcess(nullptr), m_pid(0), m_clientBase(0), m_engineBase(0) {}

Memory::~Memory() {
    if (m_hProcess) {
        CloseHandle(m_hProcess);
    }
}

DWORD Memory::GetProcessId(const std::string& processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        
        if (Process32First(snapshot, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, processName.c_str()) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

bool Memory::Attach(const std::string& processName) {
    m_pid = GetProcessId(processName);
    if (!m_pid) return false;
    
    m_hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, m_pid);
    if (!m_hProcess) return false;
    
    m_clientBase = GetModuleBase("client.dll");
    m_engineBase = GetModuleBase("engine2.dll");

    if (!m_clientBase) {
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
        return false;
    }

    return true;
}

uintptr_t Memory::GetModuleBase(const std::string& moduleName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    
    uintptr_t base = 0;
    if (Module32First(snapshot, &me)) {
        do {
            if (_stricmp(me.szModule, moduleName.c_str()) == 0) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }
    
    CloseHandle(snapshot);
    return base;
}

std::vector<uint8_t> Memory::ReadBytes(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    ReadProcessMemory(m_hProcess, (LPCVOID)address, buffer.data(), size, nullptr);
    return buffer;
}