
void relay_init(void);
void relay_turn_on(void);
void relay_turn_off(void);
bool relay_is_on(void);
void relay_schedule_turn_off(uint32_t seconds);
bool relay_turn_off_timer_active(void);