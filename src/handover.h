/*
 * handover.h
 *
 *  Created on: 14.02.2020
 *      Author: joe
 */

#ifndef SRC_HANDOVER_H_
#define SRC_HANDOVER_H_

#include <stdbool.h>

bool handover_init(bool debug_mode);

void handover_deinit();

int handover_get_event_fd();

void handover_process_event();

void handover_notify_early_setup_shut_down();

#endif /* SRC_HANDOVER_H_ */
