"""Signature patterns for Deadlock offset scanning.

COMO ATUALIZAR PATTERNS:
1. Abra o Deadlock + Cheat Engine
2. Anexe ao processo deadlock.exe
3. Encontre o valor desejado (ex: view_matrix)
4. Faça "Find what writes to this address"
5. Copie bytes únicos ao redor, use ? para wildcards
6. Atualize aqui e rode dumper novamente
"""

# Mapping of offset names to (pattern, offset, extra)
SIGNATURES = {
    "local_player_controller": (
        "48 8B 1D ? ? ? ? 48 8B 6C 24",
        3,
        7,
    ),
    "view_matrix": (
        "48 8D ? ? ? ? ? 48 C1 E0 06 48 03 C1 C3",
        3,
        7,
    ),
    "entity_list": (
        "48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1 EB",
        3,
        7,
    ),
    "camera_manager": (
        "48 8D 3D ? ? ? ? 8B D9",
        3,
        7,
    ),
    "schema_system_interface": (
        "48 89 05 ? ? ? ? 4C 8D 0D ? ? ? ? 0F B6 45 E8 4C 8D 45 E0 33 F6",
        3,
        7,
    ),
}

# Offsets ESTÁTICOS (não mudam com update do jogo)
STATIC_OFFSETS = {
    "camera_ptr_offset": 0x28,
    "camera_pos_x": 0x38,
    "camera_pos_y": 0x3C,
    "camera_pos_z": 0x40,
    "camera_yaw": 0x48,
    "camera_pitch": 0x44,
    "camera_roll": 0x4C,
    "hero_id_offset": 0x8D4,
    "game_scene_node": 0x330,
    "node_position": 0xD0,
    "team_offset": 0x3F3,
    "health_offset": 0x354,
    "camera_services": 0xF68,
    "punch_angle": 0x40,
    "bone_array": 0x210,
    "bone_step": 32,
}