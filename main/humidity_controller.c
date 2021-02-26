/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h"

#include "DHT22.h"

#define HUMIDITY_THRESHOLD 60.0

void DHT_task(void *pvParameter){
	setDHTgpio( GPIO_NUM_23 );
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
   gpio_pad_select_gpio( GPIO_NUM_22 );
    /* Set the GPIO as a push/pull output */
   gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);

   float humidity = 0.0;
   bool relay_high = false;

   while(1) {
   
      humidity = getHumidity();

      if(humidity > HUMIDITY_THRESHOLD+1 && !relay_high){
         printf("Turning on the relay\n");
         gpio_set_level(GPIO_NUM_22, 1);
         relay_high = true;
      }
      else if(humidity < HUMIDITY_THRESHOLD-1 && relay_high){
         printf("Turning off the relay\n");
         gpio_set_level(GPIO_NUM_22, 0);
         relay_high = false;
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
   }
}

void app_main(void){
   nvs_flash_init();
	vTaskDelay( 1000 / portTICK_RATE_MS );
	xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 5, NULL );
   xTaskCreate( &Relay_task, "Relay_task", 2048, NULL, 5, NULL );
}
