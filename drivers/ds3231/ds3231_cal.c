/**
 * @file ds3231_cal.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @brief 
 * @date 2021-06-18
 * 
 * @copyright Copyright (C) 2020 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include <device.h>
#include <zcal/calendar.h>
#include <logging/log.h>
#include <drivers/rtc/maxim_ds3231.h>
#include <sys/timeutil.h>

#define DT_DRV_COMPAT calendar

#if DT_HAS_CHOSEN(zcal_rtc)
#define CALENDAR_DEV_LABEL DT_LABEL(DT_CHOSEN(zcal_rtc))
#else
#define CALENDAR_DEV_LABEL DT_PROP_BY_PHANDLE(DT_PATH(calendar), rtc, label)
#endif

LOG_MODULE_REGISTER(calendar, CONFIG_CALENDAR_LOG_LEVEL);

struct ds3231_config{
	const struct device * rtc_dev;
};

/**
 * @brief Set the calendar time to the battery backed rtc domain. 
 * 
 * This will attempt to maintain subsecond accuracy when updating the time.
 * As a result, it may take up to a second to complete.
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @param tm Pointer to the time structure describing the current calendar date
 * @retval 0 on success
 * @retval -errno on failure
 */
static int ds3231_calendar_settime(const struct device * dev, struct tm * tm) {
	int rc = 0;
	const struct ds3231_config * cfg = dev->config;
	const struct device * rtc = cfg->rtc_dev;
	if (!rtc){
		return -ENODEV;
	}
	uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(rtc);
	uint32_t syncclock = maxim_ds3231_read_syncclock(rtc);
	time_t tv_sec = timeutil_timegm(tm);
	struct maxim_ds3231_syncpoint sp = {
		.rtc = {
			.tv_sec = tv_sec,
			.tv_nsec = (uint64_t)NSEC_PER_SEC * syncclock / syncclock_Hz,
		},
		.syncclock = syncclock,
	};

	struct k_poll_signal ss;
	struct sys_notify notify;
	struct k_poll_event sevt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
							    K_POLL_MODE_NOTIFY_ONLY,
							    &ss);

	k_poll_signal_init(&ss);
	sys_notify_init_signal(&notify, &ss);


	rc = maxim_ds3231_set(rtc, &sp, &notify);

	/* Wait for the set to complete */
	rc = k_poll(&sevt, 1, K_MSEC(1000));

	rc = maxim_ds3231_get_syncpoint(rtc, &sp);
	LOG_DBG("wrote sync %d: %u %u at %u", rc,
	       (uint32_t)sp.rtc.tv_sec, (uint32_t)sp.rtc.tv_nsec,
	       sp.syncclock);

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
static int ds3231_calendar_gettime(const struct device * dev, struct tm * tm) {
	const struct ds3231_config * cfg = dev->config;
	const struct device * rtc = cfg->rtc_dev;
	if (!rtc){
		return -ENODEV;
	}
	uint32_t now = 0;
	(void)counter_get_value(rtc, &now);
	struct tm tv;
	time_t tnow = now;
	LOG_DBG("time now %u", now);
	memcpy(tm, gmtime_r(&tnow, &tv), sizeof(struct tm));
	return 0;
}

/**
 * @brief Initialize calendar API. Gets the underlying rtc device
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0
 */
static int ds3231_rtc_initilize(const struct device *dev) {
	const struct device * rtc = device_get_binding(CALENDAR_DEV_LABEL);
	((struct ds3231_config *)dev->config)->rtc_dev = rtc;
	if (rtc){
		return 0;
	}
	return -ENODEV;
}

static const struct calendar_driver_api ds3231_calendar_api = {
	.settime = ds3231_calendar_settime,
	.gettime = ds3231_calendar_gettime,
};

struct ds3231_config ds3231_config = {NULL};

DEVICE_DT_INST_DEFINE(0, ds3231_rtc_initilize, NULL,
	NULL, &ds3231_config,
	POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
	&ds3231_calendar_api
); 
