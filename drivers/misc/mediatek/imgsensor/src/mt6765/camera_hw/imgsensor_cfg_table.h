/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "imgsensor_common.h"

#include <linux/atomic.h>

#ifndef __IMGSENSOR_CFG_TABLE_H__
#define __IMGSENSOR_CFG_TABLE_H__

#define IMGSENSOR_DEV_NAME "kd_camera_hw"

#define IMGSENSOR_SENSOR_IDX_NAME_MAIN  "0"
#define IMGSENSOR_SENSOR_IDX_NAME_SUB   "1"
#define IMGSENSOR_SENSOR_IDX_NAME_MAIN2 "2"
#define IMGSENSOR_SENSOR_IDX_NAME_SUB2 "3"

#define IMGSENSOR_HW_POWER_INFO_MAX	16
#define IMGSENSOR_HW_SENSOR_MAX_NUM	16

enum IMGSENSOR_HW_PIN {
	IMGSENSOR_HW_PIN_NONE = 0,
	IMGSENSOR_HW_PIN_PDN,
	IMGSENSOR_HW_PIN_RST,
	IMGSENSOR_HW_PIN_AVDD,
	IMGSENSOR_HW_PIN_DVDD,
	IMGSENSOR_HW_PIN_DOVDD,
/* Hanyue.Shao@Camera.Driver, 2018/11/29, add for control i2c power start */
	IMGSENSOR_HW_PIN_SCL,
	IMGSENSOR_HW_PIN_SDA,
/* Hanyue.Shao@Camera.Driver, 2018/11/29, add for control i2c power end */
	IMGSENSOR_HW_PIN_AFVDD,
#ifdef MIPI_SWITCH
	IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
	IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
#endif
	IMGSENSOR_HW_PIN_MCLK,
	IMGSENSOR_HW_PIN_MAX_NUM,
	IMGSENSOR_HW_PIN_UNDEF = -1
};

enum IMGSENSOR_HW_PIN_STATE {
	IMGSENSOR_HW_PIN_STATE_LEVEL_0,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1000,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1100,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1200,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1210,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1220,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1500,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1800,
	IMGSENSOR_HW_PIN_STATE_LEVEL_2500,
	IMGSENSOR_HW_PIN_STATE_LEVEL_2800,
	IMGSENSOR_HW_PIN_STATE_LEVEL_2900,
	IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,

	IMGSENSOR_HW_PIN_STATE_NONE = -1
};

/*Legacy design*/
enum PowerType {
	PDN   = IMGSENSOR_HW_PIN_PDN,
	RST   = IMGSENSOR_HW_PIN_RST,
/* Hanyue.Shao@Camera.Driver, 2018/11/29, add for control i2c power start */
	SCL = IMGSENSOR_HW_PIN_SCL,
	SDA = IMGSENSOR_HW_PIN_SDA,
/* Hanyue.Shao@Camera.Driver, 2018/11/29, add for control i2c power end */
	AVDD  = IMGSENSOR_HW_PIN_AVDD,
	DVDD  = IMGSENSOR_HW_PIN_DVDD,
	DOVDD = IMGSENSOR_HW_PIN_DOVDD,
	AFVDD = IMGSENSOR_HW_PIN_AFVDD,
	VDD_None = IMGSENSOR_HW_PIN_NONE,

	/* For backward compatible */
	SensorMCLK = IMGSENSOR_HW_PIN_MCLK
};

enum Voltage {
	Vol_Low  = IMGSENSOR_HW_PIN_STATE_LEVEL_0,
	Vol_High = IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
	Vol_1000 = IMGSENSOR_HW_PIN_STATE_LEVEL_1000,
	Vol_1100 = IMGSENSOR_HW_PIN_STATE_LEVEL_1100,
	Vol_1200 = IMGSENSOR_HW_PIN_STATE_LEVEL_1200,
	Vol_1210 = IMGSENSOR_HW_PIN_STATE_LEVEL_1210,
	Vol_1220 = IMGSENSOR_HW_PIN_STATE_LEVEL_1220,
	Vol_1500 = IMGSENSOR_HW_PIN_STATE_LEVEL_1500,
	Vol_1800 = IMGSENSOR_HW_PIN_STATE_LEVEL_1800,
	Vol_2500 = IMGSENSOR_HW_PIN_STATE_LEVEL_2500,
	Vol_2800 = IMGSENSOR_HW_PIN_STATE_LEVEL_2800,
	Vol_2900 = IMGSENSOR_HW_PIN_STATE_LEVEL_2900,
};

enum IMGSENSOR_HW_ID {
	IMGSENSOR_HW_ID_REGULATOR,
	IMGSENSOR_HW_ID_GPIO,
/*	IMGSENSOR_HW_ID_MT6306,*/
	IMGSENSOR_HW_ID_MCLK,

	IMGSENSOR_HW_ID_MAX_NUM,
	IMGSENSOR_HW_ID_NONE = -1
};

#define IMGSENSOR_I2C_DRV_NAME_0  "kd_camera_hw"
#define IMGSENSOR_I2C_DRV_NAME_1  "kd_camera_hw_bus2"
#define IMGSENSOR_I2C_DRV_NAME_2  "kd_camera_hw_bus3"

#define IMGSENSOR_I2C_OF_DRV_NAME_0 "mediatek,camera_main"
#define IMGSENSOR_I2C_OF_DRV_NAME_1 "mediatek,camera_sub"
#define IMGSENSOR_I2C_OF_DRV_NAME_2 "mediatek,camera_main_two"

enum IMGSENSOR_I2C_DEV {
	IMGSENSOR_I2C_DEV_0,
	IMGSENSOR_I2C_DEV_1,
	IMGSENSOR_I2C_DEV_2,
	IMGSENSOR_I2C_DEV_MAX_NUM,
};

struct IMGSENSOR_HW_CUSTOM_POWER_INFO {
	enum IMGSENSOR_HW_ID  id;
	enum IMGSENSOR_HW_PIN pin;
};

struct IMGSENSOR_HW_CFG {
	enum   IMGSENSOR_SENSOR_IDX           sensor_idx;
	enum   IMGSENSOR_I2C_DEV              i2c_dev;
	struct IMGSENSOR_HW_CUSTOM_POWER_INFO
	    pwr_info[IMGSENSOR_HW_POWER_INFO_MAX];
};

struct IMGSENSOR_HW_POWER_INFO {
	enum IMGSENSOR_HW_PIN       pin;
	enum IMGSENSOR_HW_PIN_STATE pin_state_on;
	u32  pin_on_delay;
	enum IMGSENSOR_HW_PIN_STATE pin_state_off;
	u32  pin_off_delay;
};

struct IMGSENSOR_HW_POWER_SEQ {
	char                   *idx;
	struct IMGSENSOR_HW_POWER_INFO pwr_info[IMGSENSOR_HW_POWER_INFO_MAX];
};

struct IMGSENSOR_HW_DEVICE {
	void              *pinstance;
	enum IMGSENSOR_RETURN (*init)(void *arg);
	enum IMGSENSOR_RETURN (*release)(void *arg);
	enum IMGSENSOR_RETURN (*set)(
		void *arg,
		enum IMGSENSOR_SENSOR_IDX,
		enum IMGSENSOR_HW_PIN,
		enum IMGSENSOR_HW_PIN_STATE);
	enum IMGSENSOR_HW_ID    id;
};

extern struct IMGSENSOR_HW_CFG       imgsensor_custom_config[];
extern struct IMGSENSOR_HW_POWER_SEQ platform_power_sequence[];
extern struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[];
extern enum IMGSENSOR_RETURN
	(*hw_open[IMGSENSOR_HW_ID_MAX_NUM])(struct IMGSENSOR_HW_DEVICE **);

#endif
