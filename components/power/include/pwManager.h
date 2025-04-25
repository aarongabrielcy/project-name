#ifndef PWMANAGER_H
#define PWMANAGER_H

#include <stdbool.h>

#define POWER_KEY_PIN 41
#define POWER_SIM_PIN 38
#define GNSS_LED_PIN  19
#define POWER_LED_PIN 20
#define SIM_DTR_PIN   42
#define IGNITION_PIN  10
#define TIMER_INTERVAL (30 * 60 * 1000) // 30 minutos en milisegundos
#define OUTPUT_1 13
#define OUTPUT_2 14

void power_init();
void power_on_module();
void power_off_module();
void power_restart();
void power_press_key();
void power_init_gnss_led();
void power_blink_gnss_led(int fixState);
void power_init_ignition();
bool power_get_ignition_state();
void set_gnss_led_state(int state);
void io_manager_init();
void io_monitor_task(void *arg);
void seco_init(void);
void out2_init(void);
void engineCutOn(void);
void engineCutOff(void);
void active_out2(void);
void desactive_out2(void);
int outputControl(int output , int state);

#endif