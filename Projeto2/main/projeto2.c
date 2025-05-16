/*
 * SPDX-FileCopyrightText: 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"

static const char* TAG = "MyModule";
static const char* TAG_GPIO = "GPIO";


/*========================================================================================================
ÁREA DE FUNÇÃO: INTERRUPÇÃO INICIO
SEÇÃO DE CONFIGURAÇÃO DE PINOS DE ENTRADA E SAÍDA
ATIVAÇÃO DE FILA (QUEUE)
TRATAMENTO DE INTERRUPÇÃO (TASK)
========================================================================================================*/
#define BOTAO_0    21
#define BOTAO_1    22
#define BOTAO_2    23
#define GPIO_INPUT_PIN_SEL  ((1ULL<<BOTAO_0) | (1ULL<<BOTAO_1) | (1ULL<<BOTAO_2))

#define LED     2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<LED)

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t evento_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(evento_queue, &gpio_num, NULL);
}



static void gpio_task_led_botao(void* arg)
{
    uint32_t io_num;
    int ESTADO_LED = 0;

     //Configuração zerada para botões.
    gpio_config_t io_conf = {};
    //interrupção borda de descida
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //setando como entrada apenas
    io_conf.mode = GPIO_MODE_INPUT;
    //mascara para os pinos 21, 22 e 23
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //disabilitando o pull down
    io_conf.pull_down_en = 0;
    //habilitando pull up
    io_conf.pull_up_en = 1;
    //Configura o GPIO
    gpio_config(&io_conf);

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set GPIO2
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

     //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BOTAO_0, gpio_isr_handler, (void*) BOTAO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BOTAO_1, gpio_isr_handler, (void*) BOTAO_1);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BOTAO_2, gpio_isr_handler, (void*) BOTAO_2);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

    for (;;) {
        if (xQueueReceive(evento_queue, &io_num, portMAX_DELAY)) {
            switch(io_num){
                case 21:                    
                    ESTADO_LED = 1; 
                    gpio_set_level(LED, ESTADO_LED);      
                    ESP_LOGI(TAG_GPIO,"LIGA LED");
                    break;          

                case 22:                    
                    ESTADO_LED = 0;                    
                    gpio_set_level(LED, ESTADO_LED);
                    ESP_LOGI(TAG_GPIO,"DESLIGA LED"); 
                    break;               

                case 23:
                    ESTADO_LED = !ESTADO_LED;
                    gpio_set_level(LED, ESTADO_LED);
                    ESP_LOGI(TAG_GPIO,"INVERTE LED LED");
                    break;
                
                default:
                    ESP_LOGI(TAG_GPIO,"Falha da fila");
                    break;
                }
        }
    }
}

/*========================================================================================================
ÁREA DE FUNÇÃO: INTERRUPÇÃO FIM
========================================================================================================*/


void app_main(void){
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG,"This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    ESP_LOGI(TAG,"silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        ESP_LOGI(TAG,"Get flash size failed");
        return;
    }

    ESP_LOGI(TAG,"%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

           ESP_LOGW(TAG,"Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        ESP_LOGI(TAG,"Restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    //create a queue to handle gpio event from isr
    evento_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_led_botao, "gpio_task_intr", 2048, NULL, 10, NULL);

    ESP_LOGI(TAG,"Restarting now.");
    fflush(stdout);
   /* esp_restart();*/
}
