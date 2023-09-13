global	my_print

	section .text
my_print:
        push    ebp             ;保存上一层的栈底
        mov     ebp, esp
        ; 栈结构（地址由高到低依次为，实参n，实参n-1 ...实参1，return地址，上一层栈底（此时ebp正好指向此处），局部变量...
        ; my_print(s, strlen(s));实参1为s，地址为ebp+8，实参2为len(s)，地址为ebp+8
        mov     edx, [ebp+12]   ;the length
        mov     ecx, [ebp+8]    ;the string
        mov     ebx, 1
        mov     eax, 4
        int     80h
        pop     ebp
        ret