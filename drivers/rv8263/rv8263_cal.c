/**
 * @file rv8263_cal.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @brief 
 * @date 2022-11-22
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/i2c.h>
#include <zcal/calendar.h>
#include <logging/log.h>

#define DT_DRV_COMPAT microcrystal_rv8263_calendar

LOG_MODULE_REGISTER(calendar, CONFIG_CALENDAR_LOG_LEVEL);

#define RV8263_BIAS_YEAR 2000
#define TM_BIAS_YEAR	1900

typedef struct  __attribute__ ((packed)) {
	uint8_t seconds;    	// 0x04
	uint8_t minutes;		// 0x05
	uint8_t hours;			// 0x06
	uint8_t date;			// 0x07
	uint8_t weekday;		// 0x08
	uint8_t month;			// 0x09
	uint8_t year;			// 0x0A
} rv8263_time_t;

typedef struct  __attribute__ ((packed)) {
	uint8_t	control1;		// 0x00
	uint8_t	control2;		// 0x01
	uint8_t	offset;			// 0x02
	uint8_t ram;			// 0x03
	rv8263_time_t calendar; //0x04 - 0x0A	
	uint8_t seconds_alarm;	// 0x0B
	uint8_t minutes_alarm;	// 0x0C
	uint8_t hours_alarm;	// 0x0D
	uint8_t date_alarm;		// 0x0E
	uint8_t weekday_alarm;	// 0x0F
	uint8_t timer_value;	// 0x10
	uint8_t timer_mode;		// 0x11
} rv8263_regmap_t;

struct rv8263_data{
	rv8263_regmap_t registers;
};

struct rv8263_config{
	const struct device * bus;
	uint8_t addr;
};

static int rv8263_convert_to_time(struct tm * dst, const rv8263_time_t * src){
	if (dst == NULL || src == NULL){
		return -ENODEV;
	}
	/* tm_sec can technically be 60 or 61 to account for leap seconds on some systems, rv8263 wont consider this*/
	dst->tm_sec = bcd2bin(src->seconds);
	dst->tm_min = src->minutes;
	dst->tm_hour = bcd2bin(src->hours);
	dst->tm_mday = bcd2bin(src->date);
	dst->tm_wday = bcd2bin(src->weekday);
	/* tm uses months indexed 0-11, rv8263 uses months indexed 1-12 */
	dst->tm_mon = bcd2bin(src->month) - 1;
	/* tm biases months to 1900, rv8263 biases months to 2000 */
	dst->tm_year = bcd2bin(src->year) + RV8263_BIAS_YEAR - TM_BIAS_YEAR;
	
	/* DST is not handled rv8263. -1 indicates unknown support so it should be ignored*/
	dst->tm_isdst = -1;

	return 0;
}

static int rv8263_filter_time(rv8263_time_t * time){
	/**
	 * @note TODO: change register map to bitfield to avoid needing this.
	 * 
	 */
	if (time == NULL){
		return -ENODEV;
	}
	time->seconds &= 0x7f;
	time->minutes &= 0x7f;
	time->hours &= 0x7f;
	time->date &= 0x3f;
	time->weekday &= 0x03;
	time->month &= 0x1f;
	time->year &= 0xff;
	return 0;
}

static int rv8263_convert_from_time(rv8263_time_t * dst, const struct tm * src){
	if (dst == NULL || src == NULL){
		return -ENODEV;
	}
	/* tm_sec can technically be 60 or 61 to account for leap seconds on some systems. Clamp it to 59 for rv8263*/
	dst->seconds = bin2bcd(MIN(src->tm_sec, 59)); 
	dst->minutes = bin2bcd(src->tm_min);
	dst->hours = bin2bcd(src->tm_hour);
	dst->date = bin2bcd(src->tm_mday);
	dst->weekday = bin2bcd(src->tm_wday);
	/* tm uses months indexed 0-11, rv8263 uses months indexed 1-12 */
	dst->month = bin2bcd(src->tm_mon + 1);
	/* tm uses months to year 1900, rv8263 biases months to 2000 */
	dst->year = bin2bcd(MAX(0, src->tm_year + TM_BIAS_YEAR - RV8263_BIAS_YEAR));
	return 0;
}

int rv8263_read(const struct device *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
	const struct rv8263_config *cfg = dev->config;
	return i2c_burst_read(cfg->bus, cfg->addr, reg, data, len);
}

int rv8263_write(const struct device *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
	const struct rv8263_config *cfg = dev->config;
	return i2c_burst_write(cfg->bus, cfg->addr, reg, data, len);
}

/**
 * @brief Set the calendar time to the battery backed rtc domain. 
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @param tm Pointer to the time structure describing the current calendar date
 * @retval 0 on success
 * @retval -errno on failure
 */
static int rv8263_calendar_settime(const struct device * dev, struct tm * tm) {
	rv8263_time_t time;
	int rc = rv8263_convert_from_time(&time, tm);
	if (rc == 0){
		rv8263_write(dev, offsetof(rv8263_regmap_t, calendar), (uint8_t *)&time, sizeof(rv8263_time_t));
	}
	return rc;
}

/**
 * @brief Function for getting the current calendar time as recorded by the
 * battery backed rtc domain
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @param tm Pointer to the time structure which will be populated with the
 * current calendar date
 * @retval 0
 */
static int rv8263_calendar_gettime(const struct device * dev, struct tm * tm) {
	rv8263_time_t time = {0};
	int rc = rv8263_read(dev, offsetof(rv8263_regmap_t, calendar), (uint8_t *)&time, sizeof(rv8263_time_t));
	rv8263_filter_time(&time);
	if (rc == 0){
		rc = rv8263_convert_to_time(tm, &time);
	}
	return rc;
}

/**
 * @brief Initialize calendar API.
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0
 */
static int rv8263_rtc_initilize(const struct device *dev) {
	/**
	 * @note TODO: Is there a need to do anything here? 
	 * Set defaults? Soft reset? Anything in RAM?
	 * 
	 */
	return 0;
}

static const struct calendar_driver_api rv8263_calendar_api = {
	.settime = rv8263_calendar_settime,
	.gettime = rv8263_calendar_gettime,
};

struct rv8263_config rv8263_config = {
	.bus = DEVICE_DT_GET(DT_INST_BUS(0)),
	.addr = DT_INST_REG_ADDR(0),
};

struct rv8263_data rv8263_data = {{0}};

DEVICE_DT_INST_DEFINE(0, rv8263_rtc_initilize, NULL,
	&rv8263_data, &rv8263_config,
	POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
	&rv8263_calendar_api
); 
