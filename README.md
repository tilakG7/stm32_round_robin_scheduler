# stm32_round_robin_scheduler

A round robin scheduler: tasks have equal priority. 

Tasks have the ability to block for a number of ticks as determined by the Systick timer.



Features:
* Systick timer configured to interrupt every 1ms
* Systick handler frees blocked tasks if they have reached target ticks
* Systick handler enables the PendSV exception
* PendSV handler executes the context switch: acheived by changing value of stack pointer to next task to be run
