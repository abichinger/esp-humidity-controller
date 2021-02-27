#include "rest_server.h"
#include "cJSON.h"
#include "settings.h"
#include "DHT22.h"
#include "esp_event.h"

#define BUFSIZE 10240

static const char *TAG = "REST";
static char buffer[BUFSIZE];

static esp_err_t info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static const httpd_uri_t info_uri = {
    .uri       = "/api/v1/info",
    .method    = HTTP_GET,
    .handler   = info_get_handler,
    .user_ctx  = NULL
};

static esp_err_t data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "h", getHumidity());
    cJSON_AddNumberToObject(root, "t", getTemperature());
    cJSON_AddBoolToObject(root, "on", true); // #TODO: replace const value
    cJSON_AddStringToObject(root, "wifi_mode", "ap"); // #TODO: replace const value

    const char *data = cJSON_Print(root);
    httpd_resp_sendstr(req, data);
    free((void *)data);
    cJSON_Delete(root);
    return ESP_OK;
}

static const httpd_uri_t data_uri = {
    .uri       = "/api/v1/data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    .user_ctx  = NULL
};

static esp_err_t settings_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    settings_t *settings = get_settings();

    cJSON_AddStringToObject(root, "wifi_ssid", settings->wifi_ssid);
    cJSON_AddStringToObject(root, "wifi_pass", settings->wifi_pass);
    cJSON_AddNumberToObject(root, "on_threshold", settings->on_threshold);
    cJSON_AddNumberToObject(root, "off_threshold", settings->off_threshold);
    cJSON_AddNumberToObject(root, "off_delay", settings->off_delay);

    const char *settings_json = cJSON_Print(root);
    httpd_resp_sendstr(req, settings_json);
    free((void *)settings_json);
    cJSON_Delete(root);
    return ESP_OK;
}

static const httpd_uri_t settings_get_uri = {
    .uri       = "/api/v1/settings",
    .method    = HTTP_GET,
    .handler   = settings_get_handler,
    .user_ctx  = NULL
};

static esp_err_t settings_post_handler(httpd_req_t *req)
{
    bool wifi_settings_changed = false;
    
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    if (total_len >= BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    char *buf = &buffer[0];
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    ESP_LOGI(TAG, "received settings: '%s'", buf);

    cJSON *root = cJSON_Parse(buf);
    settings_t *settings = get_settings();

    cJSON *wifi_ssid = cJSON_GetObjectItem(root, "wifi_ssid");
    cJSON *wifi_pass = cJSON_GetObjectItem(root, "wifi_pass");

    if(strcmp(wifi_ssid->valuestring, settings->wifi_ssid) != 0 || strcmp(wifi_pass->valuestring, settings->wifi_pass) != 0){
        wifi_settings_changed = true;
    }

    strcpy(settings->wifi_ssid, wifi_ssid->valuestring);
    strcpy(settings->wifi_pass, wifi_pass->valuestring);
    settings->on_threshold = cJSON_GetObjectItem(root, "on_threshold")->valueint;
    settings->off_threshold = cJSON_GetObjectItem(root, "off_threshold")->valueint;
    settings->off_delay = cJSON_GetObjectItem(root, "off_delay")->valueint;

    save_settings(settings);

    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "true");

    if(wifi_settings_changed) esp_event_post(SETTINGS_EVENT, SETTINGS_EVENT_WIFI_CHANGED, NULL, 0, 0);

    return ESP_OK;
}

static const httpd_uri_t settings_post_uri = {
    .uri       = "/api/v1/settings",
    .method    = HTTP_POST,
    .handler   = settings_post_handler,
    .user_ctx  = NULL
};

// static const httpd_uri_t turn_on_uri = {
//     .uri       = "/api/v1/on",
//     .method    = HTTP_POST,
//     .handler   = turn_on_post_handler,
//     .user_ctx  = NULL
// };

// static const httpd_uri_t turn_off_uri = {
//     .uri       = "/api/v1/off",
//     .method    = HTTP_POST,
//     .handler   = turn_off_post_handler,
//     .user_ctx  = NULL
// };

esp_err_t start_webserver(httpd_handle_t *server)
{
   esp_err_t err;
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
   config.lru_purge_enable = true;

   // Start the httpd server
   ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
   err = httpd_start(server, &config);
   if (err != ESP_OK) return err;

   // Set URI handlers
   ESP_LOGI(TAG, "Registering URI handlers");
   httpd_register_uri_handler(*server, &info_uri);
   httpd_register_uri_handler(*server, &data_uri);
   httpd_register_uri_handler(*server, &settings_get_uri);
   httpd_register_uri_handler(*server, &settings_post_uri);
   return ESP_OK;
}

esp_err_t stop_webserver(httpd_handle_t *server)
{
    // Stop the httpd server
    return httpd_stop(server);
}