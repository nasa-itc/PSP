# leon3-rtems PSP

This PSP is based off of the "pc-rtems" PSP, and is intended for LEON3 targets running RTEMS 5.x

This differs from the pc-rtems PSP, mainly in the fact that it does not include the RTEMS startup code, but leaves that to the mission to supply a kernel that can either link in the cFE core, or dynamically load it. 
