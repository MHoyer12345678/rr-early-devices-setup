/*
 * handover.c
 *
 *  Created on: 14.02.2020
 *      Author: joe
 */
#include "handover.h"

#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "Controller.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#define HANDOVER_FILE_PATH		"/run/"
#define HANDOVER_FILE_NAME		"system_start_complete"

static int inotify_fd=-1;
static int inotify_watch_id=-1;
static bool ho_debug_mode=false;

bool handover_init(bool debug_mode)
{
	ho_debug_mode=debug_mode;

	if (ho_debug_mode)
		printf("[DEBUG] handover_init - Initializing handover module.\n");

	if (inotify_fd!=-1)
		handover_deinit();

	inotify_fd=inotify_init();
	if (inotify_fd<0)
	{
		printf("Unable to initialize inotify: %s\n", strerror(errno));
		return false;
	}

	inotify_watch_id = inotify_add_watch( inotify_fd, HANDOVER_FILE_PATH, IN_CREATE);
	if (inotify_watch_id==-1)
	{
		printf("Unable add watch for %s: %s\n", HANDOVER_FILE_PATH HANDOVER_FILE_NAME, strerror(errno));
		return false;
	}

	return true;
}

void handover_deinit()
{
	if (ho_debug_mode)
		printf("[DEBUG] handover_deinit - DeInitializing handover module.\n");

	if (inotify_fd!=-1)
	{
		if (inotify_watch_id!=-1)
		{
			inotify_rm_watch(inotify_fd, inotify_watch_id);
			inotify_watch_id=-1;
		}

		close(inotify_fd);
		inotify_fd=-1;
	}
}

int handover_get_event_fd()
{
	return inotify_fd;
}

void handover_process_event()
{
	int length, i;
	char buffer[EVENT_BUF_LEN];

	length = read(inotify_fd, buffer, EVENT_BUF_LEN );
	if ( length < 0 )
	{
		printf("Error reading inotify event: %s\n",strerror(errno));
		return;
	}

	while ( i < length )
	{
		struct inotify_event *event = (struct inotify_event *)&buffer[i];
		if (ho_debug_mode)
			printf("[DEBUG] handover_process_event - Notify event received\n");

		if (event->len )
		{
			if ( event->mask & IN_CREATE )
			{
				if ( !(event->mask & IN_ISDIR))
				{
					if (ho_debug_mode)
						printf("[DEBUG] handover_process_event - File created: %s\n", event->name);
					if (strcmp(event->name,HANDOVER_FILE_NAME)==0)
					{
						if (ho_debug_mode)
							printf("[DEBUG] handover_process_event - A complete startup has been indicated. Signaling controller.\n");
						controller_signal_startup_complete();
					}
				}
			}
		}
		i += EVENT_SIZE + event->len;
	}
}

void handover_notify_early_setup_shut_down()
{
	//tell controller application that we are done shutting down. After this event,
	//the controller app can control securely the gpio of the LED
	unlink(HANDOVER_FILE_PATH HANDOVER_FILE_NAME);
}
