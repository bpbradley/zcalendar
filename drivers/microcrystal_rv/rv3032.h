/**
 * @file rv3032.h
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2023-08-16
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#ifndef ZEPHYR_EXTRAS_INCLUDE_RV3032_H_
#define ZEPHYR_EXTRAS_INCLUDE_RV3032_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define RV3032_PMU_REG	        0xC0
#define RV3032_EEBUSY_FLAG	 	0x04
#define RV3032_EERD_FLAG		0x04
#define RV3032_EE_COMMAND_WRITE 0x21
#define RV3032_EE_COMMAND_READ 	0x22


int rv3032_init(const struct device * dev);

#endif
