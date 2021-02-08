# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_STM32_RTC_CALENDAR drivers/stm32/stm32_rtc_cal.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE calendar_handlers.c)