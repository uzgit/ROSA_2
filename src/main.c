/*****************************************************************************

                 ,//////,   ,////    ,///' /////,
                ///' ./// ///'///  ///,    ,, //
               ///////,  ///,///   '/// ///''\\,
             ,///' '///,'/////',/////'  /////'\\,

    Copyright 2010 Marcus Jansson <mjansson256@yahoo.se>

    This file is part of ROSA - Realtime Operating System for AVR32.

    ROSA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ROSA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ROSA.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/
/* Tab size: 4 */

//Standard library includes
#include <avr32/io.h>

//Kernel includes
#include "kernel/rosa_ker.h"
#include "include/kernel/rosa_tim.h"

//Driver includes
#include "drivers/led.h"
#include "drivers/delay.h"
#include "drivers/usart.h"

//Include configuration
#include "rosa_config.h"


ROSA_taskHandle_t task1_handle;
ROSA_taskHandle_t task2_handle;
ROSA_taskHandle_t task3_handle;
ROSA_taskHandle_t task4_handle;
ROSA_taskHandle_t task5_handle;


/*************************************************************
 * Task1
 * LED0 lights up
 * LED1 goes dark
 ************************************************************/
void task1(void)
{
	while(1) {
		ledOn(LED0_GPIO);
		ledOff(LED1_GPIO);
		delay_ms(abs(1000 - ROSA_getTickCount()));
		ROSA_yield();
	}
}

/*************************************************************
 * Task2
 * LED0 goes dark
 * LED1 lights up
 ************************************************************/
void task2(void)
{
	while(1) {
		ledOff(LED0_GPIO);
		ledOn(LED1_GPIO);
		delay_ms(abs(ROSA_getTickCount() - 1000));
		ROSA_yield();
	}
}

/*************************************************************
 * Task3
 * LED2 toggles
 ************************************************************/
void task3(void)
{
	//uint64_t currentTime = ROSA_getTickCount();
	while(1) {
		ledToggle(LED2_GPIO);
		//ROSA_delay(30);
		//ROSA_delayUntil(&currentTime, 30);
		
		delay_ms(100);
		ROSA_yield();
	}
}

/*************************************************************
 * Task4
 * LED3 toggles
 ************************************************************/
void task4(void)
{
	while(1) {
		ledToggle(LED3_GPIO);
		delay_ms(200);
		ROSA_yield();
	}
}

/*************************************************************
 * Task2
 * deletes task5 after some time, then turns on LED4
 ************************************************************/
void task5(void)
{
	while(1)
	{
		//set green LED only
		ledOff(LED4_GPIO);
		ledOn(LED5_GPIO);
		ROSA_taskCreate(& task4_handle, "tsk4", task4, 0x40, 4); //blink LED4 on board
		//ROSA_tcbUninstall(task3_handle);
		//ROSA_tcbSuspend(task3_handle);
		//delay_ms(1000);
		ROSA_delay(100);
		
		ROSA_taskDelete(task4_handle); //stop blinking LED4 on board
		//ROSA_tcbUnsuspend(task3_handle);
		//ROSA_tcbInstall(task3_handle);
		ledOff(LED5_GPIO);	//set red LED only
		ledOn(LED4_GPIO);
		//delay_ms(1000);
		ROSA_delay(100);
		ROSA_yield();
	}
}

/*************************************************************
 * Main function
 ************************************************************/
int main(void)
{
	//Initialize the ROSA kernel
	ROSA_init();


	ROSA_taskCreate(& task1_handle, "tsk1", task1, 0x40, 5);
	ROSA_taskCreate(& task2_handle, "tsk2", task2, 0x40, 3);
	ROSA_taskCreate(& task3_handle, "tsk3", task3, 0x40, 2);
	//ROSA_taskCreate(& task4_handle, "tsk4", task4, 0x40, 1);
	ROSA_taskCreate(& task5_handle, "tsk5", task5, 0x40, 1);
	
	//Start the ROSA kernel
	ROSA_start();
	/* Execution will never return here */
	while(1);
}
