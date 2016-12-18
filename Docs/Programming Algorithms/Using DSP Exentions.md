Using DSP Exentions
====================

For embedded audio processing, most of the best boards use a processor from the ARM Cortex M family.  The Teensy 3.2 and 3.6, for example, use an ARM Cortex M4.  The M4 has special features built into the chip that accelerate certain kinds of math operations that are common in Digital Signal Processing (DSP).  While these M4 processors are fast relative to a basic Arduino UNO, they're not nearly as fast as a full-blown PC.  So, if you want to get the most audio processing done on these little processors, you really need to use these DSP extensions and not rely just pure-C/C++ implementations of your favorite math operations.

ARM CMSIS: Cortex 
-----------------------

Because ARM wants their chip cores to be widely used (which they are), and to ease a programmer's job of transitioning between different models of ARM chips (which they've successfully done), ARM has created a large collection of standardized resources that the programmer can invoke.  This is the "Cortex Microcontroller Software Interface Standard" ((CMSIS))[https://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php].  The CMSIS standard covers a wide range of resources, from real-time operating system through DSP functions.  The idea is that the programmer learns how to use them for one specific ARM Cortex chip, but that the same knowledge will also work on variety of ARM Cortex chips that the programmer might encounter in the future.  It's like the standard C library, yet optimized for the specific capabilities of the ARM Cortex family.
