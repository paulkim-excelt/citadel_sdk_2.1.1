/******************************************************************************
*
* Copyright (c) 2009, Broadcom Co.
* 16215 Alton Parkway
* PO Box 57013
* Irvine CA 92619-7013
*
* File: smbus.c
* Desc: Routines to be called for Power Management on A7
*
* Revision History:
*
*****************************************************************************/

#include <board.h>
#include "pm_regs.h"
#include "broadcom/i2c.h"
#include "smbus.h"
//#include "volatile_mem.h"
#include "logging/sys_log.h"

static union dev_config i2c_slave_cfg = {
    .bits = {
        .use_10_bit_addr = 0,
        .is_master_device = 0,
        .speed = I2C_SPEED_STANDARD,
    },
};

/*********************************************************************************************

 * Function:
 *      smbus_init
 * Purpose:
 *		This is a utility routine that sets up the SMBUS hardware block for normal operation.
 * Parameters: NONE
 * Returns:
 *		None.

****************************************************************************************************************/
void smbus_init()
{
	//u8_t enable = 1;
	//u32_t gpioPinVal;
    /* SMBUS/I2C TODO, may need to pull this in which needs PHX_REQ_SMBUS to be defined in bootrom/Makefile
       and volatile_mem.h needs to include smbus.h instead of smbusdriver.h */
    /* PHX2_SMBUS_FIFO_DATA_PTR->lowPowerState = 0; */
	struct device *i2c_dev;   /* SMBUS/I2C slave device */
    int ret;

    SYS_LOG_DBG("In %s\n",__func__);

	/* Need this for Citadel ?
    if ( lynx_is_fast_XIP() )
    {
        printf("\nskip sumbus init for S3 resume..\n");
        return;
	} 
	*/ 

	/* test the registers against their reset value */
	/* SYS_LOG_DBG("%s: i2c_slave_smb_blk_full_reg_test() returns %d\n",__func__,i2c_slave_smb_blk_full_reg_test()); */

	/* SMBUS/I2C slave device */
    /* NEED TO RESOLVE USE OF DEVICE BY configure_spi2_sel() in \fw\fp_spi\fp_spi_enum.c */	
    i2c_dev = device_get_binding(CONFIG_I2C0_NAME);
    if (i2c_dev == NULL) {
        SYS_LOG_DBG("I2C Driver binding not found!\n");
        return;
    }

    ret = i2c_configure(i2c_dev, i2c_slave_cfg.raw);
    if (ret) {
        SYS_LOG_DBG("i2c_configure() returned error\n");
        return;
    }

	/* enable the SMBUS hardware block */
	//i2c_slave_smb_enable_smb(enable);


	/* set the default slave address of the SMBUS block */
    /* Slave address is set in i2c driver */
	/* phx_smb_set_slave_address(DEFAULT_SMB_SLAVE_ADDRESS, 0); */
	
	/* Enable interrupts */
	//i2c_slave_smb_slave_interrupt_enable();
}

/* Get the latest EC command from host
 * In Lynx, this is named ush_poa_latest_cmd()
 */
u8_t get_latest_ec_cmd(void)
{
    return *(volatile u8_t *)CRMU_POA_LATEST_CMD;
}

