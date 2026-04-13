# Guia para Encontrar Patterns no Deadlock

## 🛠️ Ferramentas Necessárias
- Cheat Engine 7.5+
- Deadlock rodando

## 📖 Passo a Passo

### 1. Encontrando um offset (ex: health)
1. Anexe CE ao `deadlock.exe`
2. Procure seu valor de vida (4 bytes)
3. Tome dano, refine a busca
4. Encontre o endereço →右键 → "Find what writes to this address"
5. Copie a instrução: `mov [rax+0x354], ecx`
6. O offset é `0x354`

### 2. Criando Pattern
1. Encontre a instrução no memory viewer
2. Copie os bytes únicos
3. Substitua endereços relativos por `?`
4. Exemplo: `48 8B 1D ? ? ? ? 48 8B 6C 24`

### 3. Testando o Pattern
1. CE → Memory View → Search → Array of bytes
2. Cole o pattern
3. Deve encontrar EXATAMENTE 1 resultado

## 🔍 Patterns do Deadlock

| Nome | Pattern |
|------|---------|
| local_player_controller | `48 8B 1D ? ? ? ? 48 8B 6C 24` |
| view_matrix | `48 8D ? ? ? ? ? 48 C1 E0 06 48 03 C1 C3` |
| entity_list | `48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1 EB` |
| camera_manager | `48 8D 3D ? ? ? ? 8B D9` |

## ⚠️ Quando Patterns Quebram
- **Update pequeno**: patterns ainda funcionam, offsets mudam
- **Update médio**: patterns precisam de mais wildcards
- **Update grande**: patterns morrem, precisa criar novos