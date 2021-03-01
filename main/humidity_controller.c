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

#define HUMIDITY_THRESHOLD 60.0

#define DHT22_GPIO CONFIG_DHT22_GPIO
#define RELAY_GPIO CONFIG_RELAY_GPIO

static const char *TAG = "MAIN";

void DHT_task(void *pvParameter){
	setDHTgpio( DHT22_GPIO );
	printf( "Starting DHT Task\n\n");

	while(1) {
	
		printf("=== Reading DHT ===\n" );
		int ret = readDHT();
		
		errorHandler(ret);

		printf( "Hum %.1f\n", getHumidity() );
		printf( "Tmp %.1f\n", getTemperature() );
		
		// -- wait at least 2 sec before reading again ------------
		// The interval of whole process must be beyond 2 seconds !! 
		vTaskDelay( 3000 / portTICK_RATE_MS );
	}
}

void Relay_task(void *pvParameter){
   gpio_pad_select_gpio( RELAY_GPIO );
    /* Set the GPIO as a push/pull output */
   gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);

   float humidity = 0.0;
   bool relay_high = false;

   while(1) {
   
      humidity = getHumidity();

      if(humidity > HUMIDITY_THRESHOLD+1 && !relay_high){
         printf("Turning on the relay\n");
         gpio_set_level(RELAY_GPIO, 1);
         relay_high = true;
      }
      else if(humidity < HUMIDITY_THRESHOLD-1 && relay_high){
         printf("Turning off the relay\n");
         gpio_set_level(RELAY_GPIO, 0);
         relay_high = false;
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
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

   //init filesystem
   ESP_ERROR_CHECK(init_fs());

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
}
