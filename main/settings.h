
#include "esp_system.h"

typedef struct { 
   uint8_t wifi_ssid[32]; 
   uint8_t wifi_pass[64];
   uint32_t off_delay;
   uint8_t on_threshold;
   uint8_t off_threshold;
} settings_t;

settings_t * get_settings(void);
esp_err_t load_settings(settings_t *settings);
esp_err_t save_settings(settings_t *settings);