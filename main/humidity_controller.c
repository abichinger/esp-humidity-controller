/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "DHT22.h"
#include "settings.h"
#include "rest_server.h"
#include "wifi.h"
#include "relay.h"

#define DHT22_GPIO CONFIG_DHT22_GPIO
#define DHT22_VCC CONFIG_DHT22_VCC
#define SLED_GPIO CONFIG_SLED_GPIO

static const char *TAG = "MAIN";

void DHT_task(void *pvParameter){
   ESP_LOGI(TAG, "Starting DHT Task");

	setDHTgpio( DHT22_GPIO );
   gpio_set_direction(DHT22_VCC, GPIO_MODE_OUTPUT);
   gpio_set_level(DHT22_VCC, 1);
   
   uint8_t error_count = 0;

	while(1) {
	
		int ret = readDHT();
		errorHandler(ret);

      if(ret != DHT_OK){
         error_count += 1;
      }
      else {
         error_count = 0;
      }

      //power cycle DHT22
      if(error_count >= 3){
         ESP_LOGI(TAG, "Power cycling DHT22");
         gpio_set_level(DHT22_VCC, 0);
         vTaskDelay( 1000 / portTICK_RATE_MS );
         gpio_set_level(DHT22_VCC, 1);
         error_count = 0;
      }

      ESP_LOGI(TAG, "Hum: %.1f, Temp: %.1f", getHumidity(), getTemperature());
		
		// -- wait at least 2 sec before reading again ------------
		// The interval of whole process must be beyond 2 seconds !! 
		vTaskDelay( 3000 / portTICK_RATE_MS );
	}
}

void Relay_task(void *pvParameter){
   relay_init();

   float humidity = 0.0;
   settings_t *settings = get_settings();

   while(1) {

      if(settings->mode == Manual) {
         vTaskDelay(3000 / portTICK_PERIOD_MS);
         continue;
      }
   
      humidity = getHumidity();

      if(humidity > settings->on_threshold && !relay_is_on()){
         ESP_LOGI(TAG, "Turning relay on");
         relay_turn_on();
      }
      else if(humidity < settings->off_threshold && relay_is_on() && !relay_turn_off_timer_active()){
         ESP_LOGI(TAG, "Turning relay off in %d seconds", settings->off_delay);
         relay_schedule_turn_off(settings->off_delay);
      }
      vTaskDelay(3000 / portTICK_PERIOD_MS);
   }
}

//TODO: remove when web-interface is fixed 
void Restart_task(void *pvParameter){
   //restart every 24 hours
   double one_day_ms = 24*60*60*1000;
   vTaskDelay(one_day_ms / portTICK_PERIOD_MS);
   ESP_LOGE(TAG, "Restarting...");
   esp_restart();
}

void Blink_task(void *pvParameter){

   gpio_set_direction(SLED_GPIO, GPIO_MODE_OUTPUT);

	while(1) {
      gpio_set_level(SLED_GPIO, 0);
      vTaskDelay( 1900 / portTICK_RATE_MS );
      gpio_set_level(SLED_GPIO, 1);
      vTaskDelay( 100 / portTICK_RATE_MS );

      if(relay_is_on()){
         gpio_set_level(SLED_GPIO, 0);
         vTaskDelay( 100 / portTICK_RATE_MS );
         gpio_set_level(SLED_GPIO, 1);
         vTaskDelay( 100 / portTICK_RATE_MS );
      }
	}
}

esp_err_t init_fs(void)
{
   esp_vfs_spiffs_conf_t conf = {
      .base_path = CONFIG_WWW_MOUNT_POINT,
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = false
   };
   esp_err_t ret = esp_vfs_spiffs_register(&conf);

   if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
         ESP_LOGE(TAG, "Failed to mount or format filesystem");
      } else if (ret == ESP_ERR_NOT_FOUND) {
         ESP_LOGE(TAG, "Failed to find SPIFFS partition");
      } else {
         ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
      }
      return ESP_FAIL;
   }

   size_t total = 0, used = 0;
   ret = esp_spiffs_info(NULL, &total, &used);
   if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
   } else {
      ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
   }
   return ESP_OK;
}

void app_main(void){

   //init nvs
   esp_err_t ret = nvs_flash_init();
   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
   }
   ESP_ERROR_CHECK(ret);

   //init tcp/ip stack
   ESP_ERROR_CHECK(esp_netif_init());

   //init default event loop
   ESP_ERROR_CHECK(esp_event_loop_create_default());

   //load settings
   settings_t *settings = get_settings();
   ESP_ERROR_CHECK(load_settings(settings));
   if(settings->on_threshold < settings->off_threshold){
      settings->off_threshold = settings->on_threshold - 1;
   }

   //init wifi
   init_wifi();

   //init webserver
   httpd_handle_t server = NULL;
   ESP_ERROR_CHECK(start_webserver(&server));

   //init tasks
   vTaskDelay( 1000 / portTICK_RATE_MS );
	xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 5, NULL );
   xTaskCreate( &Relay_task, "Relay_task", 2048, NULL, 5, NULL );
   xTaskCreate( &Restart_task, "Restart_task", 2048, NULL, 5, NULL );
   xTaskCreate( &Blink_task, "Blink_task", 2048, NULL, 5, NULL );
}
