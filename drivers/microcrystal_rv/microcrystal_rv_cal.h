/**
 * @file microcrystal_rv_cal.h
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2023-08-16
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#ifndef ZEPHYR_EXTRAS_INCLUDE_MICROCRYSTAL_CAL_H_
#define ZEPHYR_EXTRAS_INCLUDE_MICROCRYSTAL_CAL_H_
#include <zephyr/kernel.h>
#include <zephyr/device.h>

int rv_read(const struct device *dev, uint8_t reg, uint8_t *data, uint8_t len);
int rv_write(const struct device *dev, uint8_t reg, uint8_t *data, uint8_t len);

#endif
