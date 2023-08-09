# SPDX-License-Identifier: Apache-2.0
if(CONFIG_CALENDAR)
zephyr_library()

zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_STM32_RTC_CALENDAR drivers/stm32/stm32_rtc_cal.c)
zephyr_library_sources_ifdef(CONFIG_DS3231_RTC_CALENDAR drivers/ds3231/ds3231_cal.c)
zephyr_library_sources_ifdef(CONFIG_MICROCRYSTAL_RV_RTC_CALENDAR drivers/microcrystal_rv/microcrystal_rv_cal.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE calendar_handlers.c)
endif()
