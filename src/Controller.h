/*
 * RetroradioController.h
 *
 *  Created on: 21.10.2017
 *      Author: joe
 */

#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

#include <stdbool.h>

bool controller_init(bool debug_mode);

void controller_run();

void controller_deinit();

int controller_get_exit_result();

void controller_signal_startup_complete();

#endif /* SRC_CONTROLLER_H_ */
