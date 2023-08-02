/**
 * @file calendar_handlers.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @brief Userspace command handlers for calendar driver api
 * @date 2020-10-08
 * 
 * @copyright Copyright (C) 2020 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zcal/calendar.h>
#include <zephyr/kernel.h>
#include <zephyr/syscall_list.h>

static inline int z_vrfy_calendar_gettime(const struct device *dev, struct tm * tm) 
{ 
    Z_OOPS(Z_SYSCALL_DRIVER_CALENDAR(dev, gettime)); 
    return z_impl_calendar_gettime((const struct device *)dev, tm); 
}
#include <syscalls/calendar_gettime_mrsh.c>

static inline int z_vrfy_calendar_settime(const struct device *dev, struct tm * tm) 
{ 
    Z_OOPS(Z_SYSCALL_DRIVER_CALENDAR(dev, settime));
    return z_impl_calendar_settime((const struct device *)dev, tm); 
}
#include <syscalls/calendar_settime_mrsh.c>
