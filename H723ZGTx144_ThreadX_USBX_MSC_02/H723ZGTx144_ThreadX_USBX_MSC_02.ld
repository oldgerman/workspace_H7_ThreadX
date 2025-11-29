/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                1024Kbytes FLASH and 560Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2025 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x1000;      /* required amount of heap  */
_Min_Stack_Size = 0x4000; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  ITCMRAM  (xrw)   : ORIGIN = 0x00000000,   LENGTH = 64K    /* RAM_ITCM  */
  DTCMVTOR (xrw)   : ORIGIN = 0x20000000,   LENGTH = 1K     /* VTOR_DTCM */
  DTCMRAM  (xrw)   : ORIGIN = 0x20000400,   LENGTH = 127K   /* RAM_DTCM  */
  AXISRAM1 (xrw)   : ORIGIN = 0x24000000,   LENGTH = 256K   /* RAM_D1    */
  AXISRAM2 (xrw)   : ORIGIN = 0x24040000,   LENGTH = 64K    /* RAM_D1    */
  SRAM1    (xrw)   : ORIGIN = 0x30000000,   LENGTH = 16K    /* RAM_D2    */
  SRAM2    (xrw)   : ORIGIN = 0x30004000,   LENGTH = 16K    /* RAM_D2    */
  SRAM4    (xrw)   : ORIGIN = 0x38000000,   LENGTH = 16K    /* RAM_D3    */
  BKPRAM   (xrw)   : ORIGIN = 0x38800000,   LENGTH = 4K     /* RAM_D3    */
  PSRAM    (xrw)   : ORIGIN = 0x90000000,   LENGTH = 64M    /* RAM_PSRAM */
  FLASH    (rx)    : ORIGIN = 0x08000000,   LENGTH = 1024K  /* ROM_FLASH */
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH
  .ARM (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* 【教程】：https://club.rt-thread.org/ask/article/1f31bacada2c883b.html */
  /* .xxxsram_data 位于.data section上方*/
  /* 用于指定编译 USB 驱动需从 "加载地址" (Flash) 复制到"运行地址" (XXXSRAM) 的数组 
     此 Section 添加后，还需要在 .s启动文件 中添加汇编代码复制Flash数据到XXXSRAM */
  /* 定义加载地址起始（FLASH 中存储初始值的位置） */
/*AXISRAM1 Sections Begin ---------------------------------------------------*/
  _si_axisram1_data = LOADADDR(.axisram1_data);
  
  .axisram1_data :
  {
    . = ALIGN(4);
    _s_axisram1_data = .;        /* 运行地址起始 */
    *(.axisram1_data)
    *(.axisram1_data*)
    
    . = ALIGN(4);
    _e_axisram1_data = .;        /* 运行地址结束 */
  } >AXISRAM1 AT> FLASH
  
  /* Uninitialized data section */
  . = ALIGN(4);
  
  .axisram1_bss (NOLOAD):
  {
    . = ALIGN(4);
    _s_axisram1_bss = .;  /* create a global symbol at data start */
    *(.axisram1_bss)      /* .axisram1_bss sections */
    *(.axisram1_bss*)     /* .axisram1_bss* sections */
    . = ALIGN(4);
    _e_axisram1_bss = .;  /* define a global symbol at data end */
  } >AXISRAM1
  
/*AXISRAM1 Sections End -----------------------------------------------------*/

/*AXISRAM2 Sections Begin ---------------------------------------------------*/
  _si_axisram2_data = LOADADDR(.axisram2_data);
  
  .axisram2_data :
  {
    . = ALIGN(4);
    _s_axisram2_data = .;        /* 运行地址起始 */
    *(.axisram2_data)
    *(.axisram2_data*)
    
    . = ALIGN(4);
    _e_axisram2_data = .;        /* 运行地址结束 */
  } >AXISRAM2 AT> FLASH
  
  /* Uninitialized data section */
  . = ALIGN(4);
  
  .axisram2_bss (NOLOAD):
  {
    . = ALIGN(4);
    _s_axisram2_bss = .;  /* create a global symbol at data start */
    *(.axisram2_bss)      /* .axisram2_bss sections */
    *(.axisram2_bss*)     /* .axisram2_bss* sections */
    . = ALIGN(4);
    _e_axisram2_bss = .;  /* define a global symbol at data end */
  } >AXISRAM2
  
/*AXISRAM2 Sections End -----------------------------------------------------*/

/*SRAM1 Sections Begin ---------------------------------------------------*/
  _si_sram1_data = LOADADDR(.sram1_data);
  
  .sram1_data :
  {
    . = ALIGN(4);
    _s_sram1_data = .;        /* 运行地址起始 */
    *(.sram1_data)
    *(.sram1_data*)
    
    . = ALIGN(4);
    _e_sram1_data = .;        /* 运行地址结束 */
  } >SRAM1 AT> FLASH
  
  /* Uninitialized data section */
  . = ALIGN(4);
  
  .sram1_bss (NOLOAD):
  {
    . = ALIGN(4);
    _s_sram1_bss = .;  /* create a global symbol at data start */
    *(.sram1_bss)      /* .sram1_bss sections */
    *(.sram1_bss*)     /* .sram1_bss* sections */
    . = ALIGN(4);
    _e_sram1_bss = .;  /* define a global symbol at data end */
  } >SRAM1
  
/*SRAM1 Sections End -----------------------------------------------------*/

/*SRAM2 Sections Begin ---------------------------------------------------*/
  _si_sram2_data = LOADADDR(.sram2_data);
  
  .sram2_data :
  {
    . = ALIGN(4);
    _s_sram2_data = .;        /* 运行地址起始 */
    *(.sram2_data)
    *(.sram2_data*)
    
    . = ALIGN(4);
    _e_sram2_data = .;        /* 运行地址结束 */
  } >SRAM2 AT> FLASH
  
  /* Uninitialized data section */
  . = ALIGN(4);
  
  .sram2_bss (NOLOAD):
  {
    . = ALIGN(4);
    _s_sram2_bss = .;  /* create a global symbol at data start */
    *(.sram2_bss)      /* .sram2_bss sections */
    *(.sram2_bss*)     /* .sram2_bss* sections */
    . = ALIGN(4);
    _e_sram2_bss = .;  /* define a global symbol at data end */
  } >SRAM2
  
/*SRAM2 Sections End -----------------------------------------------------*/

/*SRAM4 Sections Begin ---------------------------------------------------*/
  _si_sram4_data = LOADADDR(.sram4_data);
  
  .sram4_data :
  {
    . = ALIGN(4);
    _s_sram4_data = .;        /* 运行地址起始 */
    *(.sram4_data)
    *(.sram4_data*)
    
    . = ALIGN(4);
    _e_sram4_data = .;        /* 运行地址结束 */
  } >SRAM4 AT> FLASH
  
  /* Uninitialized data section */
  . = ALIGN(4);
  
  .sram4_bss (NOLOAD):
  {
    . = ALIGN(4);
    _s_sram4_bss = .;  /* create a global symbol at data start */
    *(.sram4_bss)      /* .sram4_bss sections */
    *(.sram4_bss*)     /* .sram4_bss* sections */
    . = ALIGN(4);
    _e_sram4_bss = .;  /* define a global symbol at data end */
  } >SRAM4
  
/*SRAM4 Sections End -----------------------------------------------------*/

/*BKPRAM Sections Begin ---------------------------------------------------*/
  _si_bkpram_data = LOADADDR(.bkpram_data);
  
  .bkpram_data :
  {
    . = ALIGN(4);
    _s_bkpram_data = .;        /* 运行地址起始 */
    *(.bkpram_data)
    *(.bkpram_data*)
    
    . = ALIGN(4);
    _e_bkpram_data = .;        /* 运行地址结束 */
  } >BKPRAM AT> FLASH
  
  /* Uninitialized data section */
  . = ALIGN(4);
  
  .bkpram_bss (NOLOAD):
  {
    . = ALIGN(4);
    _s_bkpram_bss = .;  /* create a global symbol at data start */
    *(.bkpram_bss)      /* .bkpram_bss sections */
    *(.bkpram_bss*)     /* .bkpram_bss* sections */
    . = ALIGN(4);
    _e_bkpram_bss = .;  /* define a global symbol at data end */
  } >BKPRAM
  
/*BKPRAM Sections End -----------------------------------------------------*/

/*.data RAM Sections Begin ---------------------------------------------------*/
  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >DTCMRAM AT> FLASH
/*.data RAM Sections End -----------------------------------------------------*/

  /* Uninitialized data section */
  . = ALIGN(4);
/*.bss RAM Sections Begin ----------------------------------------------------*/
/* .bss段，不从FLASH加载，但遵循C标准，启动文件以汇编实现对此段的全0填充 */
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >DTCMRAM
  
/*.bss RAM Sections End ------------------------------------------------------*/

  /* User_heap_stack section, used to check that there is enough RAM left */
  /* 用户堆栈段，使用时需检查是否有足够的内存余量 */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM
 

/* No Load RAM Sections Begin ---------------------------------------------------*/
/* 此范围的段不从FLASH加载，且启动文件中不进行全0填充 */
  /* dtcmram_nold */
  .dtcmram_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.dtcmram_nold)
    . = ALIGN(4);
  } >DTCMRAM
  
  /* itcmram_nold */
  .itcmram_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.itcmram_nold)
    . = ALIGN(4);
  } >ITCMRAM
  
  /* axiram1_nold */
  .axiram1_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.axiram1_nold)
    . = ALIGN(4);
  } >AXISRAM1
  
  /* axiram2_nold */
  .axiram2_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.axiram2_nold)
    . = ALIGN(4);
  } >AXISRAM2
  
  /* sram1_nold */
  .sram1_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.sram1_nold)
    . = ALIGN(4);
  } >SRAM1
  
  /* sram2_nold */
  .sram2_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.sram2_nold)
    . = ALIGN(4);
  } >SRAM2
  
  /* sram4_nold */
  .sram4_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.sram4_nold)
    . = ALIGN(4);
  } >SRAM4
  
  /* bkpram_nold */
  .bkpram_nold (NOLOAD):
  {
    . = ALIGN(4);
    *(.bkpram_nold)
    . = ALIGN(4);
  } >BKPRAM
  
  /* psram_nold */
  .psram_nold (NOLOAD) :
  {
    . = ALIGN(8);
    *(.psram_nold)
    . = ALIGN(8);
  } >PSRAM
  
/*No Load RAM Sections End -----------------------------------------------------*/

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
