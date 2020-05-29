/* Temporary cv main function to driver cvmanager until RTOS integration is done */
 
#include "cvmain.h"

#define TWINKLE_LED

#ifdef TWINKLE_LED
#include "phx.h"
#include "memory_map.h"
#include "gpio.h"
#endif

/* Function prototypes */
extern void cv_initialize(void);
extern void cv_manager(void);
extern void GeneralUsbHandler(void);

#define LED_PIN 0x4000	/* 5880 Demo Board */

void cv_main(void)
{	

cv_encap_command cv_cmd;
uint32_t i;

	cv_initialize();
	
	*GPIO_REG_EN |= LED_PIN;	
    *GPIO_REG_DOUT |= LED_PIN;   /* Turn LED ON */       
         
	while(1) {
		/* Run USB stuff */
	  	GeneralUsbHandler();
  
#if 0	  	
        *GPIO_REG_DOUT |= LED_PIN;   /* Turn LED ON */                                                   
        for (i=0; i<10000; i++);                                                     
        *GPIO_REG_DOUT &= ~LED_PIN;  /* Turn LED OFF */                                                                             
        for (i=0; i<10000; i++);     
#endif

    }
}

void cv_callbk(uint8_t *buf, uint16_t len)
{
	/* copy command to io buf */
	memcpy(CV_SECURE_IO_COMMAND_BUF, buf, len);

	/* Call cvManager */
	cvManager();

	return;	
}
