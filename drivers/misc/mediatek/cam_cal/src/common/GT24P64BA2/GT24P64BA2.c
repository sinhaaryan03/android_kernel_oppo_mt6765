/*
 * Copyright (C) 2015 MediaTek Inc.
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
/*
 * Driver for CAM_CAL
 *
 *
 */

#ifndef CONFIG_MTK_I2C_EXTENSION
#define CONFIG_MTK_I2C_EXTENSION
#endif
#include <linux/i2c.h>
#undef CONFIG_MTK_I2C_EXTENSION
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "GT24P64BA2.h"
/*#include <asm/system.h>//for SMP */

#ifdef CAM_CAL_DEBUG
#define CAM_CALDB pr_debug
#else
#define CAM_CALDB(x, ...)
#endif


#ifdef VENDOR_EDIT
/*Caohua.Lin@Camera.Driver 20180702 add for imx371 crosstalk*/
#define EEPROM_CHECK_ID_ADDR       0x0006
#define EEPROM_SENSORID_IMX371     0x1B
#define XTALK_DATA_SIZE 560
#define XTALK_START_ADDR 0x1400
#define XTALK_DATA_FLAG 0x1630
/*Caohua.Lin@Camera.Driver 20180707 add for s5k3p9sp crosstalk*/
#define EEPROM_SENSORID_S5K3P9SP   0xA2
#define S5K3P9SP_XTALK_DATA_FLAG   0x0769
#define S5K3P9SP_XTALK_START_ADDR  0x076A
#define S5K3P9SP_XTALK_DATA_SIZE   2048
static char s5k3p9sp_data_xtalk[2048];


//static char data_xtalk[560];
static unsigned int xtalk_readed = 0;
#endif

extern int iReadData_CAM_CAL(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata);

static DEFINE_SPINLOCK(g_CAM_CALLock);/* for SMP */
/* #define CAM_CAL_I2C_BUSNUM 1 */
#define CAM_CAL_I2C_BUSNUM 1
#define CAM_CAL_DEV_MAJOR_NUMBER 226

/* CAM_CAL READ/WRITE ID */
#define GT24P64BA2_DEVICE_ID							0xA2


/* for init.rc static struct i2c_board_info __initdata kd_cam_cal_dev={ I2C_BOARD_INFO("CAM_CAL_S24CS64A", 0xAA>>1)}; */

/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_ICS_REVISION 1 /* seanlin111208 */
/*******************************************************************************
*
********************************************************************************/
/* for init.rc #define CAM_CAL_DRVNAME "CAM_CAL_S24CS64A" */
#define CAM_CAL_DRVNAME "CAM_CAL_DRV"
/* #define CAM_CAL_I2C_GROUP_ID 0 */
/*******************************************************************************
*
********************************************************************************/
/*static struct i2c_board_info kd_cam_cal_dev __initdata = { I2C_BOARD_INFO(CAM_CAL_DRVNAME, 0xAA >> 1)};*/
static struct i2c_client *g_pstI2Cclient;

/* add for linux-4.4 */
#ifndef I2C_WR_FLAG
#define I2C_WR_FLAG		(0x1000)
#define I2C_MASK_FLAG	(0x00ff)
#endif
/*static struct class *CAM_CAL_class;*/
/*static atomic_t g_CAM_CALatomic;*/
/* static DEFINE_SPINLOCK(kdcam_cal_drv_lock); */
/* spin_lock(&kdcam_cal_drv_lock); */
/* spin_unlock(&kdcam_cal_drv_lock); */

/*******************************************************************************
*
********************************************************************************/
/* maximun read length is limited at "I2C_FIFO_SIZE" in I2c-mt65xx.c which is 8 bytes */
#if 0
static int iWriteCAM_CAL(u16 a_u2Addr, u32 a_u4Bytes, u8 *puDataInBytes)
{
	int  i4RetValue = 0;
	u32 u4Index = 0;
	char puSendCmd[8] = {(char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF),
			     0, 0, 0, 0, 0, 0
			    };
	if (a_u4Bytes + 2 > 8) {
		CAM_CALDB("[gt24p64ba2] exceed I2c-mt65xx.c 8 bytes limitation (include address 2 Byte)\n");
		return -1;
	}

	for (u4Index = 0; u4Index < a_u4Bytes; u4Index += 1)
		puSendCmd[(u4Index + 2)] = puDataInBytes[u4Index];

	i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2));
	if (i4RetValue != (a_u4Bytes + 2)) {
		CAM_CALDB("[gt24p64ba2] I2C write  failed!!\n");
		return -1;
	}
	mdelay(10); /* for tWR singnal --> write data form buffer to memory. */

	/* CAM_CALDB("[CAM_CAL] iWriteCAM_CAL done!!\n"); */
	return 0;
}
#endif

/* maximun read length is limited at "I2C_FIFO_SIZE" in I2c-mt65xx.c which is 8 bytes */
static int iReadCAM_CAL(u16 a_u2Addr, u32 ui4_length, u8 *a_puBuff)
{
	int  i4RetValue = 0;
	char puReadCmd[2] = {(char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF)};

	/* CAM_CALDB("[CAM_CAL] iReadCAM_CAL!!\n"); */

	if (ui4_length > 8) {
		CAM_CALDB("[gt24p64ba2] exceed I2c-mt65xx.c 8 bytes limitation\n");
		return -1;
	}

	spin_lock(&g_CAM_CALLock); /* for SMP */
	g_pstI2Cclient->addr = g_pstI2Cclient->addr & (I2C_MASK_FLAG | I2C_WR_FLAG);
	spin_unlock(&g_CAM_CALLock); /* for SMP */

	/* CAM_CALDB("[CAM_CAL] i2c_master_send\n"); */
	i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 2);
	if (i4RetValue != 2) {
		CAM_CALDB("[CAM_CAL] I2C send read address failed!!\n");
		return -1;
	}

	/* CAM_CALDB("[CAM_CAL] i2c_master_recv\n"); */
	i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_puBuff, ui4_length);
	if (i4RetValue != ui4_length) {
		CAM_CALDB("[CAM_CAL] I2C read data failed!!\n");
		return -1;
	}

	spin_lock(&g_CAM_CALLock); /* for SMP */
	g_pstI2Cclient->addr = g_pstI2Cclient->addr & I2C_MASK_FLAG;
	spin_unlock(&g_CAM_CALLock); /* for SMP */

	/* CAM_CALDB("[CAM_CAL] iReadCAM_CAL done!!\n"); */
	return 0;
}

#if 0
static int iWriteData(unsigned int  ui4_offset, unsigned int  ui4_length, unsigned char *pinputdata)
{
	int  i4RetValue = 0;
	int  i4ResidueDataLength;
	u32 u4IncOffset = 0;
	u32 u4CurrentOffset;
	u8 *pBuff;

	CAM_CALDB("[gt24p64ba2] iWriteData\n");

	if (ui4_offset + ui4_length >= 0x2000) {
		CAM_CALDB("[gt24p64ba2] Write Error!! gt24p64ba2 not supprt address >= 0x2000!!\n");
		return -1;
	}

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;

	CAM_CALDB("[gt24p64ba2] iWriteData u4CurrentOffset is %d\n", u4CurrentOffset);

	do {
		if (i4ResidueDataLength >= 6) {
			i4RetValue = iWriteCAM_CAL((u16)u4CurrentOffset, 6, pBuff);
			if (i4RetValue != 0) {
				CAM_CALDB("[CAM_CAL] I2C iWriteData failed!!\n");
				return -1;
			}
			u4IncOffset += 6;
			i4ResidueDataLength -= 6;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
		} else {
			i4RetValue = iWriteCAM_CAL((u16)u4CurrentOffset, i4ResidueDataLength, pBuff);
			if (i4RetValue != 0) {
				CAM_CALDB("[CAM_CAL] I2C iWriteData failed!!\n");
				return -1;
			}
			u4IncOffset += 6;
			i4ResidueDataLength -= 6;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
			/* break; */
		}
	} while (i4ResidueDataLength > 0);
	CAM_CALDB("[gt24p64ba2] iWriteData done\n");

	return 0;
}
#endif
/* int iReadData(stCAM_CAL_INFO_STRUCT * st_pOutputBuffer) */
static int iReadData(unsigned int  ui4_offset, unsigned int  ui4_length, unsigned char *pinputdata)
{
	int  i4RetValue = 0;
	int  i4ResidueDataLength;
	u32 u4IncOffset = 0;
	u32 u4CurrentOffset;
	u8 *pBuff;
	/* CAM_CALDB("[S24EEPORM] iReadData\n" ); */

	if (ui4_offset + ui4_length >= 0x2000) {
		CAM_CALDB("[gt24p64ba2] Read Error!! gt24p64ba2 not supprt address >= 0x2000!!\n");
		return -1;
	}

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	do {
		if (i4ResidueDataLength >= 8) {
			i4RetValue = iReadCAM_CAL((u16)u4CurrentOffset, 8, pBuff);
			if (i4RetValue != 0) {
				CAM_CALDB("[gt24p64ba2] I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += 8;
			i4ResidueDataLength -= 8;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
		} else {
			i4RetValue = iReadCAM_CAL((u16)u4CurrentOffset, i4ResidueDataLength, pBuff);
			if (i4RetValue != 0) {
				CAM_CALDB("[gt24p64ba2] I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += 8;
			i4ResidueDataLength -= 8;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
			/* break; */
		}
	} while (i4ResidueDataLength > 0);



	return 0;
}
unsigned int gt24p64ba2_selective_read_region(struct i2c_client *client, unsigned int addr,
	unsigned char *data, unsigned int size)
{
	#ifdef VENDOR_EDIT
	char xtalk_flag = 0;
	#endif

	g_pstI2Cclient = client;
	if (!xtalk_readed) {
			iReadData(S5K3P9SP_XTALK_DATA_FLAG, 1, &xtalk_flag);
			if (xtalk_flag == 0x55) {
				pr_debug("s5k3p9sp's to read 4cell algorithm\n");
				iReadData(S5K3P9SP_XTALK_START_ADDR, S5K3P9SP_XTALK_DATA_SIZE, s5k3p9sp_data_xtalk);
				xtalk_readed = 1;
			} else {
				pr_debug("Warning, s5k3p9sp'otp not have cross-talk calibration data !\n");
			}

	}
	if(iReadData(addr, size, data) == 0){
	    return size;
	}
	else
		return 0;
}
#ifdef VENDOR_EDIT
/*Caohua.Lin@Camera.Driver 20180702 add for imx371 crosstalk*/
/*
unsigned int gt24p64ba2_read_4cell_from_eeprom(char *data)
{
	if (data != NULL) {
		memcpy((void*)(data + 2), (void*)data_xtalk, XTALK_DATA_SIZE);
		pr_debug("imx371 read 4cell eeprom data[0]=%d, data[10]=%d, data[100]=%d, data[559]=%d \n", data[2], data[12], data[102], data[561]);
	}
	return 0;
}
*/

/*Caohua.Lin@Camera.Driver 20180707 add for s5k3p9sp crosstalk*/
unsigned int gt24p64ba2_read_4cell_from_eeprom_s5k3p9sp(char *data)
{
	if (data != NULL) {
		data[0] = (S5K3P9SP_XTALK_DATA_SIZE &0xFF);
		data[1] = ((S5K3P9SP_XTALK_DATA_SIZE >> 8) & 0XFF);
		memcpy((void*)(data + 2), (void*)s5k3p9sp_data_xtalk, S5K3P9SP_XTALK_DATA_SIZE);
		pr_debug("s5k3p9sp read 4cell size data[0]=%d,data[1]=%d\n", data[0], data[1]);
		pr_debug("s5k3p9sp read 4cell data[0]=%d,data[1]=%d,data[10]=%d, data[100]=%d, data[2047]=%d \n", data[2], data[3],data[12], data[102], data[2047]);
	}
	return 0;
}
#endif



/*#define gt24p64ba2_DRIVER_ON*/
#ifdef gt24p64ba2_DRIVER_ON

/*******************************************************************************
*
********************************************************************************/
#define NEW_UNLOCK_IOCTL
#ifndef NEW_UNLOCK_IOCTL
static int CAM_CAL_Ioctl(struct inode *a_pstInode,
			 struct file *a_pstFile,
			 unsigned int a_u4Command,
			 unsigned long a_u4Param)
#else
static long CAM_CAL_Ioctl(
	struct file *file,
	unsigned int a_u4Command,
	unsigned long a_u4Param
)
#endif
{
	int i4RetValue = 0;
	u8 *pBuff = NULL;
	u8 *pWorkingBuff = NULL;
	stCAM_CAL_INFO_STRUCT *ptempbuf;
	u8 readTryagain = 0, test_retry = 0;

#ifdef CAM_CALGETDLT_DEBUG
	struct timeval ktv1, ktv2;
	unsigned long TimeIntervalUS;
#endif



	if (_IOC_DIR(a_u4Command) != _IOC_NONE) {
		pBuff = kmalloc(sizeof(stCAM_CAL_INFO_STRUCT), GFP_KERNEL);

		if (pBuff == NULL) {
			CAM_CALDB("[gt24p64ba2] ioctl allocate mem failed\n");
			return -ENOMEM;
		}

		if (_IOC_WRITE & _IOC_DIR(a_u4Command)) {
			if (copy_from_user((u8 *) pBuff, (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT))) {
				/* get input structure address */
				kfree(pBuff);
				CAM_CALDB("[gt24p64ba2] ioctl copy from user failed\n");
				return -EFAULT;
			}
		}
	}

	ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;
	pWorkingBuff = kmalloc(ptempbuf->u4Length, GFP_KERNEL);
	if (pWorkingBuff == NULL) {
		kfree(pBuff);
		CAM_CALDB("[gt24p64ba2] ioctl allocate mem failed\n");
		return -ENOMEM;
	}
	CAM_CALDB("[gt24p64ba2] init Working buffer address 0x%8x  command is 0x%8x\n",
	(u32)pWorkingBuff, (u32)a_u4Command);


	if (copy_from_user((u8 *)pWorkingBuff, (u8 *)ptempbuf->pu1Params, ptempbuf->u4Length)) {
		kfree(pBuff);
		kfree(pWorkingBuff);
		CAM_CALDB("[gt24p64ba2] ioctl copy from user failed\n");
		return -EFAULT;
	}

	switch (a_u4Command) {
	case CAM_CALIOC_S_WRITE:
		CAM_CALDB("[gt24p64ba2] Write CMD\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif
		i4RetValue = iWriteData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff);
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		CAM_CALDB("Write data %d bytes take %lu us\n", ptempbuf->u4Length, TimeIntervalUS);
#endif
		break;
	case CAM_CALIOC_G_READ:
		CAM_CALDB("[gt24p64ba2] Read CMD\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif
		CAM_CALDB("[gt24p64ba2] offset %d\n", ptempbuf->u4Offset);
		CAM_CALDB("[gt24p64ba2] length %d\n", ptempbuf->u4Length);
		CAM_CALDB("[gt24p64ba2] Before read Working buffer address 0x%8x\n", (u32)pWorkingBuff);

		/* i4RetValue = iReadData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff); */
		CAM_CALDB("[GT24P64BA2_CAM_CAL] After read Working buffer data  0x%4x\n", *pWorkingBuff);

		if (ptempbuf->u4Offset == 0x800) {
			*(u16 *)pWorkingBuff = 0x3;

		} else if (ptempbuf->u4Offset == 0x000CB032) {
			*(u32 *)pWorkingBuff = 0x000CB032;
		} else if (ptempbuf->u4Offset == 0x100CB032) {
			*(u32 *)pWorkingBuff = 0x100CB032;
		} else if (ptempbuf->u4Offset == 0xFFFFFFFF) {
			char puSendCmd[1] = {0, };

			puSendCmd[0] = 0x7E;
			/* iReadRegI2C(puSendCmd , 1, pWorkingBuff,2, (0x53<<1) ); */
			CAM_CALDB("[gt24p64ba2] Shading CheckSum MSB=> %x %x\n", pWorkingBuff[0],
			pWorkingBuff[1]);
		} else {

			readTryagain = 3;
			test_retry = 2;
			while (readTryagain > 0) {








				i4RetValue = iReadData((u16)ptempbuf->u4Offset, ptempbuf->u4Length,
				pWorkingBuff);







				CAM_CALDB("[gt24p64ba2] error (%d) Read retry (%d)\n", i4RetValue,
				readTryagain);
				if (i4RetValue != 0)
					readTryagain--;
				else
					readTryagain = 0;

			}


		}

#if 1
		if (1) {
			unsigned short addr;
			unsigned short get_byte = 0, loop = 0;

			/* iReadReg(0x0000 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0000, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0000, get_byte);

			get_byte = 0;

			/* iReadReg(0x0001 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0001, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0001, get_byte);


			get_byte = 0;

			/* iReadReg(0x0380 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0380, 1, (u8 *)&get_byte);


			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0380, get_byte);

			get_byte = 0;

			/* iReadReg(0x0381 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0381, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0381, get_byte);


			get_byte = 0;

			/* iReadReg(0x0703 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0703, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0703, get_byte);

			get_byte = 0;

			/* iReadReg(0x0704 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0704, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0704, get_byte);

			get_byte = 0;

			/* iReadReg(0x0705 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0705, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0705, get_byte);

			get_byte = 0;

			/* iReadReg(0x0706 ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x0706, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x0706, get_byte);

			get_byte = 0;

			/* iReadReg(0x07fc ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x07fc, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x07fc, get_byte);

			get_byte = 0;

			/* iReadReg(0x07fd ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x07fd, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x05, get_byte);

			get_byte = 0;

			/* iReadReg(0x07fe ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x07fe, 1, (u8 *)&get_byte);


			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x05, get_byte);

			get_byte = 0;

			/* iReadReg(0x07ff ,(u8*)&get_byte,0xA0); */
			iReadCAM_CAL((u16)0x07fe, 1, (u8 *)&get_byte);

			CAM_CALDB("[gt24p64ba2]enter EEPROM_test function%d (%x)%x\n", ++loop,
			0x05, get_byte);

			get_byte = 0;


		}
#endif


#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		CAM_CALDB("Read data %d bytes take %lu us\n", ptempbuf->u4Length, TimeIntervalUS);
#endif

		break;
	default:
		CAM_CALDB("[gt24p64ba2] No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	if (_IOC_READ & _IOC_DIR(a_u4Command)) {
		/* copy data to user space buffer, keep other input paremeter unchange. */
		CAM_CALDB("[gt24p64ba2] to user length %d\n", ptempbuf->u4Length);
		CAM_CALDB("[gt24p64ba2] to user  Working buffer address 0x%8x\n", (u32)pWorkingBuff);
		if (copy_to_user((u8 __user *) ptempbuf->pu1Params, (u8 *)pWorkingBuff,
			ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pWorkingBuff);
			CAM_CALDB("[gt24p64ba2] ioctl copy to user failed\n");
			return -EFAULT;
		}
	}

	kfree(pBuff);
	kfree(pWorkingBuff);
	return i4RetValue;
}


static u32 g_u4Opened;
/* #define */
/* Main jobs: */
/* 1.check for device-specified errors, device not ready. */
/* 2.Initialize the device if it is opened for the first time. */
static int CAM_CAL_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	int result = 0;

	CAM_CALDB("[GT24P64BA2_CAM_CAL] CAM_CAL_Open\n");
	spin_lock(&g_CAM_CALLock);
	if (g_u4Opened) {
		result = -EBUSY;
	} else {
		g_u4Opened = 1;
		atomic_set(&g_CAM_CALatomic, 0);
	}
	spin_unlock(&g_CAM_CALLock);
	return result;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
static int CAM_CAL_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	spin_lock(&g_CAM_CALLock);

	g_u4Opened = 0;

	atomic_set(&g_CAM_CALatomic, 0);

	spin_unlock(&g_CAM_CALLock);

	return 0;
}

static const struct file_operations g_stCAM_CAL_fops = {
	.owner = THIS_MODULE,
	.open = CAM_CAL_Open,
	.release = CAM_CAL_Release,
	/* .ioctl = CAM_CAL_Ioctl */
	.unlocked_ioctl = CAM_CAL_Ioctl
};

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
static inline int RegisterCAM_CALCharDrv(void)
{
	struct device *CAM_CAL_device = NULL;

#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
	if (alloc_chrdev_region(&g_CAM_CALdevno, 0, 1, CAM_CAL_DRVNAME)) {
		CAM_CALDB("[GT24P64BA2_CAM_CAL] Allocate device no failed\n");

		return -EAGAIN;
	}
#else
	if (register_chrdev_region(g_CAM_CALdevno, 1, CAM_CAL_DRVNAME)) {
		CAM_CALDB("[GT24P64BA2_CAM_CAL] Register device no failed\n");

		return -EAGAIN;
	}
#endif

	/* Allocate driver */
	g_pCAM_CAL_CharDrv = cdev_alloc();

	if (g_pCAM_CAL_CharDrv == NULL) {
		unregister_chrdev_region(g_CAM_CALdevno, 1);

		CAM_CALDB("[GT24P64BA2_CAM_CAL] Allocate mem for kobject failed\n");

		return -ENOMEM;
	}

	/* Attatch file operation. */
	cdev_init(g_pCAM_CAL_CharDrv, &g_stCAM_CAL_fops);

	g_pCAM_CAL_CharDrv->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(g_pCAM_CAL_CharDrv, g_CAM_CALdevno, 1)) {
		CAM_CALDB("[GT24P64BA2_CAM_CAL] Attatch file operation failed\n");

		unregister_chrdev_region(g_CAM_CALdevno, 1);

		return -EAGAIN;
	}

	CAM_CAL_class = class_create(THIS_MODULE, "CAM_CALdrv");

	if (IS_ERR(CAM_CAL_class)) {
		int ret = PTR_ERR(CAM_CAL_class);

		CAM_CALDB("Unable to create class, err = %d\n", ret);
		return ret;
	}
	CAM_CAL_device = device_create(CAM_CAL_class, NULL, g_CAM_CALdevno, NULL, CAM_CAL_DRVNAME);

	return 0;
}

static inline void UnregisterCAM_CALCharDrv(void)
{
	/* Release char driver */
	cdev_del(g_pCAM_CAL_CharDrv);

	unregister_chrdev_region(g_CAM_CALdevno, 1);

	device_destroy(CAM_CAL_class, g_CAM_CALdevno);
	class_destroy(CAM_CAL_class);
}


/* //////////////////////////////////////////////////////////////////// */
#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
#elif 0
static int CAM_CAL_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
#else
#endif
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int CAM_CAL_i2c_remove(struct i2c_client *);

static const struct i2c_device_id CAM_CAL_i2c_id[] = {{CAM_CAL_DRVNAME, 0}, {} };

/* static const unsigned short * const forces[] = { force, NULL }; */
/* static struct i2c_client_address_data addr_data = { .forces = forces,}; */


static struct i2c_driver CAM_CAL_i2c_driver = {
	.probe = CAM_CAL_i2c_probe,
	.remove = CAM_CAL_i2c_remove,
	/* .detect = CAM_CAL_i2c_detect, */
	.driver.name = CAM_CAL_DRVNAME,
	.id_table = CAM_CAL_i2c_id,
};

#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strcpy(info->type, CAM_CAL_DRVNAME);
	return 0;
}
#endif
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i4RetValue = 0;

	CAM_CALDB("[GT24P64BA2_CAM_CAL] Attach I2C\n");
	/* spin_lock_init(&g_CAM_CALLock); */

	/* get sensor i2c client */
	spin_lock(&g_CAM_CALLock); /* for SMP */
	g_pstI2Cclient = client;
	g_pstI2Cclient->addr = GT24P64BA2_DEVICE_ID >> 1;
	spin_unlock(&g_CAM_CALLock); /* for SMP */

	CAM_CALDB("[GT24P64BA2_CAM_CAL] g_pstI2Cclient->addr = 0x%8x\n", g_pstI2Cclient->addr);
	/* Register char driver */
	i4RetValue = RegisterCAM_CALCharDrv();

	if (i4RetValue) {
		CAM_CALDB("[GT24P64BA2_CAM_CAL] register char device failed!\n");
		return i4RetValue;
	}


	CAM_CALDB("[GT24P64BA2_CAM_CAL] Attached!!\n");
	return 0;
}

static int CAM_CAL_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int CAM_CAL_probe(struct platform_device *pdev)
{
	return i2c_add_driver(&CAM_CAL_i2c_driver);
}

static int CAM_CAL_remove(struct platform_device *pdev)
{
	i2c_del_driver(&CAM_CAL_i2c_driver);
	return 0;
}

/* platform structure */
static struct platform_driver g_stCAM_CAL_Driver = {
	.probe              = CAM_CAL_probe,
	.remove     = CAM_CAL_remove,
	.driver             = {
		.name   = CAM_CAL_DRVNAME,
		.owner  = THIS_MODULE,
	}
};


static struct platform_device g_stCAM_CAL_Device = {
	.name = CAM_CAL_DRVNAME,
	.id = 0,
	.dev = {
	}
};
asdkljjkfdkljsadkljasfljk
static int __init CAM_CAL_i2C_init(void)
{
	i2c_register_board_info(CAM_CAL_I2C_BUSNUM, &kd_cam_cal_dev, 1);
	if (platform_driver_register(&g_stCAM_CAL_Driver)) {
		CAM_CALDB("failed to register GT24P64BA2_CAM_CAL driver\n");
		return -ENODEV;
	}

	if (platform_device_register(&g_stCAM_CAL_Device)) {
		CAM_CALDB("failed to register GT24P64BA2_CAM_CAL driver, 2nd time\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit CAM_CAL_i2C_exit(void)
{
	platform_driver_unregister(&g_stCAM_CAL_Driver);
}

module_init(CAM_CAL_i2C_init);
module_exit(CAM_CAL_i2C_exit);

MODULE_DESCRIPTION("CAM_CAL driver");
MODULE_AUTHOR("Dream Yeh <Dream.Yeh@Mediatek.com>");
MODULE_LICENSE("GPL");

#endif
