#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "threads.h"
#include "padc.h"
#include "usbcfg.h"
#include "shell.h"

static const ShellCommand commands[] = {
	{"debug", debugOnUSB},
	{"picture", printPicture},
	{"log", readLog},
	{"config", printConfig},
	{"command", command2Camera},
	{NULL, NULL}
};

static const ShellConfig shell_cfg = {
	(BaseSequentialStream*)&SDU1,
	commands
};

/**
  * Main routine is starting up system, runs the software watchdog (module monitoring), controls LEDs
  */
int main(void) {
	halInit();					// Startup HAL
	chSysInit();				// Startup RTOS

	// Voltage switching (1.8V <=> 3.0V)
	#if ACTIVATE_USB || ACTIVATE_3V
	boost_voltage(true);		// Ramp up voltage to 3V
	chThdSleepMilliseconds(100);
	#endif



	/*// Clear Wakeup flag
	PWR->CR |= PWR_CR_CWUF;

	// Select STANDBY mode
	PWR->CR |= PWR_CR_PDDS;

	// Set SLEEPDEEP bit of Cortex System Control Register
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	// This option is used to ensure that store operations are completed
	#if defined ( __CC_ARM   )
	__force_stores();
	#endif
	// Request Wait For Interrupt
	__WFI();
	while(1);*/

	// Init debugging (Serial debug port, LEDs)
	DEBUG_INIT();
	TRACE_INFO("MAIN > Startup");

	// Start USB
	#if ACTIVATE_USB
	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(100);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);
	usb_initialized = true;
	#endif

	// Startup threads
	start_essential_threads();	// Startup required modules (tracking managemer, watchdog)
	start_user_modules();		// Startup optional modules (eg. POSITION, LOG, ...)

	while(true) {
		#if ACTIVATE_USB
		if(SDU1.config->usbp->state == USB_ACTIVE) {
			thread_t *shelltp = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(512), "shell", NORMALPRIO+1, shellThread, (void*)&shell_cfg);
			chThdWait(shelltp);
		}
		#endif
		chThdSleepMilliseconds(10000);
	}
}

