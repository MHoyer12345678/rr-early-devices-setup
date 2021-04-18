/*
 * jobs.h
 *
 *  Created on: 06.02.2020
 *      Author: joe
 */

#ifndef SRC_JOBS_H_
#define SRC_JOBS_H_

#include <sys/stat.h>
#include <stdbool.h>

typedef struct change_rights_t
{
	const char *name;
	const char *path;
	uid_t owner;
	gid_t group;
	mode_t mode;
} change_rights_t;

typedef struct mount_t
{
	const char *name;
	const char *what;
	const char *fsType;
	const char *where;
	unsigned long flasgs;
	const void *data;
} mount_t;

#define RESULT_E_TIMEOUT				3
#define RESULT_E_ERROR					2
#define RESULT_OK_JOBS_PENDING			1
#define RESULT_OK_NO_MORE_JOB			0

bool job_queue_init(bool debug_mode);

void job_queue_deinit();

void job_queue_do_process();

int job_queue_get_timeout();

void job_queue_add_change_rights_job(const change_rights_t *data);

void job_queue_add_mount_job(const mount_t *data);

int job_queue_get_result();

#endif /* SRC_JOBS_H_ */
