/*
 * gpio_led.h
 *
 *  Created on: 14.02.2020
 *      Author: joe
 */

#ifndef SRC_GPIO_LED_H_
#define SRC_GPIO_LED_H_

#include <stdbool.h>

bool gpio_led_init(bool debug_mode);

void gpio_led_deinit();

void qpio_led_do_process();

int qpio_led_get_timeout();


#endif /* SRC_GPIO_LED_H_ */
