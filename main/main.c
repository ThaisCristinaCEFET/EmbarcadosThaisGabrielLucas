/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

 #include <stdio.h>
 #include <inttypes.h>
 #include "sdkconfig.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_chip_info.h"
 #include "esp_flash.h"
 #include "esp_log.h"
 
 static const char* TAG = "MyModule";
 
 void app_main(void)
 {
     /* Print chip information */
     esp_chip_info_t chip_info;
     uint32_t flash_size;
     esp_chip_info(&chip_info);
     ESP_LOGW(TAG,"This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
            (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");
 
     unsigned major_rev = chip_info.revision / 100;
     unsigned minor_rev = chip_info.revision % 100;
     ESP_LOGW(TAG,"silicon revision v%d.%d, ", major_rev, minor_rev);
     if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
         ESP_LOGW(TAG,"Get flash size failed");
         return;
     }
 
     ESP_LOGW(TAG,"%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
 
            ESP_LOGW(TAG,"Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
 
     for (int i = 10; i >= 0; i--) {
         ESP_LOGW(TAG,"Restarting in %d seconds...", i);
         vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
     ESP_LOGW(TAG,"Restarting now.");
     fflush(stdout);
     esp_restart();
 }