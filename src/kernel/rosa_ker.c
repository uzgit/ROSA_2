/*****************************************************************************

                 ///////,   .////    .///' /////,
                ///' ./// ///'///  ///,     '///
               ///////'  ///,///   '/// //;';//,
             ,///' ////,'/////',/////'  /////'/;,

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

//Kernel includes
#include "kernel/rosa_def.h"
#include "kernel/rosa_ext.h"
#include "kernel/rosa_ker.h"
#include "kernel/rosa_tim.h"
#include "kernel/rosa_scheduler.h"

//Driver includes
#include "drivers/button.h"
#include "drivers/led.h"
#include "drivers/pot.h"
#include "drivers/usart.h"

/***********************************************************
 * TCBLIST
 *
 * Comment:
 * 	Global variables that contain the list of TCB's that
 * 	have been installed into the kernel with ROSA_tcbInstall()
 **********************************************************/
tcb * TCBLIST;
tcb * TCBLIST_end;

tcb * SUSPENDEDLIST;
tcb * SUSPENDEDLIST_end;

//linked_list * ready_list;

/***********************************************************
 * EXECTASK
 *
 * Comment:
 * 	Global variables that contain the current running TCB.
 **********************************************************/
tcb * EXECTASK;

/***********************************************************
 * ROSA_init
 *
 * Comment:
 * 	Initialize the ROSA system
 *
 **********************************************************/
void ROSA_init(void)
{
	//Do initialization of I/O drivers
	ledInit();									//LEDs
	buttonInit();								//Buttons
	joystickInit();								//Joystick
	potInit();									//Potentiometer
	usartInit(USART, &usart_options, FOSC0);	//Serial communication

	//Start with empty TCBLIST and no EXECTASK.
	TCBLIST = NULL;
	EXECTASK = NULL;

	//Initialize the timer to 100 ms period.
	//...
	interruptInit();
	system_ticks = 0;
	timerInit(5);
	timerStart();
	//...
}

/***********************************************************
 * ROSA_tcbCreate
 *
 * Comment:
 * 	Create the TCB with correct values.
 *
 **********************************************************/
void ROSA_tcbCreate(tcb * tcbTask, char tcbName[NAMESIZE], void *tcbFunction, int * tcbStack, int tcbStackSize)
{
	int i;

	//Initialize the tcb with the correct values
	for(i = 0; i < NAMESIZE; i++) {
		//Copy the id/name
		tcbTask->id[i] = tcbName[i];
	}

	//Dont link this TCB anywhere yet.
	tcbTask->nexttcb = NULL;

	//Set the task function start and return address.
	tcbTask->staddr = tcbFunction;
	tcbTask->retaddr = (int)tcbFunction;

	//Set up the stack.
	tcbTask->datasize = tcbStackSize;
	tcbTask->dataarea = tcbStack + tcbStackSize;
	tcbTask->saveusp = tcbTask->dataarea;

	//Set the initial SR.
	tcbTask->savesr = ROSA_INITIALSR;

	//Initialize context.
	contextInit(tcbTask);
}


/***********************************************************
 * ROSA_tcbInstall
 *
 * Comment:
 * 	Install the TCB into the TCBLIST.
 *
 **********************************************************/

//void ROSA_tcbInstall(tcb * tcbTask)
//{
	//
	//tcb * tcbTmp;
//
	//// Is this the first tcb installed? 
	//if(TCBLIST == NULL) {
		//TCBLIST = tcbTask;
		//TCBLIST->nexttcb = tcbTask;			//Install the first tcb
		//tcbTask->nexttcb = TCBLIST;			//Make the list circular
	//}
	//else {
		//tcbTmp = TCBLIST;					//Find last tcb in the list
		//while(tcbTmp->nexttcb != TCBLIST) {
			//tcbTmp = tcbTmp->nexttcb;
		//}
		//tcbTmp->nexttcb = tcbTask;			//Install tcb last in the list
		//tcbTask->nexttcb = TCBLIST;			//Make the list circular
	//}
//}

// insert into the ready list
void ROSA_tcbInstall(tcb * tcbTask)
{
	// check if tcbTask is null is missing
	if(TCBLIST == NULL)
	{
		TCBLIST = tcbTask;
		tcbTask->nexttcb = tcbTask;
		tcbTask->prevtcb = tcbTask;
		TCBLIST_end = tcbTask;
	}
	else if(tcbTask->effective_priority < TCBLIST->effective_priority)
	{
		insert_after(TCBLIST_end, tcbTask);
		TCBLIST = tcbTask;
	}
	else
	{
		insert_by_priority(TCBLIST, tcbTask);
		if(TCBLIST->prevtcb == tcbTask)
		{
			TCBLIST_end = tcbTask;
		}		
	}
}

// removes a tcb from the ready list
void ROSA_tcbUninstall(tcb * tcbTask)
{
	// if empty OR if only one element
	if(TCBLIST == TCBLIST_end)
	{
		TCBLIST = NULL;
		TCBLIST_end = NULL;
	}
	else 
	{
		if(TCBLIST_end == tcbTask)
		{
			TCBLIST_end = tcbTask->prevtcb;
		}
		else if(TCBLIST == tcbTask)
		{
			TCBLIST = tcbTask->nexttcb;
		}
	
		tcbTask->prevtcb->nexttcb = tcbTask->nexttcb;
		tcbTask->nexttcb->prevtcb = tcbTask->prevtcb;
	
		tcbTask->nexttcb = NULL;
		tcbTask->prevtcb = NULL;
	}
}

void ROSA_tcbSuspend(tcb * tcbTask)
{
	// check if tcbTask is null is missing
	if(SUSPENDEDLIST == NULL)
	{
		SUSPENDEDLIST = tcbTask;
		tcbTask->nexttcb = tcbTask;
		tcbTask->prevtcb = tcbTask;
		SUSPENDEDLIST_end = tcbTask;
	}
	else if(tcbTask->back_online_time < SUSPENDEDLIST->back_online_time)
	{
		insert_after(SUSPENDEDLIST_end, tcbTask);
		SUSPENDEDLIST = tcbTask;
	}
	else
	{
		insert_by_back_online_time(SUSPENDEDLIST, tcbTask);
		if(SUSPENDEDLIST->prevtcb == tcbTask)
		{
			SUSPENDEDLIST_end = tcbTask;
		}
	}
}

void ROSA_tcbUnsuspend(tcb * tcbTask)
{
	// if empty OR if only one element
	if(SUSPENDEDLIST == SUSPENDEDLIST_end)
	{
		SUSPENDEDLIST = NULL;
		SUSPENDEDLIST_end = NULL;
	}
	else
	{
		if(SUSPENDEDLIST_end == tcbTask)
		{
			SUSPENDEDLIST_end = tcbTask->prevtcb;
		}
		else if(SUSPENDEDLIST == tcbTask)
		{
			SUSPENDEDLIST = tcbTask->nexttcb;
		}
	
		tcbTask->prevtcb->nexttcb = tcbTask->nexttcb;
		tcbTask->nexttcb->prevtcb = tcbTask->prevtcb;
	
		tcbTask->nexttcb = NULL;
		tcbTask->prevtcb = NULL;
	}
}

//void ROSA_tcbInstall(tcb * tcbTask)
//{
	//// working tcbinstall
	//// check if tcbTask is null is missing
	//if(TCBLIST == NULL)
	//{
		//TCBLIST = tcbTask;
		//tcbTask->nexttcb = tcbTask;
		//tcbTask->prevtcb = tcbTask;
		//TCBLIST_end = tcbTask;
	//}
	//else
	//{
		//insert_after(TCBLIST_end, tcbTask);
		//TCBLIST = tcbTask;
	//}
//}

int16_t ROSA_taskCreate(ROSA_taskHandle_t * th, char * id, void * taskFunc, uint32_t stackSize, uint8_t priority)
{
	int16_t result = -1;
	
	(*th) = (tcb*)calloc(1,sizeof(tcb));
	int* dynamic_stack = (int*)calloc(stackSize, sizeof(int));
	(*th)->priority = priority;
	(*th)->effective_priority = priority;
	(*th)->status = 1;
	(*th)->back_online_time = 0;
		
	ROSA_tcbCreate(*th, id, taskFunc, dynamic_stack, stackSize);
	ROSA_tcbInstall(*th);
	
	return result;
}

int16_t ROSA_taskDelete(ROSA_taskHandle_t th)
{
	uint16_t result = -1;
		
	if( th )
	{
		//prev = TCBLIST;
		//while( prev->nexttcb != th )
		//{
			//prev = prev->nexttcb;
		//}
		//prev->nexttcb = th->nexttcb;
		
		//if(TCBLIST_end == TCBLIST)
		//{
			//TCBLIST_end = NULL;
			//TCBLIST_end = NULL;
		//}
		
		if(TCBLIST_end == th)
		{
			TCBLIST_end = th->prevtcb;
		}
		if(TCBLIST == th)
		{
			TCBLIST = th->nexttcb;
		}
		
		th->prevtcb->nexttcb = th->nexttcb;
		th->nexttcb->prevtcb = th->prevtcb;
		
		free(th->dataarea - th->datasize);
		free(th);
		
		result = 0;
	}
	
	return result;
}


int16_t ROSA_delay(uint64_t ticks)
{
	//remove_tcb(EXECTASK);
	//EXECTASK->back_online_time=ROSA_getTickCount()+ticks;
	//insert_by_back_online_time(suspended_list, EXECTASK);
	interruptDisable();
	ROSA_tcbUninstall(EXECTASK);
	EXECTASK->back_online_time=ROSA_getTickCount()+ticks;
	ROSA_tcbSuspend(EXECTASK);
	interruptEnable();
	ROSA_yield();
}

int16_t ROSA_delayUntil(uint64_t* lastWakeTime, uint64_t ticks)
{
	//remove_tcb(EXECTASK);
	//EXECTASK->back_online_time=lastWakeTime+ticks;
	//*lastWakeTime=*lastWakeTime+ticks;
	//insert_by_back_online_time(suspended_list, EXECTASK);
	interruptDisable();
	ROSA_tcbUninstall(EXECTASK);
	//EXECTASK->back_online_time=*lastWakeTime+ticks;
	//*lastWakeTime=*lastWakeTime+ticks;
	EXECTASK->back_online_time=ROSA_getTickCount()+ticks;
	ROSA_tcbSuspend(EXECTASK);
	interruptEnable();
	ROSA_yield();
}

int16_t ROSA_delayAbsolute(uint64_t ticks)
{
	//remove_tcb(EXECTASK);
	//EXECTASK->back_online_time=ticks;
	//insert_by_back_online_time(suspended_list, EXECTASK);
	interruptDisable();
	ROSA_tcbUninstall(EXECTASK);
	EXECTASK->back_online_time=ticks;
	ROSA_tcbSuspend(EXECTASK);
	interruptEnable();
	ROSA_yield();
}