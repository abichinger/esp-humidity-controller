#include "rest_server.h"
#include "cJSON.h"
#include "settings.h"
#include "DHT22.h"
#include "esp_event.h"
#include "esp_vfs.h"
#include "esp_system.h"
#include "relay.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_http_client.h"

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define BUFSIZE 51200

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static const char *TAG = "REST";
static char * buf;
static char * cl_buf;

static esp_err_t info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "model", chip_info.model);
    cJSON_AddNumberToObject(root, "features", chip_info.features);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    cJSON_AddNumberToObject(root, "revision", chip_info.revision);
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
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "h", getHumidity());
    cJSON_AddNumberToObject(root, "t", getTemperature());
    cJSON_AddBoolToObject(root, "on", relay_is_on());
    cJSON_AddNumberToObject(root, "wifi_mode", mode);
    cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time()/1000);

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
    settings->on_threshold = cJSON_GetObjectItem(root, "on_threshold")->valuedouble;
    settings->off_threshold = cJSON_GetObjectItem(root, "off_threshold")->valuedouble;
    settings->off_delay = cJSON_GetObjectItem(root, "off_delay")->valuedouble;

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

static esp_err_t restart_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "true");

    ESP_LOGI(TAG, "restarting...");
    vTaskDelay( 200 / portTICK_RATE_MS );
    esp_restart();

    return ESP_OK;
}

static const httpd_uri_t restart_uri = {
    .uri       = "/api/v1/restart",
    .method    = HTTP_POST,
    .handler   = restart_handler,
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

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

static esp_err_t set_content_length(httpd_req_t *req, FILE *fp)
{
    long length;

    fseek(fp, 0L, SEEK_END);
    length = ftell(fp);
    rewind(fp);

    sprintf(cl_buf, "%ld", length);
    ESP_LOGI(TAG, "Content-Length: %ld", length);

    return httpd_resp_set_hdr(req, "Content-Length", cl_buf);
    //return ESP_OK;
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t static_handler(httpd_req_t *req)
{   
    char filepath[FILE_PATH_MAX];

    strlcpy(filepath, CONFIG_WWW_MOUNT_POINT, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);
    set_content_length(req, fp);
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    char *chunk = buf;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the buffer */
        read_bytes = fread(chunk, 1, BUFSIZE, fp);
        
        if (read_bytes == 0) {
            break;
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                fclose(fp);
                ESP_LOGE(TAG, "Failed to send file: %s", filepath);
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    
    if(ferror(fp) == 0){
        ESP_LOGI(TAG, "Successfully sent file: %s", filepath);
    }
    else{
        ESP_LOGE(TAG, "Failed to read file: %s", filepath);
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read file");
        return ESP_FAIL;
    }

    /* Close file after sending complete */
    fclose(fp);
    
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t static_uri = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = static_handler,
    .user_ctx  = NULL
};

/*static esp_err_t socket_opened_handler(httpd_handle_t hd, int sockfd){
    ESP_LOGI(TAG, "socket opened %d", sockfd);
    return ESP_OK;
}

static void socket_closed_handler(httpd_handle_t hd, int sockfd){
    ESP_LOGI(TAG, "socket closed %d", sockfd);
}*/

void webserver_check(void *pvParameter){

    int status;
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = "http://127.0.0.1/api/v1/data",
        .timeout_ms = 5000
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

	while(1) {
		err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            status = esp_http_client_get_status_code(client);
            if(status == 200){
                ESP_LOGI(TAG, "webserver check passed");
                vTaskDelay( 30000 / portTICK_RATE_MS );
                continue;
            }

        }

        ESP_LOGE(TAG, "webserver failed");
        esp_restart();
	}
}

esp_err_t start_webserver(httpd_handle_t *server)
{
    esp_err_t err;

    buf = calloc(1, BUFSIZE);
    if(buf == NULL) return ESP_FAIL;

    cl_buf = calloc(1, 50);
    if(cl_buf == NULL) return ESP_FAIL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 5;
    config.send_wait_timeout = 5;
    //config.open_fn = &socket_opened_handler;
    //config.close_fn = &socket_closed_handler;
    config.uri_match_fn = httpd_uri_match_wildcard;

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
    httpd_register_uri_handler(*server, &static_uri);
    httpd_register_uri_handler(*server, &restart_uri);

    xTaskCreate( &webserver_check, "webserver check", 2048, NULL, 5, NULL );

   return ESP_OK;
}

esp_err_t stop_webserver(httpd_handle_t *server)
{
    // Stop the httpd server
    return httpd_stop(server);
}