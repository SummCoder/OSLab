; file name：function.asm
; define some useful functions
; 1. strlen(String msg):caluate the length of string
; 2. sprint(String msg): print the string
; 3. sprintLn():print \n
; 4. quit(): exit the process

;------------------------------------------
; int strlen(String message)
strlen:					   ; 返回值保存在EAX中
    push    ebx  		   ; 将EBX中的值保存于栈上，因为strlen会使用该寄存器			
    mov     ebx, eax  	   ; 将EAX中msg的地址移EBX（现在二者指向内存中同一处）
 
nextchar:
    cmp     byte [eax], 0  ; 比较当前EAX地址处的内容是否为字符串结尾'\0'
    jz      finished       ; ZF为1，跳出循环到finished
    inc     eax            ; ZF不为1，EAX中的地址递增
    jmp     nextchar       ; 继续循环
 
finished:
    sub     eax, ebx       ; EBX - EAX，长度保存在EAX中
    pop     ebx            ; 将栈上之前保存的值pop回EBX
    ret                    ; 返回函数调用处
 
 
;------------------------------------------
; void sprint(String message)
; 打印字符串
sprint:
    push    edx			; 将EDX中的值保存于栈上
    push    ecx			; 将ECX中的值保存于栈上
    push    ebx			; 将EBX中的值保存于栈上
    push    eax			; 将EAX中的值保存于栈上，即参数string
    call    strlen		; 计算EAX中字符串长度，保存在EAX中
 
    mov     edx, eax	; 将长度移入到EDX
    pop     eax			; 恢复EAX值，即参数string
 
    mov     ecx, eax	; 将待打印string移入ECX
    mov     ebx, 1		; 表示写入到标准输出STDOUT
    mov     eax, 4		; 调用SYS_WRITE（操作码是4）
    int     80h
 
    pop     ebx			; 恢复原来EBX中的值
    pop     ecx			; 恢复原来ECX中的值
    pop     edx			; 恢复原来EDX中的值
    ret
 
 
;------------------------------------------
; void sprintLn
; 打印换行符
sprintLn:
    push    eax         ; 将EAX中的值保存于栈上
    mov     eax, 0Ah    ; 将换行符0Ah移入EAX
    push    eax         ; 将换行符0Ah入栈，这样可以获取其地址
    mov     eax, esp    ; 将当前栈指针ESP中的地址（指向0Ah）移入EAX
    call    sprint      ; 调用sprint打印换行符
    pop     eax         ; 换行符退栈
    pop     eax         ; 恢复调用该函数前EAX中的值
    ret                 ; 返回调用处


;------------------------------------------
; void quit()
; 退出程序
quit:
    mov     ebx, 0		; exit时返回状态0 - 'No Errors'
    mov     eax, 1		; 调用SYS_EXIT（OPCODE是1）
    int     80h
    ret