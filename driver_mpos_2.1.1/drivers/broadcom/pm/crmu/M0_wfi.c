/*
 * $Copyright Open Broadcom Corporation$
 */
#include "M0.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// ISR API     : Got notified when A7 into sleeping mode with WFI/WFE
// 
// Description :
//              - Test: update a register for debug 
//              - Configure GPIO/timer as wakeup source for M0
//              - M0 return and goes into __wfi() sleep mode
/////////////////////////////////////////////////////////////////////////////////////////////
//
__attribute__ ((interrupt ("IRQ"))) s32_t m0_wfi_handler(void) 
{
	
	u32_t sleep_mode = sys_read32(CRMU_IHOST_POWER_CONFIG);

    // update the log "register" with +1
	sys_write32( sys_read32(CRMU_SLEEP_EVENT_LOG_0) + 1, CRMU_SLEEP_EVENT_LOG_0);
	
    // Save the A7 status to the log "register"
    sys_write32( sys_read32( DRIPS_OR_DEEPSLEEP_REG) , CRMU_SLEEP_EVENT_LOG_1);

    if ( sleep_mode == CPU_PW_STAT_DEEPSLEEP) {
         // deep sleep 
         MCU_SoC_do_policy();
         MCU_SoC_DeepSleep_Handler();
    } else if ( sleep_mode == CPU_PW_STAT_SLEEP ) {
	    // Sleep 
        MCU_SoC_do_policy();
        MCU_SoC_Sleep_Handler();
    } else {
		// DRIPS mode
        MCU_SoC_do_policy();
        MCU_SoC_DRIPS_Handler();
	}
    sys_write32((0x1<<CRMU_MCU_EVENT_CLEAR__MCU_IPROC_STANDBYWFI_EVENT_CLR), CRMU_MCU_EVENT_CLEAR); //Clear SLEEPING event

    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// ISR1 API   : Power up sequence
// 
// Description :
//              - Power up PLL (and XTAL) sequence
//              - Configure GPIO/timer as wakeup source for M0
//              - M0 return and goes into __wfi() sleep mode
/////////////////////////////////////////////////////////////////////////////////////////////
//
__attribute__ ((interrupt ("IRQ"))) s32_t m0_aon_phy0_resume_handler(void)
{
	u32_t rd_event, mask;
	u32_t power_state = sys_read32(CRMU_IHOST_POWER_CONFIG);
	u32_t power_state2 = sys_read32(DRIPS_OR_DEEPSLEEP_REG)& PM_STATE_MASK_SET;
	// make sure we're sleeping

    // Check to see if this is resume event.  Seems to need both event bits set (TBD why)
	mask = (0x1<<(CRMU_MCU_EVENT_MASK__MCU_USBPHY0_WAKE_EVENT_MASK)) | (0x1<<(CRMU_MCU_EVENT_MASK__MCU_USBPHY0_FILTER_EVENT_MASK));
	rd_event = sys_read32(CRMU_MCU_EVENT_STATUS) & mask;
	
    /* not resume event. clear and ignore. */
	sys_write32(rd_event, CRMU_MCU_EVENT_CLEAR);
	
	sys_write32(0x55aa55aa, 0x30012e40);
	/* resume event and on DRIPS mode */
	if( (rd_event == mask) && (power_state == CPU_PW_STAT_DEEPSLEEP) && (power_state2 == CPU_PW_STAT_DRIPS) )
    {
		/* wake up mproc from DMU */
		sys_write32(0x55aa55aa, 0x30012e44);
		citadel_DRIPS_exit();
		sys_write32(0x55aa55aa, 0x30012e48);
	}

	return 1;
}


