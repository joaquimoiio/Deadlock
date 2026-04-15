"""
setup_chain.py
Use CE para achar o pawn, este script descobre PAWN_COMPONENT_OFF e verifica tudo.

Passos:
1. Entre numa partida
2. No CE: scan para HP atual → mude HP → next scan → repita até 1-5 endereços
3. O endereço que rastreia HP = pawn + 0x2D0
4. Informe aqui: pawn_addr = endereço - 0x2D0
"""

import pymem, pymem.process, struct

HEALTH_OFF   = 0x2D0
PAWN_PTR_OFF = 0x490
HEAP_LO      = 0x000100000000
HEAP_HI      = 0x7FF000000000

pm     = pymem.Pymem("deadlock.exe")
client = pymem.process.module_from_name(pm.process_handle, "client.dll")
cb     = client.lpBaseOfDll
csz    = client.SizeOfImage

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

# --- Acha controller ---
lp_off = find_lp_offset()
ctrl   = rq(cb + lp_off)
hero   = ri(ctrl + 0x90C)
team   = struct.unpack("<b", pm.read_bytes(ctrl + 0x3F3, 1))[0]
print(f"controller @ 0x{ctrl:X}  hero_id={hero}  team={team}")
print()

# --- Pede pawn do CE ---
print("No CE: scan pelo HP atual → mude HP → next scan → repita.")
print("O endereço que rastreia HP = pawn+0x2D0")
print()
raw = input("Cole o ENDEREÇO DO HP (pawn+0x2D0) ou do pawn base: ").strip()
addr = int(raw, 16)

# Detecta se é hp_addr ou pawn_addr
hp_test = ri(addr)
hp_test2 = ri(addr - HEALTH_OFF) if addr > HEALTH_OFF else 0
print(f"  Valor em {addr:#X} = {hp_test}")
print(f"  Valor em {addr-HEALTH_OFF:#X} = {hp_test2}")

if 1 <= hp_test <= 10000:
    pawn = addr - HEALTH_OFF
    print(f"  → Parece endereço de HP. pawn base = {pawn:#X}")
elif 1 <= hp_test2 <= 10000:
    pawn = addr
    print(f"  → Parece pawn base direto. HP = {hp_test2}")
else:
    print("  Não detectei HP válido. Usando como pawn base mesmo assim.")
    pawn = addr

hp = ri(pawn + HEALTH_OFF)
print(f"\npawn @ {pawn:#X}  HP={hp}")

# --- Descobre PAWN_COMPONENT_OFF ---
print(f"\nProcurando cadeia ctrl → inter → pawn em 0x100000 bytes do controller...")
found = []
chunk = pm.read_bytes(ctrl, 0x100000)
for i in range(0, len(chunk) - 8, 8):
    inter = struct.unpack_from("<Q", chunk, i)[0]
    if not (HEAP_LO < inter < HEAP_HI):
        continue
    try:
        candidate = rq(inter + PAWN_PTR_OFF)
        if candidate == pawn:
            found.append(("2hop_490", i, inter))
    except: pass
    # Direto também
    if inter == pawn:
        found.append(("direto", i, inter))

# Tenta outros PAWN_PTR_OFF se não achou
if not found:
    print("  Não achei com PAWN_PTR_OFF=0x490. Tentando outros...")
    for ppo in range(0x480, 0x520, 8):
        for i in range(0, len(chunk) - 8, 8):
            inter = struct.unpack_from("<Q", chunk, i)[0]
            if not (HEAP_LO < inter < HEAP_HI):
                continue
            try:
                candidate = rq(inter + ppo)
                if candidate == pawn:
                    found.append((f"2hop_{ppo:X}", i, inter))
            except: pass

if not found:
    print("  ❌ Cadeia não encontrada em 0x100000 bytes.")
    print("  PAWN_COMPONENT_OFF pode estar além de 0x100000 ou a cadeia é diferente.")
else:
    print(f"\n  ✅ Cadeia encontrada:")
    for kind, comp_off, inter in found:
        print(f"    tipo={kind}  PAWN_COMPONENT_OFF=0x{comp_off:X}  inter=0x{inter:X}")

    best = found[0]
    new_off = best[1]
    print(f"\n  >> Novo PAWN_COMPONENT_OFF: 0x{new_off:X}")
    print(f"  >> Adicione 0x{new_off:X} à lista no universal_tester.py e outros scripts!")

# --- Verifica offsets do pawn ---
print(f"\n--- Verificando offsets a partir do pawn ---")
print(f"  HP (0x2D0):      {ri(pawn + 0x2D0)}")
print(f"  hero_id (ctrl+0x90C): {ri(ctrl + 0x90C)}")

# game_scene_node
for gsn_off in [0x308, 0x318, 0x328, 0x338, 0x3A8, 0x3B8, 0x3C0]:
    try:
        gsn = rq(pawn + gsn_off)
        if HEAP_LO < gsn < HEAP_HI:
            # Testa bone_array em gsn+0x160
            try:
                ba = rq(gsn + 0x160)
                if HEAP_LO < ba < HEAP_HI:
                    f0 = struct.unpack("<f", pm.read_bytes(ba, 4))[0]
                    if -2 <= f0 <= 2:
                        print(f"  gsn @ pawn+0x{gsn_off:X}=0x{gsn:X}  bone_array=0x{ba:X}  rot[0]={f0:.3f} ✅")
                        continue
            except: pass
            print(f"  gsn @ pawn+0x{gsn_off:X}=0x{gsn:X}  (sem bone_array válido em +0x160)")
    except: pass
