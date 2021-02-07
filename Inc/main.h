
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>


// Task control block
typedef struct
{
	uint32_t pspVal;
	uint32_t blockCount;
	uint8_t  currentState;
	void (*pTaskHandler)(void);

} TCB_t;


#define TASK_STATE_BLOCKED 0U
#define TASK_STATE_READY   1U

// ARM Stack: Full Descending
// Each stack will have a stack size of 1KB
// The scheduler will also have a stack size of 1KB
#define SIZE_TASK_STACK     1024U
#define SIZE_SCHED_STACK	1024U

#define MAX_TASKS			5U

#define SRAM1_START_ADD		0x20000000U
#define SRAM1_SIZE			(96U * 1024U)
#define SRAM1_END_ADD		(SRAM1_START_ADD + SRAM1_SIZE)

#define T_IDLE_STACK_START	(SRAM1_END_ADD)
#define T1_STACK_START  	((T_IDLE_STACK_START) - (SIZE_TASK_STACK))
#define T2_STACK_START		((T1_STACK_START) - (SIZE_TASK_STACK))
#define T3_STACK_START		((T2_STACK_START) - (SIZE_TASK_STACK))
#define T4_STACK_START		((T3_STACK_START) - (SIZE_TASK_STACK))
#define SCHED_STACK_START	((T4_STACK_START) - (SIZE_TASK_STACK))


#define MSI_DEFAULT_FREQ 	4000000U
#define SYS_CLK_FREQ		(MSI_DEFAULT_FREQ)

#define TICK_HZ 			1000U



#define INTERRUPT_ENABLE()		do{__asm volatile("MOV R0, #0x01"); __asm volatile("MSR PRIMASK, R0");}while(0)
#define INTERRUPT_DISABLE()		do{__asm volatile("MOV R0, #0x00"); __asm volatile("MSR PRIMASK, R0");}while(0)

// --–----------------------- DUMMY STACK VALUES ------------------------------

#define DUMMY_xPSR	0x01000000		// t-bit is set


#endif // __MAIN_H
