"""
find_bone_array.py
Acha bone_array_offset e bone_step para Deadlock.
Não precisa de HP exato — qualquer pawn válido serve.
"""

import pymem, pymem.process, struct, os, json, ctypes, ctypes.wintypes

PAWN_PTR_OFF = 0x490
HEALTH_OFF   = 0x2D0
HEAP_LO      = 0x000100000000
HEAP_HI      = 0x7FF000000000

pm     = pymem.Pymem("deadlock.exe")
client = pymem.process.module_from_name(pm.process_handle, "client.dll")
cb     = client.lpBaseOfDll
csz    = client.SizeOfImage

def rq(a): return struct.unpack("<Q", pm.read_bytes(a, 8))[0]
def ri(a): return struct.unpack("<i", pm.read_bytes(a, 4))[0]
def rf(a): return struct.unpack("<f", pm.read_bytes(a, 4))[0]

# --- Pattern scan para local_player_offset ---
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

# --- Acha qualquer pawn válido a partir do controller ---
def find_any_pawn(ctrl):
    """Retorna o primeiro pawn encontrado com HP 1-10000."""
    chunk = pm.read_bytes(ctrl, 0x20000)
    for i in range(0, len(chunk) - 8, 8):
        inter = struct.unpack_from("<Q", chunk, i)[0]
        if not (HEAP_LO < inter < HEAP_HI):
            continue
        try:
            pawn = rq(inter + PAWN_PTR_OFF)
            if HEAP_LO < pawn < HEAP_HI:
                hp = ri(pawn + HEALTH_OFF)
                if 1 <= hp <= 10000:
                    return pawn, i
        except:
            pass
    return None, None

# --- Valida se um ponteiro aponta para dados de bone (floats de rotação entre -1 e 1) ---
def looks_like_bone_data(ptr, step, num_bones=8):
    """
    Numa bone transform (matrix3x4 = 12 floats), os 9 primeiros floats
    são a matriz de rotação com valores entre -1 e 1.
    Verifica isso para N bones consecutivos.
    """
    try:
        data = pm.read_bytes(ptr, num_bones * step)
        valid = 0
        for i in range(num_bones):
            base = i * step
            rot_ok = 0
            for j in range(0, 36, 4):  # 9 floats = 36 bytes de rotação
                v = struct.unpack_from("<f", data, base + j)[0]
                if -1.01 <= v <= 1.01 and v != 0.0:
                    rot_ok += 1
            if rot_ok >= 5:  # pelo menos 5 dos 9 floats de rotação válidos
                valid += 1
        return valid >= num_bones // 2
    except:
        return False

# ============================================================
lp_off = find_lp_offset()
ctrl   = rq(cb + lp_off)
print(f"controller @ 0x{ctrl:X}")

pawn, comp_off = find_any_pawn(ctrl)
if not pawn:
    print("Nenhum pawn encontrado. Entre numa partida primeiro.")
    exit(1)

hp = ri(pawn + HEALTH_OFF)
print(f"pawn       @ 0x{pawn:X}  HP={hp}  (PAWN_COMPONENT_OFF=0x{comp_off:X})\n")

# --- Passo 1: achar game_scene_node ---
print("[Passo 1] Procurando game_scene_node (pawn+offset → heap ptr)...")
gsn_candidates = []
pawn_data = pm.read_bytes(pawn, 0x500)
for off in range(0x300, 0x500, 8):
    ptr = struct.unpack_from("<Q", pawn_data, off)[0]
    if HEAP_LO < ptr < HEAP_HI:
        gsn_candidates.append((off, ptr))

print(f"  {len(gsn_candidates)} ponteiros heap encontrados em pawn+0x300..0x500")

# --- Passo 2: para cada gsn candidato, busca bone_array ---
print("[Passo 2] Buscando bone_array em cada game_scene_node candidato...")
print(f"  (valida matriz de rotação: 9 floats entre -1 e 1)\n")

found = []
for gsn_off, gsn in gsn_candidates:
    try:
        gsn_data = pm.read_bytes(gsn, 0x500)
    except:
        continue
    for i in range(0, len(gsn_data) - 8, 8):
        ptr = struct.unpack_from("<Q", gsn_data, i)[0]
        if not (HEAP_LO < ptr < HEAP_HI):
            continue
        for step in [0x20, 0x30, 0x1E, 0x40]:
            if looks_like_bone_data(ptr, step):
                print(f"  ✅ pawn+0x{gsn_off:X} → gsn → gsn+0x{i:X} → bone_array  step=0x{step:X}")
                found.append((gsn_off, i, step, ptr))
                break

if not found:
    print("Nenhum bone_array encontrado.")
    print("Dica: entre numa partida e rode de novo (personagem precisa estar carregado).")
else:
    print(f"\n{len(found)} candidato(s) encontrados.")
    gsn_off, ba_off, step, ptr = found[0]
    print(f"\nResultado sugerido:")
    print(f"  game_scene_node_offset = 0x{gsn_off:X}")
    print(f"  bone_array_offset      = 0x{ba_off:X}  (de game_scene_node)")
    print(f"  bone_step              = 0x{step:X}")
