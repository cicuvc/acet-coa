#include <stdio.h>
#include <stdlib.h>
#include "./coa.h"

typedef struct _promise_info {
    intptr_t next : 48;
    intptr_t state : 2;
    intptr_t ref_count : 14;
} promise_info;

typedef struct _promise_ctx_cotask2 {
    long long result;
    promise_info info __attribute__((aligned(8)));
}promise_ctx_cotask2;

typedef union _promise_typehint_cotask2 {
    cotask_ctrl ctx;
    promise_ctx_cotask2 *type_hint;
} promise_typehint_cotask2;

typedef struct _promise_ctx_cotask1 {
    long long result;
    promise_info info __attribute__((aligned(8)));
} promise_ctx_cotask1;

typedef union _promise_typehint_cotask1 {
    cotask_ctrl ctx;
    promise_ctx_cotask1 *type_hint;
} promise_typehint_cotask1;


cotask_ctrl *ictx;

#define def_async(name, ...) \
    promise_typehint_##name *name(cotask_ctrl *, __VA_ARGS__); \
    static promise_typehint_##name name##_typehint[0];\
    asm(tostring_defer(cotask_prolog name, COA_JIT_MEMORY_SIZE)); \
    extern char name##_yield_code[COA_JIT_MEMORY_SIZE]; \
    extern char name##_resume_code[COA_JIT_MEMORY_SIZE]; \
    cotask_info name##_info = { (void *)name##_yield_code,(void *)name##_resume_code,0, sizeof(*((promise_typehint_##name *)0x0)->type_hint) }; \
    promise_typehint_##name *coimpl_##name(cotask_ctrl *ctx, __VA_ARGS__)


#define promise_get_info(ctx) (&(((promise_info*)(ctx))[-1]))
#define promise_get_ctx(ctx) (&(((typeof(ctx->type_hint))(ctx))[-1]))

def_async(cotask2, int n) {
    cotask_entry(cotask2);

    typeof(&cotask2_typehint[0]) async_ctx = (typeof(&cotask2_typehint[0]))ctx;

    ictx = ctx;
    printf("Cotask2 enter\n");
    cotask_yield();

    printf("reading\n");
    long long value;
    scanf("%lld", &value);
    promise_get_ctx(async_ctx)->result = value;
    promise_info *self_info = promise_get_info(ctx);
    while(self_info->next) {
        promise_info *awaiter_info = (promise_info *)(intptr_t)self_info->next;
        cotask_ctrl *awaiter = (cotask_ctrl *)&awaiter_info[1];
        self_info->next = awaiter_info->next;
        cotask_resume(awaiter);
    }

    cotask_yield();

    return (void *)ctx;
}

def_async(cotask1, int n) {
    cotask_entry(cotask1);
    typeof(&cotask2_typehint[0]) async_ctx = (typeof(&cotask2_typehint[0]))ctx;

    printf("Cotask1 enter\n");

    promise_typehint_cotask2 *c2 = cotask_start(cotask2(_, 10));

    promise_info *self_info = promise_get_info(async_ctx);
    promise_info *callee_info = promise_get_info(c2);
    self_info->next = callee_info->next;
    callee_info->next = (intptr_t)self_info;
    cotask_yield();
    long long result = promise_get_ctx(c2)->result;
    printf("Get value %lld\n", result);

    cotask_yield();
    return (void *)ctx;
}

int main() {
    int gn = 0;
    promise_typehint_cotask1 *ctx = cotask_start(cotask1(_, 10));
    scanf("%d", &gn);
    cotask_resume(ictx);
    return 0;
}