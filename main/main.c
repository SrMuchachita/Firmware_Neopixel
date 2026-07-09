#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "led_strip.h"
#include "ota_http.h"

/* ── WiFi ── edita aqui ─────────────────────────────────────────────────── */
#define WIFI_SSID   "WTP TALLER"
#define WIFI_PASS   "24012024"

/* ── OTA ── edita aqui ──────────────────────────────────────────────────── */
#define OTA_VERSION_URL  "https://raw.githubusercontent.com/SrMuchachita/Firmware_Neopixel/main/version.json"
#define OTA_FIRMWARE_URL "https://github.com/SrMuchachita/Firmware_Neopixel/releases/latest/download/firmware.bin"
#define OTA_CHECK        60          /* segundos entre chequeos */

/* ── LED ────────────────────────────────────────────────────────────────── */
#define PIN_LED_DATA   GPIO_NUM_48
#define LED_COUNT      1

#define COLOR_ROJO      {255,   0,   0}
#define COLOR_VERDE     {  0, 255,   0}
#define COLOR_AZUL      {  0,   0, 255}
#define COLOR_AMARILLO  {255, 255,   0}
#define COLOR_MAGENTA   {255,   0, 255}
#define COLOR_CYAN      {  0, 255, 255}
#define COLOR_BLANCO    {255, 255, 255}
#define COLOR_NARANJA   {255, 165,   0}
#define COLOR_ROSA      {255, 192, 203}
#define COLOR_VERDE_LIMA { 50, 205,  50}
#define COLOR_CELESTE   {  0, 191, 255}
#define COLOR_INDIGO    { 75,   0, 130}
#define COLOR_VIOLETA   {238, 130, 238}
#define COLOR_DORADO    {255, 215,   0}
#define COLOR_MARRON    {165,  42,  42}
#define COLOR_GRIS      {128, 128, 128}
#define COLOR_APAGADO   {  0,   0,   0}

#define COLOR_SELECCIONADO   COLOR_ROJO
// #define COLOR_SELECCIONADO   COLOR_VERDE
// #define COLOR_SELECCIONADO   COLOR_AZUL
// #define COLOR_SELECCIONADO   COLOR_AMARILLO
// #define COLOR_SELECCIONADO   COLOR_MAGENTA
// #define COLOR_SELECCIONADO   COLOR_CYAN
// #define COLOR_SELECCIONADO   COLOR_BLANCO
// #define COLOR_SELECCIONADO   COLOR_NARANJA
// #define COLOR_SELECCIONADO   COLOR_ROSA
// #define COLOR_SELECCIONADO   COLOR_VERDE_LIMA
// #define COLOR_SELECCIONADO   COLOR_CELESTE
// #define COLOR_SELECCIONADO   COLOR_INDIGO
// #define COLOR_SELECCIONADO   COLOR_VIOLETA
// #define COLOR_SELECCIONADO   COLOR_DORADO
// #define COLOR_SELECCIONADO   COLOR_MARRON
// #define COLOR_SELECCIONADO   COLOR_GRIS
// #define COLOR_SELECCIONADO   COLOR_APAGADO

static const char *TAG = "main";

/* ── WiFi event handler ─────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi desconectado. Reconectando...");
        esp_wifi_connect();

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "WiFi conectado. IP: " IPSTR, IP2STR(&e->ip_info.ip));

        /* Notificar al componente OTA que ya hay red */
        ota_http_notify_connected();
    }
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cfg = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando a '%s'...", WIFI_SSID);
}

/* ── app_main ────────────────────────────────────────────────────────────── */

void app_main(void)
{
    /* NVS (necesario para WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* WiFi */
    wifi_init();

    /* OTA en segundo plano — no bloquea nada */
    ota_http_config_t ota_cfg = {
        .version_url        = OTA_VERSION_URL,
        .firmware_url       = OTA_FIRMWARE_URL,
        .check_interval_sec = OTA_CHECK,
    };
    ota_http_start(&ota_cfg);

    /* ── LED NeoPixel (codigo original sin cambios) ─────────────────────── */
    ESP_LOGI(TAG, "Inicializando NeoPixel...");

    led_strip_config_t strip_config = {
        .strip_gpio_num = PIN_LED_DATA,
        .max_leds = LED_COUNT,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    led_strip_handle_t led_strip;
    ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar LED: %s", esp_err_to_name(ret));
        return;
    }

    uint8_t color[3] = COLOR_SELECCIONADO;
    ESP_LOGI(TAG, "Color elegido: RGB(%d, %d, %d)", color[0], color[1], color[2]);

    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, color[0], color[1], color[2]));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
