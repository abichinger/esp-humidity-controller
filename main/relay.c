#include "sdkconfig.h"
#include "driver/gpio.h"

#define RELAY_GPIO CONFIG_RELAY_GPIO

static bool relay_on = false;

void relay_init(void){
    gpio_pad_select_gpio( RELAY_GPIO );
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
}

void relay_turn_on(void){
    gpio_set_level(RELAY_GPIO, 1);
    relay_on = true;
}

void relay_turn_off(void){
    gpio_set_level(RELAY_GPIO, 0);
    relay_on = false;
}

bool relay_is_on(void){
    return relay_on;
}



