# 🎮 Mini Console - ESP32 Retro Gaming System

Um console de jogos retrô completo construído com ESP32 e display OLED, featuring 11 jogos clássicos, animação de boot cyberpunk e atualização OTA sem fio.

![Mini Console Demo](https://via.placeholder.com/600x300/000000/FFFFFF?text=MINI+CONSOLE)

## 🚀 Características Principais

### 🎯 Jogos Disponíveis
- **Tetris** - O clássico puzzle de blocos com rotação e pontuação
- **Snake** - Colete comida e cresça sem bater nas paredes
- **Genius** - Jogo de memória com sequências de cores
- **Jogo da Velha** - Tic-tac-toe para dois jogadores
- **Dino Game** - Pule sobre obstáculos no estilo Chrome
- **Pong** - Tênis de mesa clássico para 1 ou 2 jogadores
- **Space Invaders** - Defenda a Terra dos invasores espaciais
- **Breakout** - Quebre todos os blocos com a bola
- **Frogger** - Atravesse ruas e rios perigosos
- **Asteroids** - Pilote uma nave e destrua asteroides
- **Centipede** - Atire na centopeia descendente

### 🌟 Recursos Avançados
- **Animação de Boot Ultra** - Sequência cinematográfica com:
  - Matrix Digital Rain Effect
  - Efeitos Glitch e Holográficos
  - Simulação de Terminal Boot
  - Partículas Cyberpunk
  - Transições Suaves

- **Sistema OTA Completo** - Atualize o código via WiFi
- **Controles Responsivos** - 7 botões com debounce e repeat
- **Menu Navegável** - Interface intuitiva com scroll
- **Persistência de Dados** - Scores e configurações mantidas

## 🛠️ Hardware Necessário

### Componentes
```
• ESP32 DevKit v1
• Display OLED 128x64 (SSD1306)
• 7x Botões Tácteis
• Resistores Pull-up 10kΩ
• Breadboard ou PCB customizada
• Cabos jumper
```

### Pinout
```cpp
// Botões
const int BTN_UP_VAL = 32;      // Cima
const int BTN_DOWN_VAL = 33;    // Baixo  
const int BTN_LEFT_VAL = 14;    // Esquerda
const int BTN_RIGHT_VAL = 12;   // Direita
const int BTN_ENTER_VAL = 25;   // Enter/Select
const int BTN_BACK_VAL = 26;    // Back/Menu
const int BTN_A = 27;           // Botão A

// Display I2C
// SDA -> GPIO 21
// SCL -> GPIO 22
```

## 📦 Instalação

### Dependências
```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
```

### Arduino IDE
1. Instale o ESP32 Board Package
2. Instale as bibliotecas necessárias via Library Manager
3. Configure as credenciais WiFi no código:
```cpp
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";
```

### Upload
1. Conecte o ESP32 via USB
2. Selecione a placa "ESP32 Dev Module"
3. Selecione a porta correta
4. Faça upload do código

## 🎮 Como Usar

### Controles Básicos
- **↑↓←→** - Navegação e movimento nos jogos
- **ENTER** - Selecionar/Confirmar/Atirar
- **BACK** - Voltar ao menu/Pausar
- **A** - Ação especial (varia por jogo)

### Menu Principal
- Use ↑↓ para navegar pelos jogos
- ENTER para selecionar
- BACK sempre retorna ao menu

### Jogos Específicos
Cada jogo possui controles únicos detalhados na tela. Exemplos:
- **Tetris**: ↑ rotaciona, ↓ acelera queda
- **Snake**: Direcionais controlam movimento
- **Space Invaders**: A alterna 1P/2P
- **Dino**: ↑/ENTER pula, ↓ abaixa

## 🔄 Atualização OTA

### Configuração
1. Acesse "OTA UPDATE" no menu
2. Conecte à sua rede WiFi
3. Anote o IP exibido

### Upload via Arduino IDE
1. Vá em Sketch → Upload Using Programmer → Selecione "ESP32 Dev Module"
2. Em Tools → Port, selecione o IP da rede ao invés da porta USB
3. Faça upload normalmente

### Upload via Navegador
Acesse `http://IP_DO_ESP32` para interface web de upload.

## 🏗️ Arquitetura do Código

### Estrutura Modular
```
📁 Mini Console
├── 🎮 Sistema Principal (loop, menu, controles)
├── 🎨 Animação de Boot (matrix, glitch, particles)
├── 🕹️ Engines de Jogos (11 jogos independentes)
├── 📡 Sistema OTA (WiFi, upload, progress)
└── 🖥️ Interface Gráfica (OLED, HUD, menus)
```

### Características Técnicas
- **Frame Rate**: ~60 FPS (16ms loop)
- **Resolução**: 128x64 pixels
- **Memória**: Otimizada para ESP32
- **Debounce**: 30ms com repeat configurável
- **WiFi**: Desligado por padrão para economia

## 🎨 Personalização

### Adicionando Jogos
1. Crie funções `initSeuJogo()`, `updateSeuJogo()`, `drawSeuJogo()`
2. Adicione case no switch principal
3. Inclua no array `gameNames[]`
4. Incremente `NUM_GAMES`

### Modificando Animações
A animação de boot é modular com fases:
- Fase 1: Matrix Rain (0-1s)
- Fase 2: Glitch Transition (1-2.2s)  
- Fase 3: Terminal Boot (2.2-3.6s)
- Fase 4: Final Assembly (3.6-4.4s)

### Ajustando Controles
Modifique as constantes de timing em `ButtonState`:
```cpp
const unsigned long debounceDelay = 30;    // Anti-bounce
const unsigned long repeatDelay = 150;     // Repeat inicial
const unsigned long fastRepeat = 80;       // Repeat rápido
```

## 🐛 Troubleshooting

### Display não funciona
- Verifique conexões I2C (SDA/SCL)
- Confirme endereço 0x3C no código
- Teste com exemplo básico da Adafruit

### Botões não respondem
- Verifique pull-ups (internos habilitados)
- Confirme pinout correto
- Ajuste `debounceDelay` se necessário

### OTA não conecta
- Verifique credenciais WiFi
- Confirme que ESP32 e PC estão na mesma rede
- Teste ping para o IP mostrado

### Performance baixa
- Reduza complexidade gráfica
- Otimize loops principais
- Considere usar Core 0 para tarefas específicas

## 📊 Especificações

| Característica | Valor |
|----------------|--------|
| Plataforma | ESP32 (240MHz dual-core) |
| Display | OLED 128x64 SSD1306 |
| RAM Usada | ~45KB de 320KB |
| Flash Usada | ~180KB de 4MB |
| Jogos | 11 completos |
| Frame Rate | 60 FPS |
| Controles | 7 botões físicos |

## 🤝 Contribuição

Contribuições são bem-vindas! Para contribuir:

1. Fork o projeto
2. Crie uma feature branch (`git checkout -b feature/NovoJogo`)
3. Commit suas mudanças (`git commit -am 'Adiciona novo jogo'`)
4. Push para a branch (`git push origin feature/NovoJogo`)
5. Abra um Pull Request

### Ideias para Contribuição
- Novos jogos clássicos (Pac-Man, Galaga, etc.)
- Sistema de high scores persistente
- Suporte a joysticks analógicos
- Interface web para configuração
- Efeitos sonoros via buzzer
- Multiplayer via WiFi

## 📝 Licença

Este projeto está sob licença MIT. Veja o arquivo `LICENSE` para detalhes.

## 🙏 Agradecimentos

- Adafruit pelas excelentes bibliotecas gráficas
- Comunidade ESP32 pelo suporte
- Jogos clássicos que inspiraram este projeto
- Todos os contribuidores e testadores

## 📞 Suporte

Para dúvidas, problemas ou sugestões:
- Abra uma Issue no GitHub
- Consulte a documentação das bibliotecas
- Verifique os exemplos fornecidos

---

**🎮 Divirta-se jogando! Nostalgia em 128x64 pixels! 🕹️**
