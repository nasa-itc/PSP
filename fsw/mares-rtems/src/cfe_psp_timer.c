/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/************************************************************************************************
** File:  cfe_psp_timer.c
**
** Purpose:
**   This file contains glue routines between the cFE and the OS Board Support Package ( BSP ).
**   The functions here allow the cFE to interface functions that are board and OS specific
**   and usually dont fit well in the OS abstraction layer.
**
** History:
**   2005/06/05  K.Audra    | Initial version,
**
*************************************************************************************************/

/*
**  Include Files
*/

/*
**  Include Files
*/
#include <stdio.h>
#include <stdlib.h>

/*
** cFE includes
*/
#include "common_types.h"
#include "osapi.h"

/*
** Types and prototypes for this module
*/
#include "cfe_psp.h"



/******************* Macro Definitions ***********************/

#define FPGA_TIME_CMD_REGISTER              0x80000900

#define FPGA_TIME_SECONDS_REGISTER          0x80000904
#define FPGA_TIME_SUBSECONDS_REGISTER       0x80000908
#define FPGA_TIME_LOAD_SECS_REGISTER        0x8000090c

#define FPGA_TIME_CTRL_REGISTER             0x80000910
#define FPGA_TIME_VERSION_REGISTER          0x80000914
#define FPGA_TIME_USOCLK_2MHZ_GENREGISTER   0x80000918

#define FPGA_TIME_LOAD_CMD_VALUE            0xA

#define CFE_PSP_TIMER_TICKS_PER_SECOND      1000000    /* Resolution of the least significant 32 bits of the 64 bit
                                                           time stamp returned by CFE_PSP_Get_Timebase in timer ticks per second.
                                                           The timer resolution for accuracy should not be any slower than 1000000
                                                           ticks per second or 1 us per tick */
#define CFE_PSP_TIMER_LOW32_ROLLOVER         0           /* The number that the least significant 32 bits of the 64 bit
                                                           time stamp returned by CFE_PSP_Get_Timebase rolls over.  If the lower 32
                                                           bits rolls at 1 second, then the CFE_PSP_TIMER_LOW32_ROLLOVER will be 1000000.
                                                           if the lower 32 bits rolls at its maximum value (2^32) then
                                                           CFE_PSP_TIMER_LOW32_ROLLOVER will be 0. */

/******************************************************************************
**  Function:  CFE_PSP_GetTime()
**
**  Purpose: Gets the value of the time from the hardware
**
**  Arguments: LocalTime - where the time is returned through
******************************************************************************/

void CFE_PSP_GetTime( OS_time_t *LocalTime)
{
    LocalTime->seconds  = *((uint32 *)FPGA_TIME_SECONDS_REGISTER);

    LocalTime->microsecs = *((uint32 *)FPGA_TIME_SUBSECONDS_REGISTER) >> 12;

 
}/* end CFE_PSP_GetLocLOCalTime */


/*
** Function Name: OS_GetLocalMET
**
** Purpose: Called by CFE_TIME function to read the time seconds from h/w register
**
*/
void OS_GetLocalMET(uint32 *Seconds)
{
    *Seconds = *((uint32 *)FPGA_TIME_SECONDS_REGISTER);
}


/*
** Function Name: OS_SetLocalMET
**
** Purpose: Called by CFE_TIME function to write time seconds to h/w register
**
*/
void OS_SetLocalMET(uint32 Seconds)
{
    
    *((uint32 *)FPGA_TIME_LOAD_SECS_REGISTER) = Seconds;
    *((uint32 *)FPGA_TIME_CMD_REGISTER) = FPGA_TIME_LOAD_CMD_VALUE;
}

 
/******************************************************************************
**  Function:  CFE_PSP_Get_Timer_Tick()
**
**  Purpose:
**    Provides a common interface to system clock tick. This routine
**    is in the BSP because it is sometimes implemented in hardware and
**    sometimes taken care of by the RTOS.
**
**  Arguments:
**
**  Return:
**  OS system clock ticks per second
*/
uint32 CFE_PSP_Get_Timer_Tick(void)
{
   return (0);
}

/******************************************************************************
**  Function:  CFE_PSP_GetTimerTicksPerSecond()
**
**  Purpose:
**    Provides the resolution of the least significant 32 bits of the 64 bit
**    time stamp returned by CFE_PSP_Get_Timebase in timer ticks per second.
**    The timer resolution for accuracy should not be any slower than 1000000
**    ticks per second or 1 us per tick
**
**  Arguments:
**
**  Return:
**    The number of timer ticks per second of the time stamp returned
**    by CFE_PSP_Get_Timebase
*/
uint32 CFE_PSP_GetTimerTicksPerSecond(void)
{
    return(CFE_PSP_TIMER_TICKS_PER_SECOND);
}

/******************************************************************************
**  Function:  CFE_PSP_GetTimerLow32Rollover()
**
**  Purpose:
**    Provides the number that the least significant 32 bits of the 64 bit
**    time stamp returned by CFE_PSP_Get_Timebase rolls over.  If the lower 32
**    bits rolls at 1 second, then the CFE_PSP_TIMER_LOW32_ROLLOVER will be 1000000.
**    if the lower 32 bits rolls at its maximum value (2^32) then
**    CFE_PSP_TIMER_LOW32_ROLLOVER will be 0.
**
**  Arguments:
**
**  Return:
**    The number that the least significant 32 bits of the 64 bit time stamp
**    returned by CFE_PSP_Get_Timebase rolls over.
*/
uint32 CFE_PSP_GetTimerLow32Rollover(void)
{
    return(CFE_PSP_TIMER_LOW32_ROLLOVER);
}

/******************************************************************************
**  Function:  CFE_PSP_Get_Timebase()
**
**  Purpose:
**    Provides a common interface to system timebase. This routine
**    is in the BSP because it is sometimes implemented in hardware and
**    sometimes taken care of by the RTOS.
**
**  Arguments:
**
**  Return:
**  Timebase register value
*/
void CFE_PSP_Get_Timebase(uint32 *Tbu, uint32* Tbl)
{
    *Tbu = *((uint32 *)FPGA_TIME_SECONDS_REGISTER);
    *Tbl = *((uint32 *)FPGA_TIME_SUBSECONDS_REGISTER);
}

/******************************************************************************
**  Function:  CFE_PSP_Get_Dec()
**
**  Purpose:
**    Provides a common interface to decrementer counter. This routine
**    is in the BSP because it is sometimes implemented in hardware and
**    sometimes taken care of by the RTOS.
**
**  Arguments:
**
**  Return:
**  Timebase register value
*/

uint32 CFE_PSP_Get_Dec(void)
{
   return(0);
}

