"""Utility to find Deadlock memory offsets using Windows APIs."""

from __future__ import annotations

import ctypes
from ctypes import wintypes
import struct
import json
import os
from dataclasses import dataclass
from typing import Dict, Optional, Tuple
import logging

import psutil

from signature_patterns import SIGNATURES, STATIC_OFFSETS

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)

logger = logging.getLogger(__name__)

class MODULEINFO(ctypes.Structure):
    _fields_ = [
        ('lpBaseOfDll', wintypes.LPVOID),
        ('SizeOfImage', wintypes.DWORD),
        ('EntryPoint', wintypes.LPVOID),
    ]

def check_zero(result, func, arguments):
    if not result:
        raise ctypes.WinError(ctypes.get_last_error())
    return result

psapi.EnumProcessModules.restype = wintypes.BOOL
psapi.EnumProcessModules.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
psapi.EnumProcessModules.errcheck = check_zero

psapi.GetModuleBaseNameW.restype = wintypes.DWORD
psapi.GetModuleBaseNameW.argtypes = [wintypes.HANDLE, wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
psapi.GetModuleBaseNameW.errcheck = check_zero

psapi.GetModuleInformation.restype = wintypes.BOOL
psapi.GetModuleInformation.argtypes = [wintypes.HANDLE, wintypes.HMODULE, ctypes.POINTER(MODULEINFO), wintypes.DWORD]
psapi.GetModuleInformation.errcheck = check_zero

kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]

kernel32.ReadProcessMemory.restype = wintypes.BOOL
kernel32.ReadProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.LPVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]

kernel32.CloseHandle.restype = wintypes.BOOL
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]

@dataclass
class Signature:
    pattern: str
    offset: int
    extra: int

    def _parse_pattern(self):
        tokens = self.pattern.split()
        parsed = []
        for tok in tokens:
            if tok in ("?", "??"):
                parsed.append(None)
            else:
                parsed.append(int(tok, 16))
        return parsed

    def find(self, memory: bytes, base_addr: int) -> int:
        pattern = self._parse_pattern()
        pat_len = len(pattern)
        size = len(memory)
        for i in range(size - pat_len):
            match = True
            for j, byte in enumerate(pattern):
                if byte is not None and memory[i + j] != byte:
                    match = False
                    break
            if match:
                offset_bytes = memory[i + self.offset : i + self.offset + 4]
                relative = struct.unpack("<i", offset_bytes)[0]
                result = base_addr + i + relative + self.extra
                return result - base_addr
        return 0

def get_process_handle(process_name: str) -> Optional[wintypes.HANDLE]:
    name = process_name[:-4] if process_name.lower().endswith(".exe") else process_name
    for proc in psutil.process_iter(["pid", "name"]):
        if proc.info["name"].lower() in {name.lower(), process_name.lower()}:
            return kernel32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, proc.info["pid"])
    return None

def get_module_info(handle: wintypes.HANDLE, module_name: str) -> Optional[MODULEINFO]:
    hmods = (wintypes.HMODULE * 1024)()
    needed = wintypes.DWORD()
    psapi.EnumProcessModules(handle, hmods, ctypes.sizeof(hmods), ctypes.byref(needed))
    count = needed.value // ctypes.sizeof(wintypes.HMODULE)
    for i in range(count):
        mod = hmods[i]
        name_buf = ctypes.create_unicode_buffer(1024)
        try:
            psapi.GetModuleBaseNameW(handle, mod, name_buf, len(name_buf))
        except WindowsError:
            continue
        if name_buf.value.lower() == module_name.lower():
            info = MODULEINFO()
            psapi.GetModuleInformation(handle, mod, ctypes.byref(info), ctypes.sizeof(info))
            return info
    return None

def read_process_memory(handle: wintypes.HANDLE, address: int, size: int) -> bytes:
    buffer = (ctypes.c_char * size)()
    bytes_read = ctypes.c_size_t()
    if not kernel32.ReadProcessMemory(handle, ctypes.c_void_p(address), buffer, size, ctypes.byref(bytes_read)):
        raise ctypes.WinError(ctypes.get_last_error())
    return bytes(buffer[: bytes_read.value])

def find_offsets(process_name: str = "deadlock.exe") -> Tuple[Dict[str, int], Dict[str, int]]:
    logger.info("Scanning %s for offsets", process_name)
    sigs = {name: Signature(pattern, offset, extra) for name, (pattern, offset, extra) in SIGNATURES.items()}
    
    handle = get_process_handle(process_name)
    if not handle:
        raise RuntimeError('Game process not found')

    client = get_module_info(handle, 'client.dll')
    if not client:
        kernel32.CloseHandle(handle)
        raise RuntimeError('client.dll not found')

    logger.info("client.dll base: 0x%X", client.lpBaseOfDll)
    
    client_mem = read_process_memory(handle, client.lpBaseOfDll, client.SizeOfImage)
    dynamic_offsets = {}
    
    for name in ["local_player_controller", "view_matrix", "entity_list", "camera_manager"]:
        logger.info("Locating %s", name)
        offset = sigs[name].find(client_mem, client.lpBaseOfDll)
        if offset == 0:
            logger.warning("Failed to find %s! Pattern may be outdated.", name)
        dynamic_offsets[name] = offset
        logger.info("%s: 0x%X", name, offset)

    kernel32.CloseHandle(handle)
    return dynamic_offsets, STATIC_OFFSETS

def save_offsets_to_file(dynamic: Dict[str, int], static: Dict[str, int], output_path: str) -> None:
    full_offsets = {
        "dynamic": dynamic,
        "static": static,
        "version": "1.0",
        "game": "Deadlock",
        "timestamp": __import__('time').time()
    }
    
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    with open(output_path, 'w') as f:
        json.dump(full_offsets, f, indent=2)
    
    logger.info("Saved offsets to %s", output_path)

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="../../config/offsets.json", help="Output path for offsets.json")
    parser.add_argument("--debug", action="store_true", help="Enable debug logging")
    args = parser.parse_args()
    
    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
    
    try:
        dynamic, static = find_offsets()
        save_offsets_to_file(dynamic, static, args.output)
        print("\n✅ Offsets saved successfully!")
        print("\nDynamic offsets:")
        for k, v in dynamic.items():
            print(f"  {k}: 0x{v:X}")
        return 0
    except Exception as e:
        print(f"\n❌ Error: {e}")
        return 1

if __name__ == "__main__":
    exit(main())