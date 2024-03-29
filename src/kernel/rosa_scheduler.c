/*****************************************************************************

                 ,//////,   ,////    ,///' /////,
                ///' ./// ///'///  ///,    ,, //
               ///////,  ///,///   '/// //;''//,
             ,///' '///,'/////',/////'  /////'/;,

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

#include "kernel/rosa_scheduler.h"
#include "kernel/rosa_ker.h"
#include "kernel/rosa_tim.h"

/***********************************************************
 * scheduler
 *
 * Comment:
 * 	Minimalistic scheduler for round robin task switch.
 * 	This scheduler choose the next task to execute by looking
 * 	at the nexttcb of the current running task.
 **********************************************************/
void scheduler(void)
{
	
	tcb * iterator = SUSPENDEDLIST;
	uint64_t current_time = ROSA_getTickCount();
	while( iterator != NULL && iterator->back_online_time <= current_time ) //for every suspended task that is now ready
	{
		ROSA_tcbUnsuspend(iterator);
		ROSA_tcbInstall(iterator);
		if (SUSPENDEDLIST)
		{
			iterator = iterator->nexttcb;
		}
		else
		{
			iterator = NULL;
		}
	}
	//Find the next task to execute
	//EXECTASK = EXECTASK->nexttcb;
	EXECTASK=TCBLIST;
	
}
