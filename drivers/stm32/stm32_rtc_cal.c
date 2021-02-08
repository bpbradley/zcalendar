/**
 * @file stm32_rtc_cal.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @brief Implementation of calendar api for stm32 rtc
 * @date 2020-10-07
 * 
 * @copyright Copyright (C) 2020 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_rtc

#include <zephyr.h>
#include <device.h>
#include <stm32f4xx_ll_rtc.h>
#include <stm32f4xx_ll_pwr.h>
#include <stm32f4xx_ll_rcc.h>
#include <zcal/calendar.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stm32rtc, CONFIG_CALENDAR_LOG_LEVEL);

// prescaler values for LSE @ 32768 Hz
#define RTC_PREDIV_ASYNC 0x7F
#define RTC_PREDIV_SYNC 0x00FF

/**
 * 0x32F2 is the magic number that the stm32 ll libraries use to inidicate
 * that the battery backed rtc / sram is already initialized 
 */
#define BAK_SRAM_MAGIC 0x32F2

/**
 * @brief Set the calendar time to the battery backed rtc domain
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @param tm Pointer to the time structure describing the current calendar date
 * @retval 0 on success
 * @retval -ECANCELLED on failure
 */
static int stm32_calendar_settime(const struct device * dev, struct tm * tm) {
	(void) dev;

  /**
   * This will convert from struct tm to the internal structure needed for 
   * the stm32 calendar. Years are only supported from 00 to 99, and months and 
   * weekdays are indexed as 1 instead of 0, and inputs are in BCD rather than 
   * decimal
   */
	LL_RTC_DateTypeDef rtc_date = {
		.Year = tm->tm_year%100,
		.Month = __LL_RTC_CONVERT_BIN2BCD(tm->tm_mon + 1),
		.Day = tm->tm_mday,
		.WeekDay = tm->tm_wday + 1
	};

	LL_RTC_TimeTypeDef rtc_time = {
		.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24,
		.Hours = tm->tm_hour,
		.Minutes = tm->tm_min,
		.Seconds = tm->tm_sec
	};

	if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_date) != SUCCESS) {
		LOG_ERR("set date failed");
    return -ECANCELED;
	}

	if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_time) != SUCCESS) {
		LOG_ERR("set time failed");
    return -ECANCELED;
	}

	LOG_INF("Calendar time set to %lld (unix timestamp)", mktime(tm));

	return 0;
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
static int stm32_calendar_gettime(const struct device * dev, struct tm * tm) {

	// 0x00HHMMSS in bcd format
	uint32_t time = LL_RTC_TIME_Get(RTC);
	// 0xWWDDMMYY in bcd format
	uint32_t date = LL_RTC_DATE_Get(RTC);

	tm->tm_sec = __LL_RTC_CONVERT_BCD2BIN(time & 0xFF);
	tm->tm_min = __LL_RTC_CONVERT_BCD2BIN((time >> 8) & 0xFF);
	tm->tm_hour = __LL_RTC_CONVERT_BCD2BIN((time >> 16) & 0xFF);

	tm->tm_year = 100 + __LL_RTC_CONVERT_BCD2BIN(date & 0xFF);
	tm->tm_mon = __LL_RTC_CONVERT_BCD2BIN((date >> 8) & 0xFF) - 1;
	tm->tm_mday = __LL_RTC_CONVERT_BCD2BIN((date >> 16) & 0xFF);
	tm->tm_wday = __LL_RTC_CONVERT_BCD2BIN((date >> 24) & 0xFF) - 1;

	return 0;
}

/**
 * @brief Initialize the stm32 rtc. If the rtc is already setup
 * (e.g. it is running from battery), then don't reset the backup domain
 * 
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0
 */
static int stm32_rtc_initilize(const struct device *dev) {

	/* Clock Config */
  LL_PWR_EnableBkUpAccess();
  
  /* Only wipe the backup domain if:
  *
  * 1. It was requested or
  * 2. it hasn't been initialized.
  * 
  * We can check if it was already initialized by seeing if the magic word
  * is present in the first sram register
  */

  if(IS_ENABLED(CONFIG_RESET_BACKUP_DOMAIN) || 
    LL_RTC_BAK_GetRegister(RTC, LL_RTC_BKP_DR0) != BAK_SRAM_MAGIC)
  {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
  }

  LL_RCC_LSE_Enable();

   /* Wait till LSE is ready */
  while(LL_RCC_LSE_IsReady() != 1)
  {

  }
  LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
  LL_RCC_EnableRTC();

	/* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
	
  /* Initialize RTC */
  LL_RTC_InitTypeDef RTC_InitStruct = {0};

  /* Peripheral clock enable */
  LL_RCC_EnableRTC();

  RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
  RTC_InitStruct.AsynchPrescaler = RTC_PREDIV_ASYNC;
  RTC_InitStruct.SynchPrescaler = RTC_PREDIV_SYNC;

  LL_RTC_Init(RTC, &RTC_InitStruct);
  LL_RTC_SetAsynchPrescaler(RTC, RTC_PREDIV_ASYNC);
  LL_RTC_SetSynchPrescaler(RTC, RTC_PREDIV_SYNC);

  if(IS_ENABLED(CONFIG_RESET_BACKUP_DOMAIN) || 
    LL_RTC_BAK_GetRegister(RTC, LL_RTC_BKP_DR0) != BAK_SRAM_MAGIC)
  {
      struct tm * t_init;
      const time_t epoch = CONFIG_CALENDAR_INIT_TIME_UNIX_TIMESTAMP;
      t_init = localtime(&epoch);
      stm32_calendar_settime(dev, t_init);
      LL_RTC_BAK_SetRegister(RTC,LL_RTC_BKP_DR0, BAK_SRAM_MAGIC);
  }
	return 0;
}

static const struct calendar_driver_api stm32_calendar_api = {
	.settime = stm32_calendar_settime,
	.gettime = stm32_calendar_gettime,
};

DEVICE_DT_INST_DEFINE(0, stm32_rtc_initilize, device_pm_control_nop,
	NULL, NULL,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	&stm32_calendar_api
); 
