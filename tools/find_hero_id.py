"""
find_hero_id.py
Lista todos os int32 no range 1-200 do controller e do pawn.
Rode com dois heróis diferentes — o offset que mudar é o hero_id_offset.
"""

import pymem, pymem.process, struct, json, os

LOCAL_PLAYER_OFFSET  = 0x372F3A0
PAWN_COMPONENT_OFF   = 0xF440
PAWN_PTR_OFF         = 0x490
SAVE_FILE            = "hero_id_scan.json"

pm = pymem.Pymem("deadlock.exe")
client = pymem.process.module_from_name(pm.process_handle, "client.dll")
cb = client.lpBaseOfDll

def rq(a): return struct.unpack("<Q", pm.read_bytes(a, 8))[0]

try:
    ctrl         = rq(cb + LOCAL_PLAYER_OFFSET)
    intermediate = rq(ctrl + PAWN_COMPONENT_OFF)
    pawn         = rq(intermediate + PAWN_PTR_OFF)
except Exception:
    print("Erro lendo memoria — jogo ainda carregando? Espere entrar na partida e tente novamente.")
    if os.path.exists(SAVE_FILE):
        print(f"(scan anterior preservado em {SAVE_FILE})")
    exit(1)

print(f"controller @ 0x{ctrl:X}")
print(f"pawn       @ 0x{pawn:X}\n")

def collect(base, name, size=0x1200):
    data = pm.read_bytes(base, size)
    results = {}
    for i in range(0, size - 4, 4):
        v = struct.unpack_from("<i", data, i)[0]
        if 1 <= v <= 200:
            results[f"{name}+0x{i:04X}"] = v
    return results

ctrl_hits = collect(ctrl, "ctrl")
pawn_hits = collect(pawn, "pawn")
all_hits  = {**ctrl_hits, **pawn_hits}

# Se existe scan anterior, compara
if os.path.exists(SAVE_FILE):
    with open(SAVE_FILE) as f:
        prev = json.load(f)

    print("=== COMPARAÇÃO COM SCAN ANTERIOR ===")
    print(f"{'OFFSET':<22}  {'ANTES':>6}  {'AGORA':>6}")
    print("-"*40)
    changed = []
    for key in sorted(set(prev) | set(all_hits)):
        v_before = prev.get(key)
        v_after  = all_hits.get(key)
        if v_before != v_after:
            print(f"  {key:<20}  {str(v_before):>6}  {str(v_after):>6}  <-- MUDOU")
            changed.append((key, v_before, v_after))
        # else: mesmo valor, ignora

    if not changed:
        print("  Nenhum valor mudou (mesmo herói?)")
    else:
        print(f"\n{len(changed)} offset(s) mudaram de valor — candidatos ao hero_id_offset!")

    os.remove(SAVE_FILE)

else:
    # Primeiro scan — salva e mostra candidatos
    with open(SAVE_FILE, "w") as f:
        json.dump(all_hits, f)

    print(f"Scan salvo ({len(all_hits)} candidatos). Valores encontrados:\n")
    print(f"{'OFFSET':<22}  {'VALOR':>6}")
    print("-"*32)
    for key, val in sorted(all_hits.items()):
        print(f"  {key:<20}  {val:>6}")

    print(f"\nAgora entre numa NOVA PARTIDA com um herói DIFERENTE")
    print(f"e rode o script de novo — ele vai mostrar o que mudou.")
