/**
 * @file rv3032.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2023-08-16
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include "rv3032.h"
#include "microcrystal_rv_cal.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "microcrystal_registers.h"

LOG_MODULE_DECLARE(calendar, CONFIG_CALENDAR_LOG_LEVEL);

static bool rv_3032_is_busy(const struct device * dev){
	const uint8_t reg = offsetof(rv_regmap_t, temp_registers);
	/* Read register with busy bit */
	uint8_t data = 0;
	int rc = rv_read(dev, reg, &data, 1);
	return (rc != 0) || ((data & RV3032_EEBUSY_FLAG) != 0);
}

static int rv3032_disable_auto_refresh(const struct device * dev){
	uint8_t auto_enabled = RV3032_EERD_FLAG;
	int rc = rv_read(dev, offsetof(rv_regmap_t, control1), &auto_enabled, 1);
	if (rc == 0)
	{
		uint8_t auto_disabled = auto_enabled & ~RV3032_EERD_FLAG;
		rc=rv_write(dev, offsetof(rv_regmap_t, control1), &auto_disabled, 1);
	}
	return rc;
}
static int rv3032_enable_auto_refresh(const struct device * dev){
	uint8_t auto_disabled = 0;
	int rc = rv_read(dev, offsetof(rv_regmap_t, control1), &auto_disabled, 1);
	if (rc == 0)
	{
		uint8_t auto_enabled = auto_disabled | RV3032_EERD_FLAG;
		rc=rv_write(dev, offsetof(rv_regmap_t, control1), &auto_enabled, 1);
	}
	return rc;
}

static int rv3032_write_eeprom(const struct device * dev, uint8_t addr, uint8_t * data, uint8_t len){
	
	/* Disable eeprom auto read, if enabled */
	int rc = 0;
	rv3032_disable_auto_refresh(dev);

	/* Wait for eeprom to be available for write */
	while(rv_3032_is_busy(dev)){
		k_msleep(5);
	}

	for(int i = 0; i < len; i++){
		/* Initiate a write command on EEPROM area */
		uint8_t cmd_write = RV3032_EE_COMMAND_WRITE;
		rc+=rv_write(dev, offsetof(rv_regmap_t, ee_addr), &addr, 1);
		rc+=rv_write(dev, offsetof(rv_regmap_t, ee_data), data + i, 1);
		rc+=rv_write(dev, offsetof(rv_regmap_t, ee_cmd), &cmd_write, 1);

		do{
			/* Wait until write cycle is finished */
			k_msleep(10);
		}while(rv_3032_is_busy(dev));
	}

	/* Re-enable eeprom auto read, if it was previousely enabled */
	rv3032_enable_auto_refresh(dev);

	return rc;
}

static int rv3032_read_eeprom(const struct device * dev, uint8_t addr, uint8_t * data, uint8_t len){
	
	int rc = 0;
	/* Disable eeprom auto read, if enabled */
	rv3032_disable_auto_refresh(dev);

	/* Wait for eeprom to be available for read */
	while(rv_3032_is_busy(dev)){
		k_msleep(5);
	}

	for(int i = 0; i < len; i++){
		/* Initiate a read command on EEPROM area */
		uint8_t cmd_read = RV3032_EE_COMMAND_READ;
		rc+=rv_write(dev, offsetof(rv_regmap_t, ee_addr), &addr, 1);
		rc+=rv_write(dev, offsetof(rv_regmap_t, ee_cmd), &cmd_read, 1);

		do{
			/* Wait until write cycle is finished */
			k_msleep(1);
		}while(rv_3032_is_busy(dev));

		rc+=rv_read(dev, offsetof(rv_regmap_t, ee_data), data + i, 1);
	}

	/* Re-enable eeprom auto read, if it was previousely enabled */
	rv3032_enable_auto_refresh(dev);
	
	return rc;
}

static int rv3032_get_cfg_register(const struct device * dev, uint8_t * cfg){
	return rv3032_read_eeprom(dev, RV3032_PMU_REG, cfg, 1);
}
static int rv3032_set_cfg_register(const struct device * dev, const uint8_t cfg){
	return rv3032_write_eeprom(dev, RV3032_PMU_REG, (uint8_t *)&cfg, 1);
}

int rv3032_init(const struct device * dev){
    /* Delay to ensure data from EEPROM has been copied to RAM mirror */
    k_msleep(66);

    /* Get the current configuration */
    uint8_t rvcfg = 0;
    rv3032_get_cfg_register(dev, &rvcfg);

    uint8_t pmu_cfg = CONFIG_RV3032_CLKOUT_ENABLE_MASK | 
        CONFIG_RV3032_SWITCHOVER_MODE_MASK | 
        CONFIG_RV3032_TCM_MASK |
        CONFIG_RV3032_TCR_MASK;
    
    if (rvcfg != pmu_cfg){
        /* Configuration does not match. Set configuration */
        LOG_DBG("rv3032 config mismatch. Have 0x%02x need 0x%02x", rvcfg, pmu_cfg);
        rv3032_set_cfg_register(dev, pmu_cfg);
    }
    return 0;
}
