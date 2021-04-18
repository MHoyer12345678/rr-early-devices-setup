/*
 * gpio_led.c
 *
 *  Created on: 14.02.2020
 *      Author: joe
 */
#include "gpio_led.h"

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define LED_GPIO		8

#define GPIO_BASE_PATH "/sys/class/gpio"
#define GPIO_EXPORT_FILE "/export"

#define GPIO_WAIT_EXPORT_POLL_TIME_US 			1000
#define GPIO_WAIT_EXPORT_TIMEOUT_US 			500000

static bool gl_debug_mode=false;

static int blink_time_sequence[]={25,200,25,1000};
static int sequence_cntr=0;
static bool led_enabled=true;

static void gpio_led_set_enabled(int gpio, bool enabled);
static bool gpio_led_export_gpio(int gpio, int timeout_us);
static bool gpio_led_wait_for_export(int gpio, int time_out_us);
static bool gpio_led_set_port_mode_output(int gpio, bool value);
static bool gpio_led_write_value(const char *path, const char* value, int val_len);

bool gpio_led_init(bool debug_mode)
{
	gl_debug_mode=debug_mode;

	if (gl_debug_mode)
		printf("[DBEUG] gpio_led_init - initializing GPIO led.\n");

	if (!gpio_led_export_gpio(LED_GPIO, GPIO_WAIT_EXPORT_TIMEOUT_US))
		return false;

	if (!gpio_led_set_port_mode_output(LED_GPIO, led_enabled))
		return false;

	return true;
}

void gpio_led_deinit()
{
	if (gl_debug_mode)
		printf("[DBEUG] gpio_led_deinit - deinitializing GPIO led.\n");
	gpio_led_set_enabled(LED_GPIO, false);
}

void qpio_led_do_process()
{
	led_enabled=!led_enabled;
	gpio_led_set_enabled(LED_GPIO, led_enabled);
	if (sequence_cntr<3)
		sequence_cntr++;
	else
		sequence_cntr=0;
}

int qpio_led_get_timeout()
{
	return blink_time_sequence[sequence_cntr];
}

static void gpio_led_set_enabled(int gpio, bool enabled)
{
	int gpioStrLen;
	char gpioStr[2], pathStr[2048];

	if (gl_debug_mode)
		printf("[DEBUG] gpio_led_set_enabled - %s gpio %d.\n", enabled ? "Enabling":"Disabling", gpio);

	snprintf(pathStr, 2048, "%s/gpio%d/value",GPIO_BASE_PATH, gpio);
	gpioStrLen=snprintf(gpioStr, 2, "%d",enabled);

	gpio_led_write_value(pathStr,gpioStr,gpioStrLen);
}

static bool gpio_led_write_value(const char *path, const char* value, int val_len)
{
	bool result=true;
	int fh;

	fh=open(path,O_WRONLY);
	if (fh==-1)
	{
		printf("Unable to open path: %s\n",path);
		return false;
	}

	if (write(fh, value, val_len)!= val_len)
	{
		printf("Unable to write value to file.\n");
		result=false;
	}

	close(fh);

	return result;
}

static bool gpio_led_export_gpio(int gpio, int timeout_us)
{
	bool result;

	int gpioStrLen;
	char gpioStr[16];

	if (gl_debug_mode)
		printf("[DEBUG] gpio_led_export_gpio - exporting GPIO %d.\n",gpio);

	//check if already there
	if (gpio_led_wait_for_export(gpio, timeout_us))
		return true;

	gpioStrLen=snprintf(gpioStr, 16, "%d",gpio);
	result=gpio_led_write_value(GPIO_BASE_PATH GPIO_EXPORT_FILE,gpioStr,gpioStrLen);

	if (result)
		result = gpio_led_wait_for_export(gpio, timeout_us);

	if (result)
	{
		if (gl_debug_mode)
			printf("[DEBUG] gpio_led_export_gpio -> Exported gpio %d.\n",gpio);
	}
	else
		printf("Failed to export gpio %d.\n",gpio);

	return result;
}

static bool gpio_led_wait_for_export(int gpio, int time_out_us)
{
	struct stat statResult;
	char pathStr[2048];
	snprintf(pathStr, 2048, "%s/gpio%d/direction",GPIO_BASE_PATH, gpio);

	while (time_out_us > 0)
	{
		if (stat(pathStr, &statResult)==0)
			return true;

		usleep(GPIO_WAIT_EXPORT_POLL_TIME_US);
		time_out_us -= GPIO_WAIT_EXPORT_POLL_TIME_US;
	}

	return false;
}

static bool gpio_led_set_port_mode_output(int gpio, bool value)
{
	bool result;

	char pathStr[2048];

	if (gl_debug_mode)
		printf("[DEBUG] gpio_led_set_port_mode_output -> Setting gpio %d to output mode.\n",gpio);

	snprintf(pathStr, 2048, "%s/gpio%d/direction",GPIO_BASE_PATH, gpio);

	result=gpio_led_write_value(pathStr,"out",3);
	if (result)
		gpio_led_set_enabled(gpio, value);
	else
		printf("Failed to set GPIO %d into output mode.\n",gpio);

	return result;
}
