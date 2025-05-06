# 🚦 Semáforo Inteligente com Modo Noturno – RP2040 + FreeRTOS

Este projeto simula um **semáforo inteligente para pedestres** utilizando a placa **BitDogLab (RP2040)** com suporte a **FreeRTOS**, **display OLED** e uma **matriz WS2812 (5x5)**. O sistema alterna entre **modo normal** e **modo noturno**, com feedback sonoro e visual.

## 📋 Funcionalidades

### 🟢 Modo Normal
- Ciclo entre verde → amarelo → vermelho.
- Sinalização sonora via buzzer:
  - Verde: beep curto a cada 1s
  - Amarelo: beeps rápidos
  - Vermelho: tom contínuo
- Exibição de bonequinho na matriz WS2812:
  - Verde: boneco verde
  - Vermelho: boneco vermelho
- Display OLED com o estado atual.

### 🌙 Modo Noturno
- Pisca LED amarelo a cada 2s.
- Matriz WS2812 inteira pisca em amarelo.
- Buzzer emite beep lento.
- Display OLED mostra “Modo Noturno: Piscando”.

### 🎮 Controles
- **Botão A (GPIO 5):** alterna entre modos.
- **Botão B (GPIO 6):** reinicia a placa em modo BOOTSEL.

---

## 🧰 Tecnologias Utilizadas

- **RP2040 (BitDogLab)**
- **FreeRTOS**: controle multitarefa
- **PIO (Programável I/O)**: controle da matriz WS2812
- **I2C**: comunicação com o display OLED
- **GPIOs**: controle de buzzer, LEDs, botões

---

## 📁 Estrutura do Projeto

📦projeto_semaforo_inteligente
┣ 📂lib
┃ ┣ ssd1306.c / .h
┃ ┣ font.h
┃ ┣ ws2812.c / .h / .pio
┃ ┣ FreeRTOSConfig.h
┣ main.c
┣ CMakeLists.txt
┣ wokwi.toml
┣ .gitignore
┗ README.md