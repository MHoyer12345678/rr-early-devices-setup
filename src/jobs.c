/*
 * jobs.c
 *
 *  Created on: 06.02.2020
 *      Author: joe
 */
#include "jobs.h"

#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define MAX_JOBS		500

#define JOB_RESULT_OK		0
#define JOB_RESULT_E_AGAIN	1
#define JOB_RESULT_E_ERROR	2

//try again pending jobs after this number of milliseconds
#define JOB_WAIT_PENDING_MS 50

//overall timeout after 1s (1000ms / 20 retries = 50ms retry timeout)<s
#define JOB_MAX_RETRIES			20

//returned by job_queue_get_result


typedef int (*job_function_t)(const void *data);

typedef struct job_t
{
	const char *name;
	job_function_t job_function;
	const void *data;
	bool job_done;
} job_t;

static job_t jobs[MAX_JOBS];

static int cnt_jobs=0;

static int cnt_retries;

static int result;

static bool jq_debug_mode;

static int job_queue_exectute_change_rights_job(const void *data);

static int job_queue_exectute_mount_job(const void *data);

bool job_queue_init(bool debug_mode)
{
	cnt_jobs=0;
	cnt_retries=0;
	result=RESULT_OK_NO_MORE_JOB;
	jq_debug_mode=debug_mode;

	if (jq_debug_mode)
		printf("[DEBUG] job_queue_init - Initialized job queue.\n");

	return true;
}

void job_queue_deinit()
{
	if (jq_debug_mode)
		printf("[DEBUG] job_queue_deinit - DeInitialized job queue.\n");
}

void job_queue_add_change_rights_job(const change_rights_t *data)
{
	assert(cnt_jobs<MAX_JOBS);
	jobs[cnt_jobs].name=data->name;
	jobs[cnt_jobs].data=data;
	jobs[cnt_jobs].job_function=job_queue_exectute_change_rights_job;
	jobs[cnt_jobs].job_done=false;
	cnt_jobs++;

	if (jq_debug_mode)
		printf("[DEBUG] job_queue_add_change_rights_job - Adding access rights job for path %s.: %d:%d, %o\n",
				data->path, data->owner, data->group, data->mode);

	result=RESULT_OK_JOBS_PENDING;
}

void job_queue_add_mount_job(const mount_t *data)
{
	assert(cnt_jobs<MAX_JOBS);
	jobs[cnt_jobs].name=data->name;
	jobs[cnt_jobs].data=data;
	jobs[cnt_jobs].job_function=job_queue_exectute_mount_job;
	jobs[cnt_jobs].job_done=false;
	cnt_jobs++;

	if (jq_debug_mode)
		printf("[DEBUG] job_queue_add_mount_job - Adding mount job for path %s: %s,%s,%lu\n",
				data->where, data->what, data->fsType, data->flasgs);

	result=RESULT_OK_JOBS_PENDING;
}

void job_queue_do_process()
{
	int a,r;
	bool jobs_pending=false;

	cnt_retries++;

	if (cnt_retries>=JOB_MAX_RETRIES)
	{
		result=RESULT_E_TIMEOUT;
		printf("Jobs still pending after overall timeout elapsed. Jobs still pending: \n");
		for (a=0;a<cnt_jobs;a++)
		{
			if (!jobs[a].job_done)
				printf("\t%s\n", jobs[a].name);
		}

		return;
	}

	if (jq_debug_mode)
		printf("[DEBUG] job_queue_do_process - Processing job queue\n");

	for (a=0;a<cnt_jobs && result!=RESULT_E_ERROR;a++)
	{
		if (jobs[a].job_done) continue;

		if (jq_debug_mode)
			printf("[DEBUG] job_queue_do_process - Trying to execute job: %s\n", jobs[a].name);

		r=jobs[a].job_function(jobs[a].data);
		if (r==JOB_RESULT_E_AGAIN)
			jobs_pending=true;
		else if (r==JOB_RESULT_OK)
			jobs[a].job_done=true;
		else if (r==JOB_RESULT_E_ERROR)
			result=RESULT_E_ERROR;

		if (jq_debug_mode)
		{
			if (r==JOB_RESULT_E_AGAIN)
				printf("[DEBUG] job_queue_do_process - Job %s not finished. Trying again later.\n", jobs[a].name);
			else if (r==JOB_RESULT_OK)
				printf("[DEBUG] job_queue_do_process - Finished job %s successfully\n", jobs[a].name);
		}

		if (r==JOB_RESULT_E_ERROR)
				printf("Unrecoverable error while executing job %s .\n", jobs[a].name);
	}

	if (!jobs_pending && result == RESULT_OK_JOBS_PENDING)
	{
		if (jq_debug_mode)
			printf("[DEBUG] job_queue_do_process - All jobs finished.\n");
		result=RESULT_OK_NO_MORE_JOB;
	}
	else
	{
		if (jq_debug_mode)
			printf("[DEBUG] job_queue_do_process - Jobs pending.\n");
	}
}

int job_queue_get_result()
{
	return result;
}

int job_queue_get_timeout()
{
	return JOB_WAIT_PENDING_MS;
}

static int job_queue_exectute_change_rights_job(const void *data)
{
	const change_rights_t *access_rights_data=(change_rights_t *)data;
	if (chown(access_rights_data->path, access_rights_data->owner,
			access_rights_data->group)!=0)
	{
		if (errno!=ENOENT)
		{
			printf("Unable to chown %s to %d:%d\n",access_rights_data->path,
				access_rights_data->owner, access_rights_data->group);
			return JOB_RESULT_E_ERROR;
		}
		else
			return JOB_RESULT_E_AGAIN;
	}

	if (chmod(access_rights_data->path, access_rights_data->mode)!=0)
	{
		printf("Unable to chmod of %s to %X\n",access_rights_data->path,
				access_rights_data->mode);

		return JOB_RESULT_E_AGAIN;
	}

	return JOB_RESULT_OK;
}

static int job_queue_exectute_mount_job(const void *data)
{
	const mount_t *mount_data=(mount_t *)data;

	if (mount(mount_data->what, mount_data->where,mount_data->fsType,
			mount_data->flasgs,mount_data->data)!=0)
	{
		if (errno!=ENOENT)
		{
			printf("Unable to mount %s to %s with fstype %s. Error: %s\n",mount_data->what,
					mount_data->where,mount_data->fsType, strerror(errno));
			return JOB_RESULT_E_ERROR;
		}
		else
			return JOB_RESULT_E_AGAIN;
	}

	return JOB_RESULT_OK;
}
