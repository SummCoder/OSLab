; file name:main.asm
; nasm -f elf main.asm
; ld -m elf_i386 main.o -o main
; ./main
; BigIntDiv

%include    'function.asm'

SECTION .data
msg1:    db  'Please input two numbers', 0Ah, 0h ;inform the user to input two numbers
msg2:    db  'The quotient is:', 0Ah, 0h ;tell the user next output is quotient
msg3:    db  'The remainder is:', 0Ah, 0h 
ZERO:    db  '0', 0Ah, 0h
error_msg:   db  'Error!', 0Ah, 0h
dividend:   times   102     dd  0
divisor:    times   102     dd  0
quotient:   times   102     dd  0

SECTION .bss
input1      resb    102 ;dividend
input2      resb    102 ;divisor
len1    resd    1
len2    resd    1
judge   resd    1
index   resd    1
printout    resb    1


SECTION .text
global  _start

; main() 
_start:
; print msg1
    mov     eax, msg1
    call    sprint
; input the two numbers
    mov     eax, 3
    mov     ebx, 0
    mov     ecx, input1
    mov     edx, 102
    int     80h
    mov     eax, 3
    mov     ebx, 0
    mov     ecx, input2
    mov     edx, 102
    int     80h
; get the length of the two num
    mov     eax, input1
    call    strlen
    sub     eax, 1
    mov     dword[len1], eax
    mov     eax, input2
    call    strlen
    sub     eax, 1
    mov     dword[len2], eax
; use list to store the two numbers,inverted order
    call    store
; two simple case
; first:divisor == 0
    xor     eax, eax
    mov     al, byte[input2]
    cmp     al, '0'
    je      error
; second:len1 < len2
    xor     eax, eax
    mov     eax, dword[len1]
    mov     ebx, dword[len2]
    cmp     eax, ebx
    jb      case1
; the most complex part
; do BigIntDiv
    jmp     case2
; make sure exit
    call    quit





; ------------------------------
; functions and loop
; ------------------------------


; dividend[i] = input1[len1-1-i] - '0';
; divisor[i] = input2[len2-1-i] - '0';
store:
    mov     ecx, 0  ; use ecx to count
    
.loop1:
    cmp     ecx, dword[len1]
    je      .L1
    mov     ebx, dword[len1]
    sub     ebx, ecx
    sub     ebx, 1      
    movzx   eax, byte[input1+ebx]
    sub     eax, '0'
    xor     ebx, ebx
    mov     dword[dividend+ecx*4], eax
    inc     ecx
    jmp     .loop1

.L1:
    mov     ecx, 0

.loop2:
    cmp     ecx, dword[len2]
    je      .L2
    mov     ebx, dword[len2]
    sub     ebx, ecx
    sub     ebx, 1
    movzx   eax, byte[input2+ebx]
    sub     eax, '0'
    xor     ebx, ebx
    mov     dword[divisor+ecx*4], eax
    inc     ecx
    jmp     .loop2

.L2:
    ret


; -------------------------------
; error
error:
    mov     eax, error_msg
    call    sprint
    call    quit


; --------------------------------
; a simple case
case1:
    mov     eax, msg2
    call    sprint
    mov     eax, ZERO
    call    sprint
    mov     eax, msg3
    call    sprint
    mov     eax, input1
    call    sprint
    call    quit

; --------------------------------
; BigIntDiv
case2:
    mov     eax, dword[len1]
    sub     eax, dword[len2]
    mov     ecx, eax    ;use ecx to represent i

.loop3:
    cmp     ecx, 0
    jl      output
    push    esi     ;use esi as j


; while(1)
.loop4:
    mov     dword[judge], 1
    mov     esi, dword[len2]
    sub     esi, 1


.loop5:
    cmp     esi, 0
    jl      .L4
    mov     eax, dword[len2]
    add     eax, ecx
    cmp     dword[dividend+4*eax], 0
    jg      .L4
    xor     eax, eax
    mov     eax, ecx
    add     eax, esi
    mov     ebx, dword[divisor+4*esi]
    cmp     dword[dividend+4*eax], ebx    
    jg      .L4
    jl      .L3
    dec     esi
    jmp     .loop5

.L3:
    mov     dword[judge], 0

.L4:
    cmp     dword[judge], 0
    je      .L6
    mov     esi, 0

.loop6:
    cmp     esi, dword[len2]
    je      .L5
    mov     eax, ecx
    add     eax, esi
    mov     ebx, dword[divisor+4*esi]
    sub     dword[dividend+4*eax], ebx
    cmp     dword[dividend+4*eax], 0
    jl     .L7
    inc     esi
    jmp     .loop6


.L5:
    add     dword[quotient+4*ecx], 1
    jmp     .loop4

.L6:
    dec     ecx
    jmp     .loop3

.L7:
    add     dword[dividend+4*eax], 10
    add     eax, 1
    sub     dword[dividend+4*eax], 1
    inc     esi
    jmp     .loop6



; output the two result
output:
    pop     esi
    mov     dword[index], 0
    mov     ecx, dword[len1]
    sub     ecx, dword[len2]

.loop7:
    cmp     ecx, 0
    jl      .L8
    cmp     dword[quotient+4*ecx], 0
    jne     .L8
    dec     ecx
    jmp     .loop7

.L8:
    mov     dword[index], ecx
    mov     ecx, dword[index]
    mov     eax, msg2
    call    sprint

.loop8:
    cmp     ecx, 0
    jl      .L9
    mov     eax, dword[quotient+4*ecx]
    and     eax, 0fh
    add     eax, '0'
    mov     byte[printout], al
    push    ecx
    mov     edx, 1
    mov     ecx, printout
    mov     ebx, 1
    mov     eax, 4
    int     80h
    pop     ecx
    dec     ecx
    jmp     .loop8


.L9:
    cmp     dword[index], -1
    je      .small_solve0
    call    sprintLn
.L11:
    mov     dword[index], 0
    mov     ecx, dword[len2]
    sub     ecx, 1

.loop9:
    cmp     ecx, 0
    jl      .L10
    cmp     dword[dividend+4*ecx], 0
    jne     .L10
    dec     ecx
    jmp     .loop9


.L10:
    mov     dword[index], ecx
    mov     ecx, dword[index]
    mov     eax, msg3
    call    sprint

.loop10:
    cmp     ecx, 0
    jl      .end
    mov     eax, dword[dividend+4*ecx]
    and     eax, 0fh
    add     eax, '0'
    mov     byte[printout], al
    push    ecx
    mov     edx, 1
    mov     ecx, printout
    mov     ebx, 1
    mov     eax, 4
    int     80h
    pop     ecx
    dec     ecx
    jmp     .loop10

.end:
    cmp     dword[index], -1
    je      .small_solve
    call    sprintLn
    call    quit

; quotient = 0
.small_solve0:
    mov     eax, ZERO
    call    sprint
    jmp     .L11

; remainder = 0
.small_solve:
    mov     eax, ZERO
    call    sprint
    call    quit