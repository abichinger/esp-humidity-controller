#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "relay.h"

#define RELAY_GPIO CONFIG_RELAY_GPIO

static bool relay_on = false;
static TimerHandle_t turn_off_timer = NULL;

static void turn_off_timer_callback(TimerHandle_t timer){
    relay_turn_off();
}

void relay_schedule_turn_off(uint32_t seconds){
    xTimerStop(turn_off_timer, 0);
    xTimerChangePeriod(turn_off_timer, (seconds*1000)/portTICK_RATE_MS, 0);
    xTimerStart(turn_off_timer, 0);
}

bool relay_turn_off_timer_active(void){
    return xTimerIsTimerActive(turn_off_timer) != pdFALSE;
}

void relay_init(void){
    gpio_pad_select_gpio( RELAY_GPIO );
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);

    turn_off_timer = xTimerCreate("TURN_OFF_TIMER", 1, pdFALSE, 0, turn_off_timer_callback);
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



