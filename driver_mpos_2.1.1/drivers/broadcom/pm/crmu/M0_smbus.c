/*
 * $Copyright Open Broadcom Corporation$
 */
#include "pm_regs.h"
#include <zephyr/types.h> 
#include "M0.h"
#include "../smbus.h"

void m0_smb_slave_rx_fifo_read(uint32_t *data);
void m0_issue_ds_cmd(void);

/* These are ported from pwr/mproc_pm.h that was used with Cortex M3 */
//#define MAILBOX_CODE1_NOTIFY_M3             0xFFFF0000
//#define MAILBOX_CODE1_PM                    0x0000FFFF
#define MAILBOX_CODE0_mProcPowerCMDMask     0xC7FFFFFF
#define MAILBOX_CODE0_mProcWakeup           0x10000000
#define MAILBOX_CODE0_mProcSleep            0x20000000
#define MAILBOX_CODE0_mProcDeepSleep        0x08000000
#define MAILBOX_CODE0_mProcSoftResetAndResume        0x40000000
#define MAILBOX_CODE0_mProcSMBUS            0x80000000
#define MAILBOX_CODE0_mProcFP               0x04000000

#define smb_buf_size       CRMU_SMBUS_BUF_SIZE
#define smb_buf            CRMU_SMBUS_BUFFER

#define smb_data_size_addr (CRMU_SMBUS_BUFFER+smb_buf_size)
#define smb_data_size      (*(volatile uint8_t *)smb_data_size_addr)
//#define smb_disable        (*(volatile uint8_t *)(smb_data_size_addr+1))

#define smb_tmp_1          ((uint32_t)(smb_data_size_addr+4))
#define smb_int_status_1 (*(volatile uint32_t *)smb_tmp_1)

#define smb_tmp_2          ((uint32_t)(smb_data_size_addr+8))
#define smb_config_reg_1 (*(volatile uint32_t *)smb_tmp_2)

#define smb_tmp_3          ((uint32_t)(smb_data_size_addr+0xc))
#define smb_data32_t     (*(volatile uint32_t *)smb_tmp_3)

#define  host_cs_state (*(volatile uint32_t *)CRMU_HOST_CS_STATE)

#define  smb_poa_wakeup (*(volatile uint32_t *)CRMU_POA_WAKEUP)

#define  poa_ec_counter  (*(volatile uint16_t *)CRMU_POA_EC_COUNTER)

#define  poa_latest_cmd  (*(volatile uint8_t *)CRMU_POA_LATEST_CMD)

/////////////////////////////////////////////////////////////////////////////////////////////
// ISR API     : Receive commands from SMBUS host/master
// 
// Description :
//              - Receive the Quick Command from pc to turn on/off the lynx
//              - Will resend the command to A7 via Mailbox mechanism
//              - Recover clock and power setting before wake up A7
//              - SMBus is initialized in A7; and the interrupt enabled in A7 as well.
//              - M0 return and goes into __wfi() sleep mode
/////////////////////////////////////////////////////////////////////////////////////////////
//

__attribute__ ((interrupt ("IRQ"))) s32_t m0_smbus_handler(void)
{
    uint32_t cmd = 0;
    uint32_t smb_int_status;

   /* Get the SMBUS events */
    smb_int_status = sys_read32(CRMU_SMBUS0_SMBus_Event_Status);

    if( smb_int_status & (SLAVE_RD_EVENT_INT | SLAVE_RX_EVENT_INT))
    {
        /* SMBus SLAVE_RD/RX_EVENT_INT detected */
        /* accept data from fifo */
        m0_smb_slave_rx_fifo_read((uint32_t*)smb_buf);

        /* de-assert SMB ALERT signal */
        (*(volatile uint32_t*)GP_DATA_OUT) |= (1<<4); // USH_SMB_ALERT_N = 4

        /* it is quick command ?*/
        if( smb_data_size == 1 )
        {
            if ( (*(volatile uint32_t *)smb_buf) == SMB_QUICK_COMMAND_ON)
            {
                /* power_on command from host */
                // wake up event...
                if ( sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_DEEPSLEEP )
                {
                    // will wake up from deep sleep
                    citadel_deep_sleep_exit();
                } else if ( sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_SLEEP )
                {
                   // will wake up from sleep
                    citadel_sleep_exit( 0, MPROC_DEFAULT_PM_MODE );
                } else
                {
                    goto NotifyA7;
                }
            }
            else if ( (*(volatile uint32_t *)smb_buf) == SMB_QUICK_COMMAND_OFF)
            {
              // Issue deepsleep command to A7 
                m0_issue_ds_cmd();
                goto ClearInterrupt;
            } 
        } 
        /* it is simple write byte protocol */
        else  if ( smb_data_size >= 2 ) 
        {
            /* EC command from Host */
            cmd = sys_read32(smb_buf + 4) & 0xff;

            /* 0X09 mean EC sees the SMBAlert, but won't be used for communication; ignore it.. */
            if( cmd == USH_POA_GET_CMD_STS ) goto ClearInterrupt;

            /* update the share data buffer with A7 */
            poa_latest_cmd = (uint8_t)cmd;

            /* increase the EC counter */
            poa_ec_counter++;

			if (cmd == USH_TURN_DEVICE_OFF) 	/* deep sleep mode */
			{
				smb_poa_wakeup = 0;
				// Issue deepsleep command to A7
				m0_issue_ds_cmd();
				goto ClearInterrupt; //return;
			}
			if (cmd == USH_DISABLE_DEVICES) /* connected standby */
			{
				/* PC/Host is in Connected Standby mode */
				host_cs_state = 1;
#ifdef ENABLE_CS
				/* Wake up Citadel to put FPM into detection mode if POA is enabled */
				if ((sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_SLEEP))
				{
					citadel_sleep_exit(0, MPROC_DEFAULT_PM_MODE);
				}
#endif
				goto ClearInterrupt; //return;
			}
			if (cmd == USH_TURN_DEVICE_ON)  /* wake up Citadel fron DS or CS mode */
			{
				smb_poa_wakeup = 0;
				/* PC/Host is not in Connect6ed Standby mode */
				host_cs_state = 0;
				if (sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_DEEPSLEEP)
				{
					// will wake up from deep sleep
					citadel_deep_sleep_exit();
				}
				else if (sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_SLEEP)
				{
					// will wake up from sleep by USB events, or smartcard events.
				}
				else
				{
					goto NotifyA7;
				}
				/*
					WSL for POA basic

					How about Citadel is in running or CS mode when received USH_POA_STARTUP_FP(0x02)????? TODO
				*/
				goto ClearInterrupt; //return;
			}
			if (cmd == USH_SOFT_RESET_DEVICE)
			{
				smb_poa_wakeup = 0;
				/* reset Citadel for system warm boot */
				sys_write32(0x2, CRMU_SOFT_RESET_CTRL); // emulate power up reset
				goto ClearInterrupt; //return;
			}
			/* POA command */
            if( cmd == USH_POA_STARTUP_FP )
            {
                    smb_poa_wakeup = 1;
                    /* PC/Host is not in Connect6ed Standby mode */
                    host_cs_state = 0;
                    if ( sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_DEEPSLEEP )
                    {
                        // will wake up from deep sleep
                        citadel_deep_sleep_exit();
                    } else if ( sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_SLEEP )
                    {
                        // will wake up from sleep by USB events, or smartcard events.
                    } else
                    {
                        goto NotifyA7;
                    }
                    /*
                        TODO for POA basic

                        How about Citadel is in running or CS mode when received USH_POA_STARTUP_FP(0x02)????? TODO
                    */
                    goto ClearInterrupt; //return;
            } 
        } 
    } else 
    { 
        /* not received data */
        /* clear the smb interrupt status */
        sys_write32(smb_int_status, CRMU_SMBUS0_SMBus_Event_Status);

        sys_write32(1 << CRMU_MCU_INTR_MASK__MCU_SMBUS_INTR_MASK, CRMU_MCU_INTR_CLEAR); //Clear SMBUS Interrupt
        return 1;
    }

NotifyA7:
    MCU_send_msg_to_mproc(MAILBOX_CODE0__SMBUS_ISR, MAILBOX_CODE1__SMBUS_ISR | smb_data_size | cmd << 8);

ClearInterrupt:
        /* clear the smb interrupt status */
   sys_write32(smb_int_status, CRMU_SMBUS0_SMBus_Event_Status);

   sys_write32(1 << CRMU_MCU_INTR_MASK__MCU_SMBUS_INTR_MASK, CRMU_MCU_INTR_CLEAR); //Clear SMBUS Interrupt

   return 0;
}

// +------------------+
// |Read Slave Rx Fifo|
// +------------------+
// This task reads the Slave Rx Fifo and returns the data.  Only one frame is
// read at a time
void m0_smb_slave_rx_fifo_read(uint32_t *data)
{
    uint32_t smb_data32;
    uint32_t smb_config_reg;
    uint32_t smb_disable = 0;

    smb_data_size = 0;
    smb_disable   = 0;    

    // Slave Rx Fifo Read cannot go through if SMBus is disabled.  Hence, if
    // SMbus is disabled, it is temporarily enabled to obtain the data
    smb_config_reg = sys_read32( CRMU_SMBUS0_SMBus_Config);

    if (!(smb_config_reg & (1U << CRMU_SMBUS0_SMBus_Config__SMB_EN)))
    {
        smb_config_reg |= (1U << CRMU_SMBUS0_SMBus_Config__SMB_EN);
        sys_write32(smb_config_reg,  CRMU_SMBUS0_SMBus_Config);
        smb_disable = 1;
    }

    // Read Until END-OF-FRAME is indicated
    do
    {
        // Read Slave Rx Fifo Until a complete Frame is obtained
        smb_data32 = sys_read32(CRMU_SMBUS0_SMBus_Slave_Data_Read);
        data[smb_data_size] = smb_data32;
        smb_data_size += 1;

        if (smb_data_size > smb_buf_size)
        {
         //   printf("ISR ERROR: Slave Rx Fifo Read Size(%0d) Too high!\n",i); 
            break;
        }
    } while ( (smb_data32>>CRMU_SMBUS0_SMBus_Slave_Data_Read__SLAVE_RD_STATUS_R) != 3 );

    if (smb_disable)
    {
        smb_config_reg &= ~(1U << CRMU_SMBUS0_SMBus_Config__SMB_EN);
        sys_write32(smb_config_reg,  CRMU_SMBUS0_SMBus_Config);
    }

    return;
}

// +--------------------------+
// | Send deepsleep CMD to A7 |
// +--------------------------+
void m0_issue_ds_cmd(void)
{
    uint32_t rd_data;

   // Citadel will enter deepsleep mode, disable wakeup source for suspended sleep mode.
    rd_data  = sys_read32(CRMU_MCU_EVENT_MASK);
    rd_data |= (0x1<<(CRMU_MCU_EVENT_MASK__MCU_USBPHY0_WAKE_EVENT_MASK)) | (0x1<<(CRMU_MCU_EVENT_MASK__MCU_USBPHY0_FILTER_EVENT_MASK)) |  //USB
               (0x1<<CRMU_MCU_EVENT_MASK__MCU_MPROC_CRMU_EVENT0_MASK);  //NFC
    sys_write32(rd_data, CRMU_MCU_EVENT_MASK);
   
    rd_data = sys_read32(CRMU_MCU_INTR_MASK);
    rd_data |=  (0x1<<(CRMU_MCU_INTR_MASK__MCU_AON_GPIO_INTR_MASK));    // SC_DET
    sys_write32(rd_data, CRMU_MCU_INTR_MASK);

    if ( sys_read32(CRMU_IHOST_POWER_CONFIG) == CPU_PW_STAT_SLEEP )
    {
        // for switching to deepsleep mode
        sys_write32(CPU_PW_STAT_DEEPSLEEP,  CRMU_IHOST_POWER_CONFIG);
        // will wake up from sleep, then A7 put itself into the deepsleep mode
        citadel_sleep_exit( 0, MPROC_DEFAULT_PM_MODE);
    } else
    {
        sys_write32((sys_read32(CRMU_IPROC_MAIL_BOX0) & MAILBOX_CODE0_mProcPowerCMDMask) | MAILBOX_CODE0_mProcDeepSleep, CRMU_IPROC_MAIL_BOX0);
        sys_write32(MAILBOX_CODE1__PM, CRMU_IPROC_MAIL_BOX1);
    }
}

