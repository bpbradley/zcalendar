/**
 * @file microcrystal_registers.h
 * @author Brian Bradley (bbradley@lionprotects.com)
 * @date 2023-08-09
 * 
 * @copyright Copyright (C) 2023 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef ZEPHYR_EXTRAS_INCLUDE_MICROCRYSTAL_REGISTERS_H_
#define ZEPHYR_EXTRAS_INCLUDE_MICROCRYSTAL_REGISTERS_H_

#include <zephyr.h>
#include <device.h>

#if MICROCRYSTAL_RTC_RV8263
typedef struct  __attribute__ ((packed)) {
	uint8_t seconds;    	// 0x04
	uint8_t minutes;		// 0x05
	uint8_t hours;			// 0x06
	uint8_t date;			// 0x07
	uint8_t weekday;		// 0x08
	uint8_t month;			// 0x09
	uint8_t year;			// 0x0A
} rv_time_t;

typedef struct  __attribute__ ((packed)) {
	uint8_t	control1;		// 0x00
	uint8_t	control2;		// 0x01
	uint8_t	offset;			// 0x02
	uint8_t ram;			// 0x03
	rv_time_t calendar; //0x04 - 0x0A	
	uint8_t seconds_alarm;	// 0x0B
	uint8_t minutes_alarm;	// 0x0C
	uint8_t hours_alarm;	// 0x0D
	uint8_t date_alarm;		// 0x0E
	uint8_t weekday_alarm;	// 0x0F
	uint8_t timer_value;	// 0x10
	uint8_t timer_mode;		// 0x11
} rv_regmap_t;

#elif CONFIG_MICROCRYSTAL_RTC_RV3032

typedef struct  __attribute__ ((packed)) {
	uint8_t milliseconds; 	// 0x00
	uint8_t seconds;    	// 0x01
	uint8_t minutes;		// 0x02
	uint8_t hours;			// 0x03
	uint8_t weekday;		// 0x04
	uint8_t date;			// 0x05
	uint8_t month;			// 0x06
	uint8_t year;			// 0x07
} rv_time_t;

typedef struct  __attribute__ ((packed)) {
	uint8_t count;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t date;
	uint8_t month;
	uint8_t year;
} rv_timestamp_t;

typedef struct  __attribute__ ((packed)) {
	uint8_t count;
	uint8_t milliseconds;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t date;
	uint8_t month;
	uint8_t year;
} rv_ts_evi_t;

typedef struct  __attribute__ ((packed)) {
	rv_time_t calendar; 	// 0x00 - 0x07	
	uint8_t minutes_alarm;		// 0x08
	uint8_t hours_alarm;		// 0x09
	uint8_t date_alarm;			// 0x0A
	uint8_t timer_val0;			// 0x0B
	uint8_t timer_val1;			// 0x0C
	uint8_t status; 			// 0x0D
	uint16_t temp_registers;	// 0x0E - 0x0F
	uint8_t	control1;			// 0x10
	uint8_t	control2;			// 0x11
	uint8_t	control3;			// 0x12
	uint8_t	timestamp_ctl;		// 0x13
	uint8_t	clk_int_mask;		// 0x14
	uint8_t	evi_ctl;			// 0x15
	uint8_t	thresh_tlow;		// 0x16
	uint8_t	thresh_thigh;		// 0x17
	rv_timestamp_t ts_low; //0x18 - 0x1E
	rv_timestamp_t ts_hi;  //0x1F - 0x25
	rv_ts_evi_t ts_evi; 		//0x26 - 0x2D
	uint8_t reserved[11]; 		//0x2E - 0x38
	uint32_t password;			//0x39 - 0x3C
	uint8_t ee_addr;			//0x3D
	uint8_t ee_data;			//0x3E
	uint8_t ee_cmd;				//0x3F
	uint8_t magic;				//0x40
	uint8_t sram[15];			//0x41
} rv_regmap_t;
#endif

#endif
