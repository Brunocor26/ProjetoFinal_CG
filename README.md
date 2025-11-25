# Projeto Labirinto - CG

Jogo de labirinto multiplayer usando **OpenGL moderno (3.3 Core)** com shaders GLSL e comunicação via sockets TCP.

## Tecnologias

- **OpenGL 3.3 Core** - API gráfica moderna
- **GLSL 3.30** - Shaders vertex e fragment
- **GLFW** - Gestão de janelas e input
- **GLEW** - Extensões OpenGL
- **Sockets TCP** - Comunicação em rede (próxima fase)

## Estrutura do Projeto

```
ProjetoFinal/
├── include/          # Ficheiros header
│   ├── common.h      # Constantes e estruturas comuns
│   ├── labirintos.h  # Mapas e funções do labirinto
│   └── shader.h      # Classe para carregar shaders
├── src/              # Código fonte
│   ├── host.cpp      # PC A (Host/Emissor)
│   └── cliente.cpp   # PC B (Cliente/Recetor)
├── shaders/          # Shaders GLSL
│   ├── vertex.glsl   # Vertex shader
│   └── fragment.glsl # Fragment shader
├── Makefile          # Compilação do projeto
└── info.md           # Plano de desenvolvimento
```

## Dependências

Instalar as bibliotecas necessárias:

```bash
# Ubuntu/Debian
sudo apt-get install libglew-dev libglfw3-dev

# Arch Linux
sudo pacman -S glew glfw-x11

# Fedora
sudo dnf install glew-devel glfw-devel
```

## Compilar

**Usando Makefile:**

```bash
make              # Compila ambos os executáveis
make host         # Compila apenas o host
make cliente      # Compila apenas o cliente
make clean        # Remove os executáveis
```

**Compilação manual:**

```bash
g++ -Iinclude -std=c++11 src/host.cpp -o host -lGLEW -lglfw -lGL
g++ -Iinclude -std=c++11 src/cliente.cpp -o cliente -lGLEW -lglfw -lGL
```

## Executar

⚠️ **ORDEM IMPORTANTE:**

**1️⃣ Primeiro inicia o CLIENTE (PC B - Servidor TCP):**

```bash
./cliente   # Fica a aguardar conexão na porta 8080
            # Verás: "Servidor aguardando conexão na porta 8080..."
            # A janela abre mas SEM jogador (está vazia)
```

**2️⃣ Depois inicia o HOST (PC A - onde começa o jogo):**

```bash
./host              # Conecta a localhost (127.0.0.1)
                    # O jogador COMEÇA AQUI (quadrado vermelho)
# ou
./host 192.168.1.X  # Para conectar a outro PC na rede
```

**Para testar localmente (mesma máquina):**

```bash
# Terminal 1 - PRIMEIRO
./cliente

# Terminal 2 - DEPOIS (o jogo começa aqui)
./host
```

**Como jogar:**

- O jogador começa no **host** (PC A)
- Navega até a linha do meio (túnel aberto à direita)
- Sai pela direita e o jogador é transferido para o **cliente** (PC B)
- Continua jogando no segundo labirinto!

## Controles

- **W/A/S/D**: Movimento (Cima/Esquerda/Baixo/Direita)
- **ESC**: Sair

## Como Jogar

1. **Inicie o cliente primeiro** (`./cliente`) - aguarda na porta 8080
2. **Inicie o host** (`./host`) - o jogador começa aqui
3. **Navegue pelo labirinto** até encontrar o túnel na **linha 10** (meio da tela)
4. **Saia pela direita** quando estiver alinhado com o túnel
5. **O jogador é transferido** via TCP para o PC B
6. **Continue no segundo labirinto** até encontrar a saída!

## Estado Atual

✅ **Fase 1 Completa** - Sistema gráfico funcional com OpenGL moderno

- Janela com GLFW
- Shaders GLSL (vertex + fragment)
- VAO/VBO/EBO para geometria
- Projeção ortográfica via matriz
- Jogador (quadrado vermelho)
- Movimento com teclado
- Colisão com paredes do labirinto
- Limites da janela

✅ **Fase 2 Completa** - Comunicação em rede

- Servidor TCP no cliente (porta 8080)
- Cliente TCP no host
- Thread separada para recepção
- Transferência da struct Jogador

✅ **Fase 3 Completa** - Labirintos com colisão

- Mapas 20x20 com paredes
- Sistema de colisão funcional
- Desenho otimizado com shaders

✅ **Fase 4 Completa** - Integração (Teletransporte Dinâmico)

- **Túnel Aleatório:** O Host gera uma saída em uma linha aleatória a cada execução
- **Adaptação do Cliente:** O Cliente abre a entrada na parede exatamente onde o jogador chega
- Sincronização via variável `jogadorPresente`
- Posicionamento correto ao receber

✅ **Fase 5 Completa** - Objetivo Final (Vitória)

- **Saída Aleatória:** O Cliente gera um ponto de saída (bloco verde) em uma posição aleatória
- **Garantia de Solução:** Algoritmo BFS garante que a saída é sempre alcançável
- **Vitória:** Ao tocar no bloco verde, o jogo termina com mensagem de sucesso

🔲 **Fase 6** - Monstro e IA (opcional/bónus)

```
