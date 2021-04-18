/*
 * RetroradioClient.cpp
 *
 *  Created on: 21.10.2017
 *      Author: joe
 */

#include "Controller.h"

#include <systemd/sd-daemon.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>

#include "jobs.h"
#include "gpio_led.h"
#include "handover.h"

static int result=0;
static bool startup_complete=false;
static bool co_debug_mode=false;

//TODO: Set correct gid of group audio
static const change_rights_t snd1dev={"Change access rights of /dev/snd/controlC0", "/dev/snd/controlC0", 0, 29, S_IRGRP |	S_IWGRP | S_IROTH | S_IWOTH};
static const change_rights_t snd2dev={"Change access rights of /dev/snd/pcmC0D0c", "/dev/snd/pcmC0D0c", 0, 29, S_IRGRP |	S_IWGRP | S_IROTH | S_IWOTH};
static const change_rights_t snd3dev={"Change access rights of /dev/snd/pcmC0D0p", "/dev/snd/pcmC0D0p", 0, 29, S_IRGRP |	S_IWGRP | S_IROTH | S_IWOTH};

static const mount_t tmp_mnt={"Mount /tmp", "tmpfs", "tmpfs", "/tmp", 0, NULL};

static void controller_setup_job_queue();

typedef void (*timeout_process_func_t)();

typedef int (*timeout_get_time_func_t)();

typedef struct mainloop_timeout_control_t
{
	int time_remaining_ms;
	timeout_process_func_t timeout_process_func;
	timeout_get_time_func_t timeout_get_time_func;

} mainloop_timeout_control_t;

bool controller_init(bool debug_mode)
{
	co_debug_mode=debug_mode;

	if (co_debug_mode)
		printf("[DEBUG] controller_init - Initializing controller.\n");

	if (!handover_init(debug_mode)) return false;
	if (!job_queue_init(debug_mode)) return false;
	if (!gpio_led_init(debug_mode)) return false;

	controller_setup_job_queue();

	return true;
}

static void controller_setup_job_queue()
{
	if (co_debug_mode)
		printf("[DEBUG] controller_setup_job_queue - Filling job queue\n");

	job_queue_add_change_rights_job(&snd1dev);
	job_queue_add_change_rights_job(&snd2dev);
	job_queue_add_change_rights_job(&snd3dev);

	job_queue_add_mount_job(&tmp_mnt);

	//TODO: Setup jobs
}

void controller_run()
{
	struct timespec ts1, ts2;
	struct pollfd handover_events;
	int poll_result;

	handover_events.fd=handover_get_event_fd();
	handover_events.events=POLLIN;

	result=RESULT_OK_JOBS_PENDING;

	mainloop_timeout_control_t jobs_timeout_ctrl={0, job_queue_do_process, job_queue_get_timeout};
	mainloop_timeout_control_t gpio_led_ctrl={0, qpio_led_do_process, qpio_led_get_timeout};

	//kick of processing jobs without delay
	job_queue_do_process();
	result=job_queue_get_result();
	if (result==RESULT_OK_NO_MORE_JOB)
		sd_notify(0,"READY=1");

	jobs_timeout_ctrl.time_remaining_ms=jobs_timeout_ctrl.timeout_get_time_func();
	gpio_led_ctrl.time_remaining_ms=gpio_led_ctrl.timeout_get_time_func();

	clock_gettime(CLOCK_REALTIME, &ts1);

	if (co_debug_mode)
		printf("[DEBUG] controller_run - About to enter main loop.\n");

	while ((!startup_complete || result!=RESULT_OK_NO_MORE_JOB) &&
			(result==RESULT_OK_JOBS_PENDING || result==RESULT_OK_NO_MORE_JOB))
	{
		int timeout_ms, time_passed_ms;

		clock_gettime(CLOCK_REALTIME, &ts2);
		time_passed_ms=(ts2.tv_sec-ts1.tv_sec)*1000;
		time_passed_ms+=(ts2.tv_nsec-ts1.tv_nsec)/1000000;

		memcpy(&ts1,&ts2,sizeof(struct timespec));

		if (co_debug_mode)
			printf("[DEBUG] controller_run - time passed in mailoop sine last round: %d ms.\n", time_passed_ms);

		// ------------ gpio leds --------------------------------------------------------
		gpio_led_ctrl.time_remaining_ms-=time_passed_ms;
		if (gpio_led_ctrl.time_remaining_ms<=0)
		{
			gpio_led_ctrl.timeout_process_func();
			gpio_led_ctrl.time_remaining_ms=gpio_led_ctrl.timeout_get_time_func();
		}
		timeout_ms=gpio_led_ctrl.time_remaining_ms;

		// ------------ jobs -------------------------------------------------------------
		if (result==RESULT_OK_JOBS_PENDING)
		{
			jobs_timeout_ctrl.time_remaining_ms-=time_passed_ms;
			if (jobs_timeout_ctrl.time_remaining_ms<=0)
			{
				jobs_timeout_ctrl.timeout_process_func();
				jobs_timeout_ctrl.time_remaining_ms=jobs_timeout_ctrl.timeout_get_time_func();
				result=job_queue_get_result();
				if (result==RESULT_E_ERROR || result==RESULT_E_TIMEOUT)
					break;
				if (result==RESULT_OK_NO_MORE_JOB)
					sd_notify(0,"READY=1");
			}
			if (jobs_timeout_ctrl.time_remaining_ms<timeout_ms)
				timeout_ms=jobs_timeout_ctrl.time_remaining_ms;
		}

		if (co_debug_mode)
			printf("[DEBUG] controller_run - going to sleep for %d ms.\n", timeout_ms);

		//wait for the time calculated or events of handover module
		poll_result=poll(&handover_events, 1, timeout_ms);
		if (poll_result<0 && errno == EINTR)
		{
			printf("Poll error: %s\n", strerror(errno));
			break;
		}
		else if (poll_result > 0)
			handover_process_event();
	}

	if (co_debug_mode)
		printf("[DEBUG] controller_run - Left main loop.\n");
}

void controller_deinit()
{
	if (co_debug_mode)
		printf("[DEBUG] controller_run - Deinitializing controller.\n");
	gpio_led_deinit();
	job_queue_deinit();
	handover_deinit();

	handover_notify_early_setup_shut_down();
}

int controller_get_exit_result()
{
	return result;
}

void controller_signal_startup_complete()
{
	startup_complete=true;
}
