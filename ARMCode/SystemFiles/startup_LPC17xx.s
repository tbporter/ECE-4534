/*****************************************************************************/
/* startup_LPC17xx.s: Startup file for LPC17xx device series                 */
/*****************************************************************************/
/* Version: CodeSourcery Sourcery G++ Lite (with CS3)                        */
/*****************************************************************************/


/* Declaration of certain interrupt handlers used by FreeRTOS & the Ethernet Code */
.extern xPortPendSVHandler
.extern xPortSysTickHandler
.extern vPortSVCHandler
.extern vEMAC_ISR
.extern vtSSPIsr
.extern vtI2C0Isr
.extern vtI2C1Isr
.extern vtI2C2Isr
.extern g9UART0Isr
.extern g9UART1Isr
.extern g9UART2Isr
.extern g9UART3Isr
/*
// <h> Stack Configuration
//   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
// </h>
*/
/* NOTE: This is just the stack for main() -- other tasks have their own stacks */
/* MTJ has removed this because it is done in the linker script instead
    .equ    Stack_Size, 0x00000100
    .section ".stack", "w"
    .align  3
    .globl  __cs3_stack_mem
    .globl  __cs3_stack_size
__cs3_stack_mem:
    .if     Stack_Size
    .space  Stack_Size
    .endif
    .size   __cs3_stack_mem,  . - __cs3_stack_mem
    .set    __cs3_stack_size, . - __cs3_stack_mem
*/

/*
// <h> Heap Configuration
//   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
// </h>
*/

/* Note: This is just one heap block.  Another is directly set up in the linker in RAM2 and is 16K */
/*       The heap is for dynamically allocated memory.  It is managed by s_brk() in system_calls.c */
/*       s_brk() is not called by the user, instead, memory is managed for users by malloc() & free() */
/*       which in turn call s_brk() */
/* MTJ has removed this because it is done in the linker script instead
    .equ    Heap_Size,  0x00002000

    .section ".heap", "w"
    .align  3
    .globl  __cs3_heap_start
    .globl  __cs3_heap_end
__cs3_heap_start:
    .if     Heap_Size
    .space  Heap_Size
    .endif
__cs3_heap_end:
*/


/* Vector Table */

    .section ".cs3.interrupt_vector"
    .globl  __cs3_interrupt_vector_cortex_m
    .type   __cs3_interrupt_vector_cortex_m, %object

__cs3_interrupt_vector_cortex_m:
    .long   __cs3_stack                 /* Top of Stack                 */
    .long   __cs3_reset                 /* Reset Handler                */
    .long   NMI_Handler                 /* NMI Handler                  */
    .long   HardFault_Handler           /* Hard Fault Handler           */
    .long   MemManage_Handler           /* MPU Fault Handler            */
    .long   BusFault_Handler            /* Bus Fault Handler            */
    .long   UsageFault_Handler          /* Usage Fault Handler          */
    .long   0                           /* Reserved                     */
    .long   0                           /* Reserved                     */
    .long   0                           /* Reserved                     */
    .long   0                           /* Reserved                     */
    .long   vPortSVCHandler				/* MTJ changed from default SVC_Handler */                /* SVCall Handler               */
    .long   DebugMon_Handler            /* Debug Monitor Handler        */
    .long   0                           /* Reserved                     */
    .long   xPortPendSVHandler			/* MTJ changed from default PendSV_Handler    */          /* PendSV Handler               */
    .long   xPortSysTickHandler			/* MTJ changed from default SysTick_Handler  */            /* SysTick Handler              */

    /* External Interrupts */
    .long   WDT_IRQHandler              /* 16: Watchdog Timer               */
    .long   TIMER0_IRQHandler           /* 17: Timer0                       */
    .long   TIMER1_IRQHandler           /* 18: Timer1                       */
    .long   TIMER2_IRQHandler           /* 19: Timer2                       */
    .long   TIMER3_IRQHandler           /* 20: Timer3                       */
    .long   g9UART0Isr		            /* BLS changed from UART0_IRQHandler 21: UART0                        */
    .long   g9UART1Isr		            /* BLS changed from UART1_IRQHandler 22: UART1                        */
    .long   g9UART2Isr		            /* BLS changed from UART2_IRQHandler 23: UART2                        */
    .long   g9UART3Isr		            /* BLS changed from UART3_IRQHandler 24: UART3                        */
    .long   PWM1_IRQHandler             /* 25: PWM1                         */
    .long   vtI2C0Isr      		       /* MTJ changed from default I2C0_IRQHandler 26: I2C0                         */
    .long   vtI2C1Isr      		       /* MTJ changed from default I2C1_IRQHandler             /* 27: I2C1                         */
    .long   vtI2C2Isr      		       /* MTJ changed from default I2C2_IRQHandler             /* 28: I2C2                         */
    .long   SPI_IRQHandler              /* 29: SPI                          */
    .long   SSP0_IRQHandler             /* 30: SSP0                         */
    .long   vtSSPIsr					/* MTJ changed from default SSP1_IRQHandler             /* 31: SSP1                         */
    .long   PLL0_IRQHandler             /* 32: PLL0 Lock (Main PLL)         */
    .long   RTC_IRQHandler              /* 33: Real Time Clock              */
    .long   EINT0_IRQHandler            /* 34: External Interrupt 0         */
    .long   EINT1_IRQHandler            /* 35: External Interrupt 1         */
    .long   EINT2_IRQHandler            /* 36: External Interrupt 2         */
    .long   EINT3_IRQHandler            /* 37: External Interrupt 3         */
    .long   ADC_IRQHandler              /* 38: A/D Converter                */
    .long   BOD_IRQHandler              /* 39: Brown-Out Detect             */
    .long   USB_IRQHandler              /* 40: USB                          */
    .long   CAN_IRQHandler              /* 41: CAN                          */
    .long   DMA_IRQHandler              /* 42: General Purpose DMA          */
    .long   I2S_IRQHandler              /* 43: I2S                          */
    .long   vEMAC_ISR					/* MTJ changed from default ENET_IRQHandler  */           /* 44: Ethernet                     */
    .long   RIT_IRQHandler              /* 45: Repetitive Interrupt Timer   */
    .long   MCPWM_IRQHandler            /* 46: Motor Control PWM            */
    .long   QEI_IRQHandler              /* 47: Quadrature Encoder Interface */
    .long   PLL1_IRQHandler             /* 48: PLL1 Lock (USB PLL)          */
    .long	USBActivity_IRQHandler		/* 49: USB Activity 				*/
    .long 	CANActivity_IRQHandler		/* 50: CAN Activity					*/

    .size   __cs3_interrupt_vector_cortex_m, . - __cs3_interrupt_vector_cortex_m


    .thumb

/* Reset Handler */

    .section .cs3.reset,"x",%progbits
    .thumb_func
    .globl  __cs3_reset_cortex_m
    .type   __cs3_reset_cortex_m, %function
__cs3_reset_cortex_m:
    .fnstart
.if (RAM_MODE)
/* Clear .bss section (Zero init) */
	MOV     R0, #0
	LDR     R1, =__bss_start__
	LDR     R2, =__bss_end__
	CMP     R1,R2
	BEQ     BSSIsEmpty
LoopZI:
	CMP     R1, R2
	BHS		BSSIsEmpty
	STR   	R0, [R1]
	ADD		R1, #4
	BLO     LoopZI
BSSIsEmpty:
    LDR     R0, =SystemInit
    BLX     R0
    LDR     R0,=main
    BX      R0
.else
    LDR     R0, =SystemInit
    BLX     R0
	LDR     R0,=_start
    BX      R0
.endif
    .pool
    .cantunwind
    .fnend
    .size   __cs3_reset_cortex_m,.-__cs3_reset_cortex_m

    .section ".text"

/* Exception Handlers */

    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
    B       .
    .size   NMI_Handler, . - NMI_Handler

    .weak   HardFault_Handler
    .type   HardFault_Handler, %function
HardFault_Handler:
    B       .
    .size   HardFault_Handler, . - HardFault_Handler

    .weak   MemManage_Handler
    .type   MemManage_Handler, %function
MemManage_Handler:
    B       .
    .size   MemManage_Handler, . - MemManage_Handler

    .weak   BusFault_Handler
    .type   BusFault_Handler, %function
BusFault_Handler:
    B       .
    .size   BusFault_Handler, . - BusFault_Handler

    .weak   UsageFault_Handler
    .type   UsageFault_Handler, %function
UsageFault_Handler:
    B       .
    .size   UsageFault_Handler, . - UsageFault_Handler

    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    B       .
    .size   SVC_Handler, . - SVC_Handler

    .weak   DebugMon_Handler
    .type   DebugMon_Handler, %function
DebugMon_Handler:
    B       .
    .size   DebugMon_Handler, . - DebugMon_Handler

    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    B       .
    .size   PendSV_Handler, . - PendSV_Handler

    .weak   SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    B       .
    .size   SysTick_Handler, . - SysTick_Handler


/* IRQ Handlers */

    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    B       .
    .size   Default_Handler, . - Default_Handler

    .macro  IRQ handler
    .weak   \handler
    .set    \handler, Default_Handler
    .endm

    IRQ     WDT_IRQHandler
    IRQ     TIMER0_IRQHandler
    IRQ     TIMER1_IRQHandler
    IRQ     TIMER2_IRQHandler
    IRQ     TIMER3_IRQHandler
    IRQ     UART0_IRQHandler
    IRQ     UART1_IRQHandler
    IRQ     UART2_IRQHandler
    IRQ     UART3_IRQHandler
    IRQ     PWM1_IRQHandler
    IRQ     I2C0_IRQHandler
    IRQ     I2C1_IRQHandler
    IRQ     I2C2_IRQHandler
    IRQ     SPI_IRQHandler
    IRQ     SSP0_IRQHandler
    IRQ     SSP1_IRQHandler
    IRQ     PLL0_IRQHandler
    IRQ     RTC_IRQHandler
    IRQ     EINT0_IRQHandler
    IRQ     EINT1_IRQHandler
    IRQ     EINT2_IRQHandler
    IRQ     EINT3_IRQHandler
    IRQ     ADC_IRQHandler
    IRQ     BOD_IRQHandler
    IRQ     USB_IRQHandler
    IRQ     CAN_IRQHandler
    IRQ     DMA_IRQHandler
    IRQ     I2S_IRQHandler
    IRQ     ENET_IRQHandler
    IRQ     RIT_IRQHandler
    IRQ     MCPWM_IRQHandler
    IRQ     QEI_IRQHandler
    IRQ     PLL1_IRQHandler
    IRQ		USBActivity_IRQHandler
    IRQ		CANActivity_IRQHandler

    .end
