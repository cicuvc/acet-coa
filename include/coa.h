#ifndef _H_COROUTINE_COA
#define _H_COROUTINE_COA

asm(".equ CTL_CONT_OFFSET, 0x10");
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

#define tostring(...) #__VA_ARGS__
#define tostring_defer(...) tostring(__VA_ARGS__)


#define def_cotask(name, ...) \
    cotask_ctrl *name(cotask_ctrl *, __VA_ARGS__); \
    asm(tostring_defer(cotask_prolog name, COA_JIT_MEMORY_SIZE)); \
    extern char name##_yield_code[COA_JIT_MEMORY_SIZE]; \
    extern char name##_resume_code[COA_JIT_MEMORY_SIZE]; \
    cotask_info name##_info = { (void *)name##_yield_code,(void *)name##_resume_code,0 }; \
    cotask_ctrl *coimpl_##name(cotask_ctrl *ctx, __VA_ARGS__)


#define cotask_entry(name) \
    if(ctx->ext_size < 0) { \
        asm volatile("push %0; call *CTL_CONT_OFFSET(%0);"::"r"(ctx) : "rax", "rcx", "rdx"); \
        ctx->ext_size = 0;\
        ctx->save_size = 0;\
        ctx->ext_frame = __builtin_frame_address(0); \
    } \
    void (*cotask_yield_proc)(cotask_ctrl *) =  ((void (*)(cotask_ctrl *))name##_yield_code); \


#define cotask_yield() do{ cotask_yield_proc(ctx);}while(0)
#define cotask_resume(ctx) do{ cotask_ctrl *ctx_ = (ctx); ((void (*)(cotask_ctrl *))ctx_->proc_resume)(ctx_); }while(0)
#define cotask_start(call) ({ void *_; asm volatile("mov %%rsp, %0":"=r"(_)::"cc"); call;})


#define COA_JIT_MEMORY_SIZE 256


typedef struct _cotask_info {
    void *proc_yield;
    void *proc_resume;
    int frame_size;
    int yield_type;
} cotask_info;

typedef struct _cotask_ctrl {
    void *proc_resume; //0x0
    void *ext_frame; // 0x8
    void *continuation_sp; //0x10
    int save_size; //0x18
    int ext_size;//0x1c
} cotask_ctrl;

typedef long long intptr_t;

#define JIT_CMD_RELOC (0x1)
#define JIT_CMD_LABEL (0x2)
#define JIT_XCHG_NVS (0x3)
#define JIT_CMD_END (0x5)
#define JIT_CMD_RELOC_S (0x6)
#define JIT_CMD_RELOC_L (0xa)
#define JIT_CMD_FSIZE_RW (0xb)


#endif