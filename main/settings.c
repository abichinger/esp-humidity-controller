#include <stdio.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "settings.h"

#define ON_THRESHOLD       CONFIG_ON_THRESHOLD
#define OFF_THRESHOLD      CONFIG_OFF_THRESHOLD
#define OFF_DELAY          CONFIG_OFF_DELAY
#define WIFI_SSID          CONFIG_WIFI_SSID
#define WIFI_PASS          CONFIG_WIFI_PASSWORD

const char *SETTINGS_EVENT = "settings_evt";
int32_t SETTINGS_EVENT_WIFI_CHANGED = 0;

static const char *TAG = "SETTINGS";

static const char *STORAGE_NAMESPACE = "sto";
static const char *SETTINGS_KEY = "settings";

settings_t settings = {
   .wifi_ssid = WIFI_SSID,
   .wifi_pass = WIFI_PASS,
   .on_threshold = ON_THRESHOLD,
   .off_threshold = OFF_THRESHOLD,
   .off_delay = OFF_DELAY,
   .mode = Automatic
};

settings_t * get_settings(void){
    return &settings;
}

esp_err_t load_settings(settings_t *settings){
   nvs_handle_t my_handle;
   esp_err_t err;

   // Open
   err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
   if (err != ESP_OK) return err;

   size_t required_size = sizeof(settings_t);
   err = nvs_get_blob(my_handle, SETTINGS_KEY, settings, &required_size);
   if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

   if(err == ESP_ERR_NVS_NOT_FOUND){
      ESP_LOGI(TAG, "default settings loaded");
   }
   
   ESP_LOGI(TAG, "settings loaded\n");
   print_settings(settings);

   return ESP_OK;
}

esp_err_t save_settings(settings_t *settings){
   nvs_handle_t my_handle;
   esp_err_t err;

   // Open
   err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
   if (err != ESP_OK) return err;

   size_t required_size = sizeof(settings_t);
   err = nvs_set_blob(my_handle, SETTINGS_KEY, settings, required_size);
   if (err != ESP_OK) return err;
   
   ESP_LOGI(TAG, "settings saved %d", required_size);
   print_settings(settings);

   return ESP_OK;
}

void print_settings(settings_t *settings){
   ESP_LOGI(TAG, "ssid:%s", settings->wifi_ssid);
   ESP_LOGI(TAG, "on_threshold:%d", settings->on_threshold);
   ESP_LOGI(TAG, "off_threshold:%d", settings->off_threshold);
   ESP_LOGI(TAG, "off_delay:%d", settings->off_delay);
   ESP_LOGI(TAG, "mode:%d", settings->mode);
}