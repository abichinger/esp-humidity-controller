#include "esp_system.h"
#include "esp_log.h"
#include <esp_http_server.h>

esp_err_t start_webserver(httpd_handle_t *server);
esp_err_t stop_webserver(httpd_handle_t *server);