#include <string.h>
#include "wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "freertos/event_groups.h"

#include "settings.h"

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define MAXIMUM_RETRY 3

static const char *TAG = "WIFI";

static esp_netif_t *esp_netif_ap;
static esp_netif_t *esp_netif_sta;

static EventGroupHandle_t s_wifi_event_group;

static uint16_t s_retry_num = 0;

esp_event_handler_instance_t wifi_event_handler_instance;
esp_event_handler_instance_t wifi_event_handler_ip_instance;
esp_event_handler_instance_t wifi_settings_handler_instance;

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static void wifi_settings_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    ESP_LOGI(TAG, "wifi_settings_handler");
    reinit_wifi();
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    
    //ap events
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }

    //sta events
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            reinit_wifi();
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG,"connect to the AP success");
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_ap(void){

   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
}


void wifi_init_sta(void){

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    settings_t *settings = get_settings();

    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            }
        }
    };

    strcpy((char*) wifi_config.sta.ssid, settings->wifi_ssid);
    strcpy((char*) wifi_config.sta.password, settings->wifi_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    // /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    //  * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
    //         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    //         pdFALSE,
    //         pdFALSE,
    //         portMAX_DELAY);

    // /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    //  * happened. */
    // if (bits & WIFI_CONNECTED_BIT) {
    //     ESP_LOGI(TAG, "connected to ap SSID:%s", settings->wifi_ssid);
    // } else if (bits & WIFI_FAIL_BIT) {
    //     ESP_LOGI(TAG, "Failed to connect to SSID:%s", settings->wifi_ssid);
    //     deinit_wifi();
    //     wifi_init_ap();
    // } else {
    //     ESP_LOGE(TAG, "UNEXPECTED EVENT");
    // }
}

void reinit_wifi(){
    wifi_mode_t mode;
    esp_err_t err;
    EventBits_t bits;
    settings_t *settings;

    settings = get_settings();
    
    err = esp_wifi_get_mode(&mode);
    if(err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) ESP_ERROR_CHECK(err);

    if(err == ESP_OK){
        //ESP_ERROR_CHECK(esp_wifi_disconnect());
        //esp_netif_destroy(esp_netif);
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
    }

    bits = xEventGroupGetBits(s_wifi_event_group);

    if(strlen(settings->wifi_ssid) > 0 && (bits & WIFI_FAIL_BIT) == 0){
        wifi_init_sta();
    }
    else{
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
        wifi_init_ap();
    }
}

void init_wifi(void){
    esp_netif_ap = esp_netif_create_default_wifi_ap();
    esp_netif_sta = esp_netif_create_default_wifi_sta();

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_event_handler_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &wifi_event_handler_ip_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(SETTINGS_EVENT, SETTINGS_EVENT_WIFI_CHANGED, &wifi_settings_handler, NULL, &wifi_settings_handler_instance));

    reinit_wifi();
}

void deinit_wifi(void){
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler_ip_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(SETTINGS_EVENT, SETTINGS_EVENT_WIFI_CHANGED, wifi_settings_handler_instance));
    //ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    //esp_netif_destroy(esp_netif);
    vEventGroupDelete(s_wifi_event_group);
}
