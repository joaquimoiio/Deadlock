"""
Universal Pattern & Static Offset Tester for Deadlock
Testa patterns e static offsets em um único script

Uso: python universal_tester.py
"""

import pymem
import pymem.process
import struct
from typing import Optional, Dict, Any, List, Tuple
import time

class UniversalTester:
    def __init__(self):
        self.pm = None
        self.client_base = None
        self.process_id = None
        # Offsets resolvidos dinamicamente pelos patterns
        self.entity_list_offset = None
        self.local_player_offset = None
        self.camera_manager_offset = None
        
    # ==================== CONEXÃO ====================
    
    def connect(self) -> bool:
        """Conecta ao Deadlock"""
        try:
            self.pm = pymem.Pymem("deadlock.exe")
            self.process_id = self.pm.process_id
            print(f"✅ Conectado ao Deadlock (PID: {self.process_id})")
            
            client = pymem.process.module_from_name(self.pm.process_handle, "client.dll")
            self.client_base = client.lpBaseOfDll
            print(f"📦 client.dll base: 0x{self.client_base:X}")
            print(f"📦 client.dll size: {client.SizeOfImage} bytes\n")
            return True
            
        except pymem.exception.ProcessNotFound:
            print("❌ Deadlock não está rodando!")
            return False
        except Exception as e:
            print(f"❌ Erro: {e}")
            return False
    
    # ==================== PATTERN TESTER ====================
    
    def parse_pattern(self, pattern_str: str) -> Tuple[List[Optional[int]], str]:
        """Converte pattern string para lista de bytes (None = wildcard)"""
        pattern_bytes = []
        error = None
        
        for byte_str in pattern_str.split():
            if byte_str == '?' or byte_str == '??':
                pattern_bytes.append(None)
            else:
                try:
                    pattern_bytes.append(int(byte_str, 16))
                except ValueError:
                    error = f"Byte inválido: '{byte_str}'"
                    return [], error
        
        return pattern_bytes, error
    
    def find_pattern(self, pattern_str: str, module_name: str = "client.dll") -> Optional[int]:
        """Procura um pattern na memória do módulo"""
        
        # Pega o módulo
        module = pymem.process.module_from_name(self.pm.process_handle, module_name)
        if not module:
            print(f"❌ Módulo {module_name} não encontrado!")
            return None
        
        # Lê a memória
        try:
            module_bytes = self.pm.read_bytes(module.lpBaseOfDll, module.SizeOfImage)
        except Exception as e:
            print(f"❌ Falha ao ler memória: {e}")
            return None
        
        # Parse do pattern
        pattern_bytes, error = self.parse_pattern(pattern_str)
        if error:
            print(f"❌ Erro no pattern: {error}")
            return None
        
        # Procura
        pattern_len = len(pattern_bytes)
        for i in range(len(module_bytes) - pattern_len):
            match = True
            for j, byte in enumerate(pattern_bytes):
                if byte is not None and module_bytes[i + j] != byte:
                    match = False
                    break
            
            if match:
                return module.lpBaseOfDll + i
        
        return None
    
    def extract_offset(self, pattern_str: str, module_name: str = "client.dll", 
                       offset: int = 3, extra: int = 7) -> Optional[int]:
        """Extrai o offset relativo do pattern encontrado"""
        
        module = pymem.process.module_from_name(self.pm.process_handle, module_name)
        if not module:
            print(f"❌ Módulo {module_name} não encontrado!")
            return None
        
        try:
            module_bytes = self.pm.read_bytes(module.lpBaseOfDll, module.SizeOfImage)
        except Exception as e:
            print(f"❌ Falha ao ler memória: {e}")
            return None
        
        pattern_bytes, error = self.parse_pattern(pattern_str)
        if error:
            print(f"❌ Erro no pattern: {error}")
            return None
        
        pattern_len = len(pattern_bytes)
        for i in range(len(module_bytes) - pattern_len):
            match = True
            for j, byte in enumerate(pattern_bytes):
                if byte is not None and module_bytes[i + j] != byte:
                    match = False
                    break
            
            if match:
                offset_bytes = module_bytes[i + offset : i + offset + 4]
                relative = struct.unpack("<i", offset_bytes)[0]
                result = module.lpBaseOfDll + i + relative + extra
                return result - module.lpBaseOfDll
        
        return None
    
    # ==================== STATIC OFFSET TESTER ====================
    
    def read_with_offset(self, base_addr: int, offset: int, data_type: str = "int") -> Any:
        """Lê um valor usando um offset"""
        try:
            addr = base_addr + offset
            if data_type == "int":
                return self.pm.read_int(addr)
            elif data_type == "float":
                return self.pm.read_float(addr)
            elif data_type == "longlong":
                return self.pm.read_longlong(addr)
            elif data_type == "byte":
                return self.pm.read_byte(addr)
            else:
                return self.pm.read_int(addr)
        except:
            return None
    
    def test_health_offset(self, offset: int = 0x354) -> Dict:
        """Testa health_offset"""
        result = {"offset": offset, "valid": False, "value": None, "message": ""}

        if not self.entity_list_offset:
            result["message"] = "entity_list offset não resolvido"
            return result

        try:
            entity_list = self.pm.read_longlong(self.client_base + self.entity_list_offset)
            
            for i in range(1, 16):
                try:
                    list_entry = self.pm.read_longlong(entity_list + 0x8 * ((i & 0x7FFF) >> 9) + 0x10)
                    if not list_entry:
                        continue
                    
                    controller = self.pm.read_longlong(list_entry + 120 * (i & 0x1FF))
                    if not controller:
                        continue
                    
                    pawn_handle = self.pm.read_longlong(controller + 0x874)
                    pawn_list_entry = self.pm.read_longlong(entity_list + 0x8 * ((pawn_handle & 0x7FFF) >> 9) + 0x10)
                    pawn = self.pm.read_longlong(pawn_list_entry + 0x78 * (pawn_handle & 0x1FF))
                    
                    if pawn:
                        health = self.read_with_offset(pawn, offset, "int")
                        if health and 1 <= health <= 1000:
                            result["valid"] = True
                            result["value"] = health
                            result["message"] = f"Vida encontrada: {health}"
                            return result
                except:
                    continue
        except Exception as e:
            result["message"] = f"Erro: {e}"
        
        return result
    
    def test_team_offset(self, offset: int = 0x3F3) -> Dict:
        """Testa team_offset"""
        result = {"offset": offset, "valid": False, "value": None, "message": ""}

        lp_off = self.local_player_offset or 0x372F3A0
        try:
            local_controller = self.pm.read_longlong(self.client_base + lp_off)
            if local_controller:
                team = self.read_with_offset(local_controller, offset, "int")
                if team in [2, 3, 4, 5]:
                    result["valid"] = True
                    result["value"] = team
                    result["message"] = f"Time encontrado: {team}"
                    return result
        except Exception as e:
            result["message"] = f"Erro: {e}"
        
        return result
    
    def test_camera_offsets(self, offsets: Dict[str, int]) -> Dict:
        """Testa todos os offsets da câmera"""
        results = {}

        cam_off = self.camera_manager_offset or 0x322C2A0
        try:
            camera_manager = self.pm.read_longlong(self.client_base + cam_off)
            if camera_manager:
                camera = self.read_with_offset(camera_manager, offsets.get("camera_ptr_offset", 0x28), "longlong")
                
                if camera:
                    for name, offset in offsets.items():
                        if name == "camera_ptr_offset":
                            continue
                        value = self.read_with_offset(camera, offset, "float" if "pos" in name or "yaw" in name or "pitch" in name else "int")
                        results[name] = {"offset": offset, "valid": value is not None, "value": value}
        except Exception as e:
            results["error"] = {"valid": False, "message": str(e)}
        
        return results
    
    def test_hero_id_offset(self, offset: int = 0x8D4) -> Dict:
        """Testa hero_id_offset"""
        result = {"offset": offset, "valid": False, "value": None, "message": ""}

        lp_off = self.local_player_offset or 0x372F3A0
        try:
            local_controller = self.pm.read_longlong(self.client_base + lp_off)
            if local_controller:
                hero_id = self.read_with_offset(local_controller, offset, "int")
                if hero_id and 1 <= hero_id <= 100:
                    result["valid"] = True
                    result["value"] = hero_id
                    result["message"] = f"Hero ID: {hero_id}"
                    return result
        except Exception as e:
            result["message"] = f"Erro: {e}"
        
        return result
    
    def test_bone_offsets(self, bone_array_offset: int = 0x210, bone_step: int = 32) -> Dict:
        """Testa bone_array e bone_step"""
        result = {"bone_array": {"offset": bone_array_offset, "valid": False},
                  "bone_step": {"offset": bone_step, "valid": False}}

        if not self.entity_list_offset:
            result["bone_array"]["message"] = "entity_list offset não resolvido"
            return result

        try:
            entity_list = self.pm.read_longlong(self.client_base + self.entity_list_offset)
            
            for i in range(1, 5):
                try:
                    list_entry = self.pm.read_longlong(entity_list + 0x8 * ((i & 0x7FFF) >> 9) + 0x10)
                    if not list_entry:
                        continue
                    
                    controller = self.pm.read_longlong(list_entry + 120 * (i & 0x1FF))
                    if not controller:
                        continue
                    
                    pawn_handle = self.pm.read_longlong(controller + 0x874)
                    pawn_list_entry = self.pm.read_longlong(entity_list + 0x8 * ((pawn_handle & 0x7FFF) >> 9) + 0x10)
                    pawn = self.pm.read_longlong(pawn_list_entry + 0x78 * (pawn_handle & 0x1FF))
                    
                    if pawn:
                        game_scene_node = self.read_with_offset(pawn, 0x330, "longlong")
                        if game_scene_node:
                            bone_array = self.read_with_offset(game_scene_node, bone_array_offset, "longlong")
                            if bone_array:
                                result["bone_array"]["valid"] = True
                                result["bone_array"]["value"] = hex(bone_array)
                                
                                # Testa bone_step lendo um bone
                                bone_x = self.read_with_offset(bone_array + 7 * bone_step, 0, "float")
                                if bone_x and -10000 < bone_x < 10000:
                                    result["bone_step"]["valid"] = True
                                    result["bone_step"]["value"] = bone_step
                                    return result
                except:
                    continue
        except Exception as e:
            result["error"] = str(e)
        
        return result
    
    # ==================== UNIVERSAL TEST ====================
    
    def test_patterns(self, patterns: Dict[str, Tuple[str, int, int, str]]) -> Dict:
        """Testa múltiplos patterns"""
        results = {}
        
        for name, (pattern, offset, extra, module) in patterns.items():
            print(f"\n🔍 Testando pattern: {name}")
            print(f"   Pattern: {pattern}")
            
            # Procura o pattern
            address = self.find_pattern(pattern, module)
            
            if address:
                print(f"   ✅ Pattern encontrado! Endereço: 0x{address:X}")
                
                # Extrai offset
                offset_value = self.extract_offset(pattern, module, offset, extra)
                if offset_value:
                    print(f"   📍 Offset extraído: 0x{offset_value:X}")
                    results[name] = {"found": True, "address": address, "offset": offset_value}
                else:
                    results[name] = {"found": True, "address": address, "offset": None}
            else:
                print(f"   ❌ Pattern NÃO encontrado!")
                results[name] = {"found": False, "address": None, "offset": None}
        
        return results
    
    def test_static_offsets(self, static_offsets: Dict[str, int]) -> Dict:
        """Testa static offsets"""
        results = {}
        
        print("\n" + "="*60)
        print("📋 TESTANDO STATIC OFFSETS")
        print("="*60)
        
        # Testa health
        print("\n🔍 Testando HEALTH_OFFSET...")
        health_result = self.test_health_offset(static_offsets.get("health_offset", 0x354))
        results["health_offset"] = health_result
        print(f"   {'✅' if health_result['valid'] else '❌'} {health_result['message']}")
        
        # Testa team
        print("\n🔍 Testando TEAM_OFFSET...")
        team_result = self.test_team_offset(static_offsets.get("team_offset", 0x3F3))
        results["team_offset"] = team_result
        print(f"   {'✅' if team_result['valid'] else '❌'} {team_result['message']}")
        
        # Testa hero ID
        print("\n🔍 Testando HERO_ID_OFFSET...")
        hero_result = self.test_hero_id_offset(static_offsets.get("hero_id_offset", 0x8D4))
        results["hero_id_offset"] = hero_result
        print(f"   {'✅' if hero_result['valid'] else '❌'} {hero_result['message']}")
        
        # Testa camera
        print("\n🔍 Testando CAMERA OFFSETS...")
        camera_offsets = {
            "camera_ptr_offset": static_offsets.get("camera_ptr_offset", 0x28),
            "camera_pos_x": static_offsets.get("camera_pos_x", 0x38),
            "camera_pos_y": static_offsets.get("camera_pos_y", 0x3C),
            "camera_pos_z": static_offsets.get("camera_pos_z", 0x40),
            "camera_yaw": static_offsets.get("camera_yaw", 0x48),
            "camera_pitch": static_offsets.get("camera_pitch", 0x44),
        }
        camera_results = self.test_camera_offsets(camera_offsets)
        results["camera_offsets"] = camera_results
        for name, data in camera_results.items():
            if name != "error":
                print(f"   {'✅' if data.get('valid') else '❌'} {name}: 0x{data['offset']:X} = {data.get('value', 'N/A')}")
        
        # Testa bones
        print("\n🔍 Testando BONE OFFSETS...")
        bone_results = self.test_bone_offsets(
            static_offsets.get("bone_array", 0x210),
            static_offsets.get("bone_step", 32)
        )
        results["bone_offsets"] = bone_results
        for name, data in bone_results.items():
            if name != "error":
                print(f"   {'✅' if data.get('valid') else '❌'} {name}: 0x{data['offset']:X}")
        
        return results
    
    def run(self):
        """Executa o teste universal"""
        
        print("="*70)
        print("   UNIVERSAL TESTER - Patterns & Static Offsets")
        print("="*70)
        
        if not self.connect():
            return
        
        # ==================== PATTERNS ====================
        print("\n" + "="*60)
        print("📋 TESTANDO PATTERNS")
        print("="*60)
        
        patterns = {
            "local_player_controller": (
                "48 8B 1D ? ? ? ? 48 8B 6C 24",
                3, 7, "client.dll"
            ),
            "view_matrix": (
                "48 8D ? ? ? ? ? 48 C1 E0 06 48 03 C1 C3",
                3, 7, "client.dll"
            ),
            "entity_list": (
                "48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA C1 EB",
                3, 7, "client.dll"
            ),
            "camera_manager": (
                "48 8D 3D ? ? ? ? 8B D9",
                3, 7, "client.dll"
            ),
            "schema_system_interface": (
                "48 89 05 ? ? ? ? 4C 8D 0D ? ? ? ? 0F B6 45 E8 4C 8D 45 E0 33 F6",
                3, 7, "schemasystem.dll"
            ),
        }
        
        pattern_results = self.test_patterns(patterns)

        # Salva offsets resolvidos para uso nos testes estáticos
        if pattern_results.get("entity_list", {}).get("offset"):
            self.entity_list_offset = pattern_results["entity_list"]["offset"]
        if pattern_results.get("local_player_controller", {}).get("offset"):
            self.local_player_offset = pattern_results["local_player_controller"]["offset"]
        if pattern_results.get("camera_manager", {}).get("offset"):
            self.camera_manager_offset = pattern_results["camera_manager"]["offset"]

        # ==================== STATIC OFFSETS ====================
        static_offsets = {
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
        
        static_results = self.test_static_offsets(static_offsets)
        
        # ==================== RESUMO FINAL ====================
        print("\n" + "="*70)
        print("   RESUMO FINAL")
        print("="*70)
        
        print("\n📋 PATTERNS:")
        for name, result in pattern_results.items():
            if result["found"]:
                offset_info = f" (offset: 0x{result['offset']:X})" if result["offset"] else ""
                print(f"   ✅ {name}{offset_info}")
            else:
                print(f"   ❌ {name} - NÃO ENCONTRADO")
        
        print("\n📋 STATIC OFFSETS:")
        for name, result in static_results.items():
            if "error" in result:
                continue
            if name == "camera_offsets":
                for cam_name, cam_result in result.items():
                    if cam_name != "error" and cam_result.get("valid"):
                        print(f"   ✅ {cam_name}: 0x{cam_result['offset']:X}")
                    elif cam_name != "error":
                        print(f"   ❌ {cam_name}: 0x{cam_result['offset']:X}")
            elif name == "bone_offsets":
                for bone_name, bone_result in result.items():
                    if bone_name != "error" and bone_result.get("valid"):
                        print(f"   ✅ {bone_name}: 0x{bone_result['offset']:X}")
                    elif bone_name != "error":
                        print(f"   ❌ {bone_name}: 0x{bone_result['offset']:X}")
            elif isinstance(result, dict) and result.get("valid"):
                print(f"   ✅ {name}: 0x{result['offset']:X}")
            elif isinstance(result, dict):
                print(f"   ❌ {name}: 0x{result['offset']:X}")
        
        print("\n" + "="*70)
        print("✅ Teste concluído!")
        print("="*70)

def main():
    tester = UniversalTester()
    tester.run()

if __name__ == "__main__":
    main()