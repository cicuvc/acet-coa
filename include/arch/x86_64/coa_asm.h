#ifndef _H_COA_ARCH_X86_64_ASM
#define _H_COA_ARCH_X86_64_ASM

asm(
    ".macro cotask_prolog name size\n"

    ".global \\name\n"
    ".text\n"
    "\\name:\n"
    "   sub %rsp, %rdi\n"
    "   push %rsi\n"
    "   push %rdx\n"
    "   push %rcx\n"
    "   push %r8\n"
    "   push %r9\n"
    "   sub $0x10, %rsp\n"

    "   lea \\name\\()_info(%rip), %rsi\n"
    "   lea coimpl_\\name(%rip), %rdx\n"
    "   lea 0x38(%rsp), %rcx\n"
    "   lea (%rsp), %r8\n"

    "   call cotask_init\n"
    "   mov %rax, %rdi\n"
    "   add $0x10, %rsp\n"
    "   pop %r9\n"
    "   pop %r8\n"
    "   pop %rcx\n"
    "   pop %rdx\n"
    "   pop %rsi\n"
    "   mov -0x38(%rsp), %rsp\n"

    "   jmp coimpl_\\name\n"

    "\\name\\()_yield_code:\n"
    "   .zero \\size\n"
    "\\name\\()_resume_code:\n"
    "   .zero \\size\n"

    ".endm"
);

#define cotask_start(call) ({ void *_; asm volatile("mov %%rsp, %0":"=r"(_)::"cc"); call;})

#define cotask_entry_asm(ctx) \
    do { \
        asm volatile("push %0; call *CTL_CONT_OFFSET(%0);"::"r"(ctx) : "rax", "rcx", "rdx");\
    } while(0)

#endif