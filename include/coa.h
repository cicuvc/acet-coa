#ifndef _H_COROUTINE_COA
#define _H_COROUTINE_COA

#include "./coa_def.h"

#if ARCH==x86_64
#include <arch/x86_64/coa_asm.h>
#endif

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
        cotask_entry_asm(ctx); \
        ctx->ext_size = 0;\
        ctx->save_size = 0;\
        ctx->ext_frame = __builtin_frame_address(0); \
    } \
    void (*cotask_yield_proc)(cotask_ctrl *) =  ((void (*)(cotask_ctrl *))name##_yield_code); \


#define cotask_yield() do{ cotask_yield_proc(ctx);}while(0)
#define cotask_resume(ctx) do{ cotask_ctrl *ctx_ = (ctx); ((void (*)(cotask_ctrl *))ctx_->proc_resume)(ctx_); }while(0)

#define COA_JIT_MEMORY_SIZE 256

typedef long long intptr_t;

#define JIT_CMD_RELOC (0x1)
#define JIT_CMD_LABEL (0x2)
#define JIT_XCHG_NVS (0x3)
#define JIT_CMD_END (0x5)
#define JIT_CMD_RELOC_S (0x6)
#define JIT_CMD_RELOC_L (0xa)
#define JIT_CMD_FSIZE_RW (0xb)

#endif