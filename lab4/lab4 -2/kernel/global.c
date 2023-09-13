
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

// 声明进程表
PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

PUBLIC	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"}};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
					{ReporterA, STACK_SIZE_TESTA, "ReporterA"},
					{ProducerB, STACK_SIZE_TESTB, "ProducerB"},
					{ProducerC, STACK_SIZE_TESTC, "ProducerC"},
                                        {ConsumerD, STACK_SIZE_TESTD, "ConsumerD"},
                                        {ConsumerE, STACK_SIZE_TESTE, "ConsumerE"},
                                        {ConsumerF, STACK_SIZE_TESTF, "ConsumerF"}};

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {
        sys_get_ticks,
        sys_write_str,
        sys_sleep,
        p_process,
        v_process
};


PUBLIC  SEMAPHORE sp = {2, 0, 0};
PUBLIC  SEMAPHORE sg1 = {1, 0, 0};
PUBLIC  SEMAPHORE sg2 = {1, 0, 0};