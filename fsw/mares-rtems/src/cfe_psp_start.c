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

/******************************************************************************
** File:  cfe_psp_start.c
**
** Purpose:
**   cFE BSP main entry point.
**
**
******************************************************************************/
#define _USING_RTEMS_INCLUDES_

/*
**  Include Files
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <rtems.h>
#include <bsp.h>

/*
** cFE includes 
*/
#include "common_types.h"
#include "osapi.h"
#include "cfe_psp.h" 
#include "cfe_psp_memory.h"
#include "cfe_psp_module.h"


/*
 * The preferred way to obtain the CFE tunable values at runtime is via
 * the dynamically generated configuration object.  This allows a single build
 * of the PSP to be completely CFE-independent.
 */
#include <target_config.h>

#define CFE_PSP_MAIN_FUNCTION        (*GLOBAL_CONFIGDATA.CfeConfig->SystemMain)
#define CFE_PSP_NONVOL_STARTUP_FILE  (GLOBAL_CONFIGDATA.CfeConfig->NonvolStartupFile)

/*
** Global variables
*/

rtems_id          RtemsTimerId;

/*
** 1 HZ Timer "ISR"
*/
int timer_count = 0;

/******************************************************************************
**  Function:  CFE_PSP_Setup()
**
**  Purpose:
**    Perform initial setup.
**
**    This function is invoked before OSAL is initialized.
**      NO OSAL CALLS SHOULD BE USED YET.
**
**    The root file system is created, and mount points are created and mounted:
**     - /ram as ramdisk (RFS), read-write
**     - /boot from /dev/hda1, read-only, contain the boot executable(s) (CFE core)
**
**  Arguments:
**    (none)
**
**  Return:
**    OS error code.  RTEMS_SUCCESSFUL if everything worked.
**
**  Note:
**    If this fails then CFE will not run properly, so a non-success here should
**    stop the boot so the issue can be fixed.  Trying to continue booting usually
**    just obfuscates the issue when something does not work later on.
*/
int CFE_PSP_Setup(void)
{
   return RTEMS_SUCCESSFUL;
}

/******************************************************************************
**  Function:  CFE_PSP_SetupSystemTimer
**
**  Purpose:
**    BSP system time base and timer object setup.
**    This does the necessary work to start the 1Hz time tick required by CFE
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
**
** NOTE:
**      The handles to the timebase/timer objects are "start and forget"
**      as they are supposed to run forever as long as CFE runs.
**
**      If needed for e.g. additional timer creation, they can be recovered
**      using an OSAL GetIdByName() call.
**
**      This is preferred anyway -- far cleaner than trying to pass the uint32 value
**      up to the application somehow.
*/

void CFE_PSP_SetupSystemTimer(void)
{
    uint32 SystemTimebase;
    int32  Status;

    Status = OS_TimeBaseCreate(&SystemTimebase, "cFS-Master", NULL);
    if (Status == OS_SUCCESS)
    {
        Status = OS_TimeBaseSet(SystemTimebase, 250000, 250000);
    }

    /*
     * If anything failed, cFE/cFS will not run properly, so a panic is appropriate
     */
    if (Status != OS_SUCCESS)
    {
        OS_printf("CFE_PSP: Error configuring cFS timing: %d\n", (int)Status);
        CFE_PSP_Panic(Status);
    }
}

/*
** A simple entry point to start from the BSP loader
**
** This entry point is used when building an RTEMS+CFE monolithic
** image, which is a single executable containing the RTEMS
** kernel and Core Flight Executive in one file.  In this mode
** the RTEMS BSP invokes the "Init" function directly.
**
** This sets up the root fs and the shell prior to invoking CFE via
** the CFE_PSP_Main() routine.
**
** In a future version this code may be moved into a separate bsp
** integration unit to be more symmetric with the VxWorks implementation.
*/
void OS_Application_Startup(void)
{
   if (CFE_PSP_Setup() != RTEMS_SUCCESSFUL)
   {
       CFE_PSP_Panic(CFE_PSP_ERROR);
   }

   /*
   ** Run the PSP Main - this will return when init is complete
   */
   CFE_PSP_Main();
}

/******************************************************************************
**  Function:  CFE_PSP_Main()
**
**  Purpose:
**    Application entry point.
**
**    The basic RTEMS system including the root FS and shell (if used) should
**    be running prior to invoking this function.
**
**    This entry point is used when building a separate RTEMS kernel/platform
**    boot image and Core Flight Executive image.  This is the type of deployment
**    used on e.g. VxWorks platforms.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/

void CFE_PSP_Main(void)
{
   uint32            reset_type;
   uint32            reset_subtype;
   uint32            fs_id;
   int32 Status;

   /*
   ** Initialize the OS API
   */
   Status = OS_API_Init();
   if (Status != OS_SUCCESS)
   {
       /* irrecoverable error if OS_API_Init() fails. */
       /* note: use printf here, as OS_printf may not work */
       printf("CFE_PSP: OS_API_Init() failure\n");
       CFE_PSP_Panic(Status);
   }

   /*
    * Initialize the CFE reserved memory map
    */
   CFE_PSP_SetupReservedMemoryMap();

   /*
   ** Set up the virtual FS mapping for the "/cf" directory
   */
   Status = OS_FileSysAddFixedMap(&fs_id, "/eeprom", "/cf");
   if (Status != OS_SUCCESS)
   {
       /* Print for informational purposes --
        * startup can continue, but loads may fail later, depending on config. */
       OS_printf("CFE_PSP: OS_FileSysAddFixedMap() failure: %d\n", (int)Status);
   }

   /*
   ** Initialize the statically linked modules (if any)
   */
   CFE_PSP_ModuleInit();

   /* Prepare the system timing resources */
   CFE_PSP_SetupSystemTimer();

   /*
   ** Determine Reset type by reading the hardware reset register.
   */
   reset_type = CFE_PSP_RST_TYPE_POWERON;
   reset_subtype = CFE_PSP_RST_SUBTYPE_POWER_CYCLE;

   /*
   ** Initialize the reserved memory 
   */
   CFE_PSP_InitProcessorReservedMemory(reset_type);

   /*
   ** Call cFE entry point. This will return when cFE startup
   ** is complete.
   */
   CFE_PSP_MAIN_FUNCTION(reset_type,reset_subtype, 1, CFE_PSP_NONVOL_STARTUP_FILE);

   /*
   ** Enable 1Hz
   */

   CFE_PSP_Setup1HzInterrupt();


}
