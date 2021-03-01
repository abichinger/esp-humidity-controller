idf_component_register(SRCS "wifi.c" "settings.c" "rest_server.c" "humidity_controller.c"
                    INCLUDE_DIRS ".")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../webclient")
if(EXISTS ${WEB_SRC_DIR}/dist)
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'yarn build' in ${WEB_SRC_DIR}")
endif()