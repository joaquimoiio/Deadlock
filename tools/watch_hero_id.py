"""
watch_hero_id.py
Mostra o hero_id em tempo real. Troque de herói e veja o valor mudar.
"""

import pymem, pymem.process, struct, time, os

PAWN_PTR_OFF = 0x490
HEALTH_OFF   = 0x2D0
HERO_ID_OFF  = 0x90C
HEAP_LO      = 0x000100000000
HEAP_HI      = 0x7FF000000000

pm = pymem.Pymem("deadlock.exe")
client = pymem.process.module_from_name(pm.process_handle, "client.dll")
cb = client.lpBaseOfDll
csz = client.SizeOfImage

def rq(a): return struct.unpack("<Q", pm.read_bytes(a, 8))[0]
def ri(a): return struct.unpack("<i", pm.read_bytes(a, 4))[0]

def scan_pattern(data, pattern):
    plen = len(pattern)
    for i in range(len(data) - plen):
        if all(p is None or data[i+j] == p for j, p in enumerate(pattern)):
            return i
    return -1

def find_lp_offset():
    pattern = [0x48, 0x8B, 0x1D, None, None, None, None, 0x48, 0x8B, 0x6C, 0x24]
    data = pm.read_bytes(cb, csz)
    idx = scan_pattern(data, pattern)
    if idx == -1:
        raise RuntimeError("Pattern não encontrado")
    return idx + 7 + struct.unpack_from("<i", data, idx + 3)[0]

def find_pawn(ctrl):
    for comp_off in [0x5608, 0xF440]:
        try:
            inter = rq(ctrl + comp_off)
            if HEAP_LO < inter < HEAP_HI:
                pawn = rq(inter + PAWN_PTR_OFF)
                if HEAP_LO < pawn < HEAP_HI:
                    hp = ri(pawn + HEALTH_OFF)
                    if 1 <= hp <= 10000:
                        return pawn
        except:
            pass
    candidates = []
    chunk = pm.read_bytes(ctrl, 0x20000)
    for i in range(0, len(chunk) - 8, 8):
        ptr = struct.unpack_from("<Q", chunk, i)[0]
        if not (HEAP_LO < ptr < HEAP_HI):
            continue
        try:
            pawn = rq(ptr + PAWN_PTR_OFF)
            if HEAP_LO < pawn < HEAP_HI:
                hp = ri(pawn + HEALTH_OFF)
                if 1 <= hp <= 10000:
                    candidates.append((hp, pawn))
        except:
            pass
    if candidates:
        candidates.sort(reverse=True)
        return candidates[0][1]
    return None

lp_off = find_lp_offset()
print(f"local_player_offset: 0x{lp_off:X}")
print("Lendo hero_id ao vivo (Ctrl+C para sair)...\n")

last_id = None
while True:
    try:
        ctrl = rq(cb + lp_off)
        if ctrl and HEAP_LO < ctrl < HEAP_HI:
            hero_id = ri(ctrl + HERO_ID_OFF)
            if hero_id != last_id:
                print(f"hero_id = {hero_id}")
                last_id = hero_id
    except:
        pass
    time.sleep(0.5)
