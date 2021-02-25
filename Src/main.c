/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
#include "main.h"
#include "tasks.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

uint8_t  g_currentTask = 1;
uint32_t g_tickCount;
TCB_t g_taskTCB[MAX_TASKS];


void taskDelay(uint32_t delayTicks)
{
	if(delayTicks > 0)
	{
		INTERRUPT_DISABLE(); // disable interrupts to ensure sequential operations of following instructions


		g_taskTCB[g_currentTask].currentState = TASK_STATE_BLOCKED;
		g_taskTCB[g_currentTask].blockCount = g_tickCount + delayTicks; // wrap around may occur here, but this is OK.
		// enable interrupts again


		// Set PendSV bit to allow for other tasks to execute while this is blocked
		uint32_t *pICSR = ((uint32_t *)0xE000ED04); // interrupt control and state register
		*pICSR |= (1 << 28);

		INTERRUPT_ENABLE(); // re-enable interrupts
	}
}


// Changes MSP to scheduler task stack start address
__attribute__((naked)) void initSchedulerStack(uint32_t schedTaskStackAddr)
{
	__asm volatile("MSR MSP, %0" : : "r"(schedTaskStackAddr) : );
	__asm volatile("BX LR");
}

void initSysTick(uint32_t tickFreq)
{
	volatile uint32_t *pSCSR = ((uint32_t *)(0xE000E010));	// Systick Control and Status Register
	volatile uint32_t *pSRVR = ((uint32_t *)(0xE000E014));  // Systick Reload Value Register

	uint32_t reload = (SYS_CLK_FREQ / tickFreq) - 1; 		// value where the countdown begins
	*pSRVR &= ~(0x00FFFFFF); 								// Clear pSRVR
	*pSRVR |= (reload & 0x00FFFFFF); 						// 24 bit timer

	*pSCSR |= 0x6;                     // enable systick exceptions and set systick clock source
	*pSCSR |= 0x1; 					   // enable systick couner
}

void initTasksStack()
{
	// store address of task handlers as a uint32_t
	g_taskTCB[0].pTaskHandler = taskIdle_handler;
 	g_taskTCB[1].pTaskHandler = task1_handler;
 	g_taskTCB[2].pTaskHandler = task2_handler;
 	g_taskTCB[3].pTaskHandler = task3_handler;
 	g_taskTCB[4].pTaskHandler = task4_handler;

 	g_taskTCB[0].pspVal = T_IDLE_STACK_START;
 	g_taskTCB[1].pspVal = T1_STACK_START;
 	g_taskTCB[2].pspVal = T2_STACK_START;
 	g_taskTCB[3].pspVal = T3_STACK_START;
 	g_taskTCB[4].pspVal = T4_STACK_START;


	for(int taskIndx = 0; taskIndx < MAX_TASKS; taskIndx++)
	{
		g_taskTCB[taskIndx].currentState = TASK_STATE_READY;
		uint32_t *pPSP = ((uint32_t *)g_taskTCB[taskIndx].pspVal); // cast the value into a pointer type

		// 1. Store dummy stack frame #1

		pPSP--;
		*pPSP = DUMMY_xPSR;              // xPSR

		pPSP--;
		*pPSP = ((uint32_t)g_taskTCB[taskIndx].pTaskHandler); // PC

		pPSP--;
		*pPSP = 0xFFFFFFFD; 			 // LR with EXC_RETURN value

		// 2. Store dummy stack frame #2
		// GP register R0 - R12
		for(int i=0; i < 13; i++)
		{
			pPSP--;
			*pPSP = 0;
		}

		g_taskTCB[taskIndx].pspVal = (uint32_t)pPSP; 	// update PSP value
	}
}

uint32_t getPSPValue()
{
	return g_taskTCB[g_currentTask].pspVal;
}
void savePSPValue(uint32_t val)
{
	g_taskTCB[g_currentTask].pspVal = val;
}

void updateNextTask(void)
{
	int state = 0;

	for(int i=0; i < MAX_TASKS; i++)
	{
		g_currentTask++;
		g_currentTask %= MAX_TASKS;
		state = g_taskTCB[g_currentTask].currentState;
		if(g_currentTask != 0 && state == TASK_STATE_READY)
			break;
	}
	if(state != TASK_STATE_READY)
	{
		g_currentTask = 0;
	}
}

__attribute__((naked)) void changeSP()
{
	// 1. Initialize PSP to Task 1 address
	__asm volatile("PUSH {LR}"); 		// preserve LR to go back to main (step required since naked function does not do automatically)
	__asm volatile("BL getPSPValue");
	__asm volatile("MSR PSP, R0");
	__asm volatile("POP {LR}");         // safe to bring back LR from stack since no other function calls

	// 2. Change SP to PSP in CONTROL register
	__asm volatile ("MOV R0, #0x02");
	__asm volatile ("MSR CONTROL, R0");
	__asm volatile("BX LR");
}

void enableProcessorFaults()
{
	uint32_t *pSHCSR = (uint32_t *)0xE000ED24;
	*pSHCSR |= (1 << 16); // mem manage
	*pSHCSR |= (1 << 17); // bus fault
	*pSHCSR |= (1 << 18); // usage fault
}


int main(void)
{

 	enableProcessorFaults();
 	initSchedulerStack(SCHED_STACK_START);


	// store dummy stack frames when task not initialized
	// GP registers are fine with 0
	// xPSR: ensure T bit is set to 1 -> thumb instructions
	// PC:   address of task handler
	// LR:   special return: EXC_RETURN (exception return)
		// Possible values for EXC_RETURN
			// 0xFFFFFFF1 - Return to handler mode (nope)
			// 0xFFFFFFF9 - Return to thread mode, use MSP
			// ((((((0xFFFFFFFD - Return to thread mode, use PSP)))))))
			// 0xFFFFFFE1 - For processors with floating point engine
			// 0xFFFFFFE9 - For processors with floating point engine
			// 0xFFFFFFED - For processors with floating point engine
	initTasksStack();

	initSysTick(TICK_HZ);

	// change from MSP to pointer to PSP
	changeSP();

	task1_handler();

    /* Loop forever */
	for(;;);
}

/*
 * Decided task to execute next and execute.
 */
__attribute((naked)) void PendSV_Handler(void)
{
	/* Save the context of current task */
	// 1. Get current tasks PSP value
	__asm volatile("MRS R0, PSP");
	// 2. Using PSP value, store stack frame 2 (R4 to R11)
	__asm volatile("STMDB R0!, {R4-R11}");


	// 3. Save PSP value
	__asm volatile("PUSH {LR}");
	__asm volatile("BL savePSPValue");

	/* Retrieve context of next task */
	// 1. Decide next task to run
	__asm volatile("BL updateNextTask");
	// 2. Get the task's PSP value
	__asm volatile("BL getPSPValue");
	// 3. Retrieve SF2
	__asm volatile("LDMIA R0!, {R4-R11}");
	// 4. Update PSP
	__asm volatile("MSR PSP, R0");

	// 5. Trigger the exit sequence
	__asm volatile("POP {LR}");
	__asm volatile("BX LR");
}

/*
 * Updates global tick count.
 * Checks to see whether blocked tasks are ready to run by comparing with the global
 * tick count.
 * Pends the PendSV bit to trigger PendSV exception.
 */
void SysTick_Handler(void)
{
	g_tickCount++;

	// TODO: could be made more efficient if only call PendSV if there is a state change?

	// unblock tasks whose delay time is achieved
	for(int taskIndx = 1; taskIndx < MAX_TASKS; taskIndx++)
	{
		if(g_taskTCB[taskIndx].currentState == TASK_STATE_BLOCKED)
		{
			if(g_taskTCB[taskIndx].blockCount == g_tickCount)
			{
				g_taskTCB[taskIndx].currentState = TASK_STATE_READY;
			}
		}
	}


	// Pend sv to trigger next task
	uint32_t *pICSR = ((uint32_t *)0xE000ED04); // interrupt control and state register
	*pICSR |= (1 << 28);
}

void HardFault_Handler(void)
{
	while(1);
}
void MemManage_Handler(void)
{
	while(1);
}
void BusFault_Handler(void)
{
	while(1);
}
