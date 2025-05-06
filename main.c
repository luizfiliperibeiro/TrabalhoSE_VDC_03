#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/ws2812.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "pico/bootrom.h"

// Definições dos pinos
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER 21
#define BOTAO_A 5
#define BOTAO_B 6

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25
#define WS2812_PIN 7
#define IS_RGBW false

bool led_buffer[NUM_PIXELS] = {0}; // Será preenchido conforme bonecos
uint8_t led_r = 0, led_g = 0, led_b = 0; // Cor atual dos LEDs

PIO pio = pio0;
int sm = 0;

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_led_buffer(const bool *figura)
{
    for (int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = figura[i];
    }
}

void atualizar_matriz()
{
    uint32_t cor = urgb_u32(led_r, led_g, led_b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (led_buffer[i])
            put_pixel(cor);
        else
            put_pixel(0);
    }
}

const bool boneco_verde[25] = {
    0,1,0,1,0,
    0,1,1,1,0,
    0,0,1,0,0,
    1,0,1,0,1,
    0,1,0,1,0
};

const bool boneco_vermelho[25] = {
    0,1,0,1,0,
    0,1,1,1,0,
    0,0,1,0,0,
    1,0,1,0,1,
    0,1,0,1,0
};

ws2812_t matriz;

typedef enum {
    SEMAFORO_VERDE,
    SEMAFORO_AMARELO,
    SEMAFORO_VERMELHO,
    SEMAFORO_NOTURNO
} EstadoSemaforo;

volatile EstadoSemaforo estado_semaforo = SEMAFORO_VERDE;

// Flag global para alternar os modos
volatile bool modo_noturno = false;

// Inicialização dos GPIOs
void init_gpio() {
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);

    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
}

// Task para alternar o modo com botão A
void vTaskBotaoA(void *pvParameters) {
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    bool estadoAnterior = true;

    while (true) {
        bool estadoAtual = gpio_get(BOTAO_A);
        if (!estadoAtual && estadoAnterior) {
            modo_noturno = !modo_noturno;
            vTaskDelay(pdMS_TO_TICKS(300)); // debounce
        }
        estadoAnterior = estadoAtual;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Task principal do semáforo
void vModoNormalTask(void *pvParameters) {
    init_gpio();

    while (true) {
        if (!modo_noturno) {
            estado_semaforo = SEMAFORO_VERDE;
            gpio_put(LED_G, 1);
            for (int i = 0; i < 5; i++) {
                gpio_put(BUZZER, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_put(BUZZER, 0);
                vTaskDelay(pdMS_TO_TICKS(900));
                if (modo_noturno) break;
            }
            gpio_put(LED_G, 0);
            if (modo_noturno) continue;

            estado_semaforo = SEMAFORO_AMARELO;
            gpio_put(LED_R, 1);
            gpio_put(LED_G, 1);
            for (int i = 0; i < 10; i++) {
                gpio_put(BUZZER, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_put(BUZZER, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                if (modo_noturno) break;
            }
            gpio_put(LED_R, 0);
            gpio_put(LED_G, 0);
            if (modo_noturno) continue;

            estado_semaforo = SEMAFORO_VERMELHO;
            gpio_put(LED_R, 1);
            for (int i = 0; i < 3; i++) {
                gpio_put(BUZZER, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_put(BUZZER, 0);
                vTaskDelay(pdMS_TO_TICKS(1500));
                if (modo_noturno) break;
            }
            gpio_put(LED_R, 0);
        } else {
            estado_semaforo = SEMAFORO_NOTURNO;
            gpio_put(LED_R, 1);
            gpio_put(LED_G, 1);
            gpio_put(BUZZER, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_put(LED_R, 0);
            gpio_put(LED_G, 0);
            gpio_put(BUZZER, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void vDisplayTask()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    EstadoSemaforo estado_anterior = -1;

    while (true)
    {
        if (estado_anterior != estado_semaforo) {
            estado_anterior = estado_semaforo;

            ssd1306_fill(&ssd, false);

            ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 0);
            ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 10);
            ssd1306_draw_string(&ssd, "SEMAFORO", 30, 20);

            switch (estado_semaforo) {
                case SEMAFORO_VERDE:
                    ssd1306_draw_string(&ssd, "Modo: NORMAL", 15, 34);
                    ssd1306_draw_string(&ssd, "Status:SIGA", 1, 46);
                    break;
                case SEMAFORO_AMARELO:
                    ssd1306_draw_string(&ssd, "Modo: NORMAL", 15, 34);
                    ssd1306_draw_string(&ssd, "Status:ATENCAO", 1, 46);
                    break;
                case SEMAFORO_VERMELHO:
                    ssd1306_draw_string(&ssd, "Modo: NORMAL", 15, 34);
                    ssd1306_draw_string(&ssd, "Status:PARE", 1, 46);
                    break;
                case SEMAFORO_NOTURNO:
                    ssd1306_draw_string(&ssd, " Modo: NOTURNO", 10, 34);
                    ssd1306_draw_string(&ssd, " PISCANDO...", 10, 46);
                    break;
            }

            ssd1306_send_data(&ssd);
        }

        vTaskDelay(pdMS_TO_TICKS(300)); // Reduz chamadas ao display
    }
}

void exibirBonequinho(ws2812_t *matriz, const uint8_t figura[], uint8_t r, uint8_t g, uint8_t b) {
    ws2812_clear(matriz);
    for (int i = 0; i < 25; i++) {
        if (figura[i]) {
            ws2812_set_pixel(matriz, i, r, g, b);
        }
    }
    ws2812_show(matriz);
}

void vTaskMatrizPIO(void *params) {
    while (true) {
        if (!modo_noturno) {
            if (estado_semaforo == SEMAFORO_VERDE) {
                set_led_buffer(boneco_verde);
                led_r = 0; led_g = 255; led_b = 0;
            } else if (estado_semaforo == SEMAFORO_VERMELHO) {
                set_led_buffer(boneco_vermelho);
                led_r = 255; led_g = 0; led_b = 0;
            } else {
                memset(led_buffer, 0, sizeof(led_buffer));
            }
            atualizar_matriz();
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            // Modo Noturno: pisca amarelo
            for (int i = 0; i < NUM_PIXELS; i++) led_buffer[i] = true;
            led_r = 255; led_g = 255; led_b = 0;
            atualizar_matriz();
            vTaskDelay(pdMS_TO_TICKS(1000));
            memset(led_buffer, 0, sizeof(led_buffer));
            atualizar_matriz();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// Função de interrupção para entrar em modo BOOTSEL com botão B
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

int main() {
    // Configuração do botão B (modo BOOTSEL)
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Criação das tasks
    xTaskCreate(vModoNormalTask, "ModoNormal", 256, NULL, 1, NULL);
    xTaskCreate(vTaskBotaoA, "BotaoA", 128, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "TaskDisplay", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskMatrizPIO, "MatrizPIO", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true); // nunca deve chegar aqui
}