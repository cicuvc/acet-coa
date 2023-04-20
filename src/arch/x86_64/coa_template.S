.equ CTL_CONT_OFFSET, 0x10
.equ CTL_SP_OFFSET, 0x10
.equ CTL_SSIZE_OFFSET,0x18
.equ CTL_ESIZE_OFFSET,0x1c
.equ CTL_EFRAME_OFFSET,0x8
.equ CTL_SIZE, 0x20
.equ CTL_PROC_OFFSET, 0x0


.equ CIF_FSIZE_OFFSET, 0x10
.equ CIF_YIELD_OFFSET, 0x0
.equ CIF_RESUME_OFFSET, 0x8


.equ JIT_CMD_RELOC, 0x01
.equ JIT_CMD_LABEL, 0x02
.equ JIT_XCHG_NVS, 0x03
.equ JIT_CMD_END, 0x05
.equ JIT_CMD_RELOC_S, 0x06
.equ JIT_CMD_RELOC_L, 0xa
.equ JIT_CMD_FSIZE_RW, 0xb
.equ JMP_OFFSET, 0x40000000
.macro jit_cmd cmd
     nopw \cmd(%rax)
.endm


.global LCotaskEnter
.text
LCotaskEnter:

    pop %rax
    pop %rcx
    mov CTL_EFRAME_OFFSET(%rcx), %rsp
    mov %rsp, CTL_SP_OFFSET(%rcx)
    jmp *%rax


.global cotask_probe_frame_size
.text
cotask_probe_frame_size:
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    push %rdi
    mov %rsp, CTL_EFRAME_OFFSET(%rdi)
    movl $-0x1, CTL_ESIZE_OFFSET(%rdi)
    lea .LProbeContinuation(%rip), %rax
    mov %rax, CTL_CONT_OFFSET(%rdi)
    call *%rsi
.LProbeContinuation:
    pop %rdi
    pop %rdi
    mov CTL_EFRAME_OFFSET(%rdi),%rax
    mov %rax, %rcx
    sub %rsp, %rax
    add %rax, %rsp
    pop %rdi
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    ret

.global cotask_probe_stack_structure
.text
cotask_probe_stack_structure:
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    lea 0x1(%rdx), %rbx
    lea 0x2(%rdx), %rbp
    lea 0x3(%rdx), %r12
    lea 0x4(%rdx), %r13
    lea 0x5(%rdx), %r14
    lea 0x6(%rdx), %r15

    sub $CTL_SIZE, %rsp
    lea .LProbeStructContinuation(%rip), %rax
    mov %rax, CTL_CONT_OFFSET(%rsp)
    movl $-0x1, CTL_ESIZE_OFFSET(%rsp)
    xchg %rsp, %rdi
    mov %rsi, %rax
    xor %rsi, %rsi
    xor %rcx, %rcx
    xor %rdx, %rdx
    xor %r8, %r8
    xor %r9, %r9
    jmp *%rax

.LProbeStructContinuation:
    pop %rdi
    pop %rdi
    lea CTL_SIZE(%rdi),%rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    ret


    

.global cotask_yield_template
.text
cotask_yield_template:
    mov CTL_SP_OFFSET(%rdi), %r8
    pop CTL_SP_OFFSET(%rdi)
    cmp %rsp, %r8
    jne .LSaveExtFrame
    jit_cmd JIT_CMD_RELOC_S
jit_cmd JIT_CMD_LABEL
.LSaveExtComplete:
    lea 0x10000000(%r8), %rsp
    jit_cmd JIT_CMD_FSIZE_RW

    mov %rbp, %rdx
    jit_cmd JIT_XCHG_NVS

    mov %rdi, %rax
    jmp *0x8(%rdx)

jit_cmd JIT_CMD_LABEL
.LSaveExtFrame:

    mov %r8, %rdx
    sub %rsp, %rdx

    push %r8
    push %rsi
    lea 0x10(%rsp), %rsi

    call cotask_save_eframe
    jit_cmd JIT_CMD_RELOC
    mov %rax, %rdi

    pop %rsi
    pop %r8
    jmp .LSaveExtComplete
    jit_cmd JIT_CMD_RELOC_S
jit_cmd JIT_CMD_END

.global cotask_resume_template
.text
cotask_resume_template:
    pop %rax
    movl $0x0, %ecx
    jit_cmd JIT_CMD_FSIZE_RW
    movl CTL_SSIZE_OFFSET(%rdi), %edx
    mov CTL_CONT_OFFSET(%rdi), %r11
    sub %rcx, %rsp
    mov %rsp, CTL_CONT_OFFSET(%rdi)
    sub %rdx, %rsp

    lea (CTL_SIZE - 0x10)(%rdi, %rcx, 1), %rdx
    mov %rax, 0x8(%rdx)
    jit_cmd JIT_XCHG_NVS

    movl CTL_SSIZE_OFFSET(%rdi), %edx

    test %edx, %edx
    jnz .LCopyExtFrame
    jit_cmd JIT_CMD_RELOC_S
    jmp *%r11

jit_cmd JIT_CMD_LABEL
.LCopyExtFrame:
    mov CTL_EFRAME_OFFSET(%rdi), %rsi
    push %r11
    lea 0x8(%rsp), %rdi

    call memcpy
    jit_cmd JIT_CMD_RELOC
    ret
jit_cmd JIT_CMD_END