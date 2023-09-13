
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


PRIVATE void init_tasks()
{
	init_screen(tty_table);
	clean(console_table);

	// 表驱动，对应进程0, 1, 2, 3, 4, 5, 6
	int prior[7] = {1, 1, 1, 1, 1, 1, 1};
	for (int i = 0; i < 7; ++i) {
        proc_table[i].ticks    = prior[i];
        proc_table[i].priority = prior[i];
	}

	// initialization
	k_reenter = 0;
	ticks = 0;

	cnt1 = 0;
    cnt2 = 0;
    cnt3 = 0;
    cnt4 = 0;
    cnt5 = 0;
    sp.value = 5;
    sg1.value = 0;
    sg2.value = 0;
	p_proc_ready = proc_table;
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
    u8              privilege;
    u8              rpl;
    int             eflags;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
        if (i < NR_TASKS) {     /* 任务 */
            p_task    = task_table + i;
            privilege = PRIVILEGE_TASK;
            rpl       = RPL_TASK;
            eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
        }else{                  /* 用户进程 */
            p_task    = user_proc_table + (i - NR_TASKS);
            privilege = PRIVILEGE_USER;
            rpl       = RPL_USER;
            eflags    = 0x202; /* IF=1, bit 2 is always 1 */
        }        
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid
		p_proc->sleeping = 0; // 初始化结构体新增成员
		p_proc->blocked = 0;
        p_proc->issleep = 0;
		
		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	init_tasks();
	init_clock();
    init_keyboard();
	restart();

	while(1){}
}

PRIVATE produce_proc(int slices){
    sleep_ms(slices * TIME_SLICE,0); // 耗时slices个时间片
}

PRIVATE	consume_proc(int slices){
    sleep_ms(slices * TIME_SLICE,0); // 耗时slices个时间片
}

void produce1(int slices){
    P(&sp);
    produce_proc(slices);
    cnt1++;
    V(&sg1);
}

void produce2(int slices){
    P(&sp);
    produce_proc(slices);
    cnt2++;
    V(&sg2);
}

void consume1(int slices){
    P(&sg1);
    consume_proc(slices);
    cnt3++;
    V(&sp);
}

void consume2(char proc, int slices){
    P(&sg2);
    consume_proc(slices);
    if(proc == 'E'){
        cnt4++;
    }else{
        cnt5++;
    }
    V(&sp);
}

/*======================================================================*
                               Reporter
 *======================================================================*/
void ReporterA()
{
    sleep_ms(TIME_SLICE,0);
    char white = '\06';
    int time = 1;
    while (1) {
        if (time < 21){
            if (time < 10){
				printf(" %c%c%c ", white, time + '0', ':');
			}else{
				int temp1 = time / 10;
				int temp2 = time - temp1 * 10;
                printf("%c%c%c%c ", white, temp1 + '0', temp2 + '0', ':');
			}
            int temp1;
            int temp2;
            temp1 = cnt1 / 10;
            if(cnt1 >= 10){
                temp2 = cnt1 - temp1 * 10;
                printf("%c%c ", temp1 + '0', temp2 + '0');
            }else{
                printf(" %c ", cnt1 + '0');
            }
            temp1 = cnt2 / 10;
            if(cnt2 >= 10){
                temp2 = cnt2 - temp1 * 10;
                printf("%c%c ", temp1 + '0', temp2 + '0');
            }else{
                printf(" %c ", cnt2 + '0');
            }
            temp1 = cnt3 / 10;
            if(cnt3 >= 10){
                temp2 = cnt3 - temp1 * 10;
                printf("%c%c ", temp1 + '0', temp2 + '0');
            }else{
                printf(" %c ", cnt3 + '0');
            }
            temp1 = cnt4 / 10;
            if(cnt4 >= 10){
                temp2 = cnt4 - temp1 * 10;
                printf("%c%c ", temp1 + '0', temp2 + '0');
            }else{
                printf(" %c ", cnt4 + '0');
            }
            temp1 = cnt5 / 10;
            if(cnt5 >= 10){
                temp2 = cnt5 - temp1 * 10;
                printf("%c%c ", temp1 + '0', temp2 + '0');
            }else{
                printf(" %c ", cnt5 + '0');
            }
            printf("\n");
        }
        sleep_ms(TIME_SLICE,0);
        time++;
    }
}

void ProducerB()
{
	while(1){
		produce1(1);
        sleep_ms(1 * TIME_SLICE,1);
	}
}

void ProducerC()
{
	while(1){
		produce2(1);
        sleep_ms(1 * TIME_SLICE,1);
	}
}

void ConsumerD()
{
    while(1){
        consume1(1);
        sleep_ms(1 * TIME_SLICE,1);
    }
}

void ConsumerE()
{
	while(1){
		consume2('E', 1);
        sleep_ms(1 * TIME_SLICE,1);
	}
}

void ConsumerF()
{
    while(1){
        consume2('F', 1);
        sleep_ms(1 * TIME_SLICE,1);
    }
}