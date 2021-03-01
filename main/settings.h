
#include "esp_system.h"

typedef struct { 
   char wifi_ssid[32]; 
   char wifi_pass[64];
   uint32_t off_delay;
   uint8_t on_threshold;
   uint8_t off_threshold;
} settings_t;

const char * SETTINGS_EVENT;

int32_t SETTINGS_EVENT_WIFI_CHANGED;

settings_t * get_settings(void);
void print_settings(settings_t *settings);
esp_err_t load_settings(settings_t *settings);
esp_err_t save_settings(settings_t *settings);