"""
Ferramenta para verificar patterns no Deadlock
Uso: python test_pattern.py
"""

import pymem
import pymem.process
import struct

def find_pattern(pm, pattern, module_name="client.dll"):
    """
    Procura um pattern na memória do Deadlock
    
    Args:
        pm: Objeto Pymem
        pattern: String com bytes (ex: "48 8B 1D ? ? ? ? 48 8B 6C 24")
        module_name: Nome do módulo (client.dll)
    
    Returns:
        Endereço encontrado ou None
    """
    
    # Pega o módulo
    module = pymem.process.module_from_name(pm.process_handle, module_name)
    if not module:
        print(f"❌ Módulo {module_name} não encontrado!")
        return None
    
    print(f"📦 {module_name} base: 0x{module.lpBaseOfDll:X}, tamanho: {module.SizeOfImage} bytes")
    
    # Lê a memória do módulo
    try:
        module_bytes = pm.read_bytes(module.lpBaseOfDll, module.SizeOfImage)
    except Exception as e:
        print(f"❌ Falha ao ler memória: {e}")
        return None
    
    # Converte pattern para bytes (None = wildcard)
    pattern_bytes = []
    for byte_str in pattern.split():
        if byte_str == '?':
            pattern_bytes.append(None)
        else:
            pattern_bytes.append(int(byte_str, 16))
    
    # Procura o pattern
    pattern_len = len(pattern_bytes)
    for i in range(len(module_bytes) - pattern_len):
        match = True
        for j, byte in enumerate(pattern_bytes):
            if byte is not None and module_bytes[i + j] != byte:
                match = False
                break
        
        if match:
            address = module.lpBaseOfDll + i
            print(f"✅ Pattern encontrado! Endereço: 0x{address:X}")
            return address
    
    print(f"❌ Pattern NÃO encontrado!")
    return None

def extract_offset_from_pattern(pm, pattern, module_name="client.dll", offset=3, extra=7):
    """
    Extrai o offset relativo do pattern encontrado
    
    Args:
        pm: Objeto Pymem
        pattern: String com bytes
        module_name: Nome do módulo
        offset: Quantos bytes pular após o match
        extra: Valor extra para somar
    
    Returns:
        Offset relativo (int) ou None
    """
    
    module = pymem.process.module_from_name(pm.process_handle, module_name)
    if not module:
        print(f"❌ Módulo {module_name} não encontrado!")
        return None
    
    # Lê a memória
    try:
        module_bytes = pm.read_bytes(module.lpBaseOfDll, module.SizeOfImage)
    except Exception as e:
        print(f"❌ Falha ao ler memória: {e}")
        return None
    
    # Converte pattern
    pattern_bytes = []
    for byte_str in pattern.split():
        if byte_str == '?':
            pattern_bytes.append(None)
        else:
            pattern_bytes.append(int(byte_str, 16))
    
    # Procura o pattern
    pattern_len = len(pattern_bytes)
    for i in range(len(module_bytes) - pattern_len):
        match = True
        for j, byte in enumerate(pattern_bytes):
            if byte is not None and module_bytes[i + j] != byte:
                match = False
                break
        
        if match:
            # Pega os bytes do offset relativo
            offset_bytes = module_bytes[i + offset : i + offset + 4]
            relative = struct.unpack("<i", offset_bytes)[0]
            result = module.lpBaseOfDll + i + relative + extra
            offset_value = result - module.lpBaseOfDll
            print(f"✅ Pattern encontrado! Offset: 0x{offset_value:X}")
            return offset_value
    
    print(f"❌ Pattern NÃO encontrado!")
    return None

def test_pattern_simple(pattern, module_name="client.dll"):
    """Teste simples: verifica se o pattern existe"""
    print(f"\n{'='*60}")
    print(f"Testando pattern: {pattern}")
    print(f"Módulo: {module_name}")
    print(f"{'='*60}")
    
    pm = None
    try:
        # Anexa ao Deadlock
        pm = pymem.Pymem("deadlock.exe")
        print(f"✅ Conectado ao Deadlock (PID: {pm.process_id})")
        
        # Procura o pattern
        result = find_pattern(pm, pattern, module_name)
        return result is not None
        
    except pymem.exception.ProcessNotFound:
        print("❌ Deadlock não está rodando!")
        return False
    except Exception as e:
        print(f"❌ Erro: {e}")
        return False
    finally:
        # Pymem não precisa de close, apenas deletar a referência
        if pm:
            del pm

def test_with_offset_extraction(pattern, module_name="client.dll", offset=3, extra=7):
    """Testa e extrai o offset do pattern"""
    print(f"\n{'='*60}")
    print(f"Extraindo offset do pattern: {pattern}")
    print(f"{'='*60}")
    
    pm = None
    try:
        pm = pymem.Pymem("deadlock.exe")
        print(f"✅ Conectado ao Deadlock (PID: {pm.process_id})")
        
        result = extract_offset_from_pattern(pm, pattern, module_name, offset, extra)
        return result
        
    except pymem.exception.ProcessNotFound:
        print("❌ Deadlock não está rodando!")
        return None
    except Exception as e:
        print(f"❌ Erro: {e}")
        return None
    finally:
        if pm:
            del pm

def test_all_patterns():
    """Testa todos os patterns do projeto"""
    
    patterns = {
        "local_player_controller": {
            "pattern": "48 8B 1D ? ? ? ? 48 8B 6C 24",
            "offset": 3,
            "extra": 7
        },
        "view_matrix": {
            "pattern": "48 8D ? ? ? ? ? 48 C1 E0 06 48 03 C1 C3",
            "offset": 3,
            "extra": 7
        },
        "entity_list": {
            "pattern": "48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1 EB",
            "offset": 3,
            "extra": 7
        },
        "camera_manager": {
            "pattern": "48 8D 3D ? ? ? ? 8B D9",
            "offset": 3,
            "extra": 7
        },
    }
    
    results = {}
    offsets = {}
    
    for name, data in patterns.items():
        result = test_pattern_simple(data["pattern"])
        results[name] = result
        
        if result:
            # Tenta extrair o offset
            offset = test_with_offset_extraction(
                data["pattern"], 
                "client.dll", 
                data["offset"], 
                data["extra"]
            )
            offsets[name] = offset
        
        print()
    
    # Resumo final
    print("\n" + "="*60)
    print("RESUMO FINAL")
    print("="*60)
    
    for name, result in results.items():
        status = "✅ OK" if result else "❌ QUEBRADO"
        offset_info = f" (offset: 0x{offsets[name]:X})" if offsets.get(name) else ""
        print(f"{status} - {name}{offset_info}")
    
    return results

def interactive_mode():
    """Modo interativo - usuário digita o pattern"""
    print("\n🔍 MODO INTERATIVO")
    print("Digite o pattern para testar (ex: 48 8B 1D ? ? ? ? 48 8B 6C 24)")
    print("Digite 'sair' para encerrar\n")
    
    while True:
        pattern = input("Pattern: ").strip()
        if pattern.lower() == 'sair':
            break
        if not pattern:
            continue
        
        test_pattern_simple(pattern)
        
        # Pergunta se quer extrair offset
        extract = input("Extrair offset? (s/n): ").strip().lower()
        if extract == 's':
            test_with_offset_extraction(pattern)

def quick_test():
    """Teste rápido dos patterns mais importantes"""
    
    print("\n" + "="*60)
    print("   TESTE RÁPIDO - Patterns Críticos")
    print("="*60)
    
    critical_patterns = [
        ("local_player_controller", "48 8B 1D ? ? ? ? 48 8B 6C 24"),
        ("view_matrix", "48 8D ? ? ? ? ? 48 C1 E0 06 48 03 C1 C3"),
    ]
    
    all_ok = True
    
    pm = None
    try:
        pm = pymem.Pymem("deadlock.exe")
        print(f"✅ Conectado ao Deadlock (PID: {pm.process_id})\n")
        
        for name, pattern in critical_patterns:
            print(f"Testando {name}...")
            result = find_pattern(pm, pattern)
            if not result:
                all_ok = False
            print()
        
        if all_ok:
            print("\n🎉 Todos os patterns críticos estão OK!")
        else:
            print("\n⚠️ Alguns patterns falharam. Execute o modo completo.")
            
    except pymem.exception.ProcessNotFound:
        print("❌ Deadlock não está rodando!")
    except Exception as e:
        print(f"❌ Erro: {e}")
    finally:
        if pm:
            del pm

def main():
    print("="*60)
    print("   DeadUnlock Pattern Validator v2.1")
    print("="*60)
    print("\nCertifique-se que o Deadlock está RODANDO!\n")
    
    # Menu
    print("Opções:")
    print("  1 - Teste rápido (apenas patterns críticos)")
    print("  2 - Testar todos os patterns do projeto")
    print("  3 - Modo interativo (digitar pattern)")
    print("  4 - Extrair offset de um pattern")
    print("  5 - Sair")
    
    choice = input("\nEscolha: ").strip()
    
    if choice == "1":
        quick_test()
    elif choice == "2":
        test_all_patterns()
    elif choice == "3":
        interactive_mode()
    elif choice == "4":
        pattern = input("Digite o pattern: ").strip()
        if pattern:
            test_with_offset_extraction(pattern)
    else:
        print("Saindo...")

if __name__ == "__main__":
    main()