#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>

#include "coa.h"

#define ptr_align_floor(ptr, size) (typeof(ptr))((((intptr_t)(ptr)) / (size)) * (size))
#define ptr_align_ceil(ptr, size) (typeof(ptr))((((((intptr_t)(ptr))-1) / (size))+1) * (size))


extern void cotask_yield_template(cotask_ctrl *ctx) asm("cotask_yield_template");
extern void cotask_resume_template(cotask_ctrl *ctx) asm("cotask_resume_template");


typedef struct _coa_jit_kv {
    char *k;
    char *v;
    struct _coa_jit_kv *next;
} coa_jit_kv;

typedef struct _cotask_probe {
    char *base;
    int frame_size;
    int magic;
} cotask_probe;


int reg_code[] = { 0x0, 0b0011, 0b0101, 0b1100, 0b1101,0b1110,0b1111, };


void LCotaskEnter() asm("LCotaskEnter");
int cotask_probe_frame_size(cotask_ctrl *ctx, void *proc) asm("cotask_probe_frame_size");
void cotask_probe_stack_structure(char *probe_stack, void *proc, int magic) asm("cotask_probe_stack_structure");

cotask_ctrl *cotask_save_eframe(cotask_ctrl *ctx, void *sp, size_t new_size) {
    if(ctx->ext_size < new_size) {
        ctx->ext_frame = ctx->ext_size ? realloc(ctx->ext_frame, new_size) : malloc(new_size);
        ctx->ext_size = new_size;
    }
    ctx->save_size = new_size;
    memcpy(ctx->ext_frame, sp, new_size);
    return ctx;
}

char *coa_emit_mov_rr(char *jit_mem, int src, int dst) {
    int src_h8 = !!(src & 0b1000);
    int dst_h8 = !!(dst & 0b1000);
    *(jit_mem++) = 0x48 | (src_h8 << 2) | (dst_h8);
    *(jit_mem++) = 0x89;
    *(jit_mem++) = 0b11000000 | ((src & 0b111) << 3) | (dst & 0b111);
    return jit_mem;
}

char *coa_emit_mov_rtm(char *jit_mem, int src, int dst, int offset) {
    int src_h8 = !!(src & 0b1000);
    int dst_h8 = !!(dst & 0b1000);
    int disp32 = offset >= 128 || offset < -128;
    *(jit_mem++) = 0x48 | (src_h8 << 2) | (dst_h8);
    *(jit_mem++) = 0x89;
    *(jit_mem++) = (disp32 ? 0b10000000 : 0b01000000) | ((src & 0b111) << 3) | (dst & 0b111);
    if(disp32) {
        *(int *)jit_mem = offset;
        jit_mem += 4;
    } else {
        *(jit_mem++) = offset;
    }
    return jit_mem;
}

char *coa_emit_mov_mtr(char *jit_mem, int src, int dst, int offset) {
    int src_h8 = !!(src & 0b1000);
    int dst_h8 = !!(dst & 0b1000);
    int disp32 = offset >= 128 || offset < -128;
    *(jit_mem++) = 0x48 | (dst_h8 << 2) | (src_h8);
    *(jit_mem++) = 0x8b;
    *(jit_mem++) = (disp32 ? 0b10000000 : 0b01000000) | ((dst & 0b111) << 3) | (src & 0b111);
    if(disp32) {
        *(int *)jit_mem = offset;
        jit_mem += 4;
    } else {
        *(jit_mem++) = offset;
    }
    return jit_mem;
}

// rax,rcx,rsi,r8,r9,r10
int spare_reg[] = { 0b0000,0b0001,0b0110,0b1000,0b1001,0b1010 };
char *coa_jit_emit_nvrs_code(char *jit_mem, cotask_probe *probe1, cotask_probe *probe2) {
    int frame_size = probe1->frame_size;
    char *p1_base_ptr = probe1->base - 8;
    int spare_index = 0;
    for(int i = 0;i < frame_size;i += sizeof(void *)) {
        intptr_t *pv1 = (intptr_t *)(probe1->base - i - 8);
        intptr_t *pv2 = (intptr_t *)(probe2->base - i - 8);
        intptr_t v1 = *pv1 - probe1->magic;
        intptr_t v2 = *pv2 - probe2->magic;
        if(v1 == v2 && v1 >= 1 && v1 <= 6) {
            jit_mem = coa_emit_mov_rr(jit_mem, reg_code[v1], spare_reg[spare_index++]);
        }
    }
    for(int i = 0;i < frame_size;i += sizeof(void *)) {
        intptr_t *pv1 = (intptr_t *)(probe1->base - i - 8);
        intptr_t *pv2 = (intptr_t *)(probe2->base - i - 8);
        intptr_t v1 = *pv1 - probe1->magic;
        intptr_t v2 = *pv2 - probe2->magic;
        if(v1 == v2 && v1 >= 1 && v1 <= 6) {
            jit_mem = coa_emit_mov_mtr(jit_mem, 0b0010, reg_code[v1], ((char *)pv1) - p1_base_ptr);
        }
    }
    spare_index = 0;
    for(int i = 0;i < frame_size;i += sizeof(void *)) {
        intptr_t *pv1 = (intptr_t *)(probe1->base - i - 8);
        intptr_t *pv2 = (intptr_t *)(probe2->base - i - 8);
        intptr_t v1 = *pv1 - probe1->magic;
        intptr_t v2 = *pv2 - probe2->magic;
        if(v1 == v2 && v1 >= 1 && v1 <= 6) {
            jit_mem = coa_emit_mov_rtm(jit_mem, spare_reg[spare_index++], 0b0010, ((char *)pv1) - p1_base_ptr);
        }
    }
    return jit_mem;
}

coa_jit_kv *coa_kv_find(coa_jit_kv *head, char *k) {
    for(;head;head = head->next) {
        if(head->k == k) return head;
    }
    return 0x0;
}

void coa_kv_insert(coa_jit_kv **head, char *k, char *v) {
    coa_jit_kv *node = malloc(sizeof(coa_jit_kv));
    node->next = *head;
    node->k = k;
    node->v = v;
    *head = node;
}

char *coa_jit_compile(char *jit_mem, char *template, cotask_probe *probe1, cotask_probe *probe2, cotask_info *info) {
    coa_jit_kv *reloc_list = 0x0;
    coa_jit_kv *reloc_list_s = 0x0;
    coa_jit_kv *label_list = 0x0;

    for(char *i = template; ;) {
        if((i[0] == 0x66) && (i[1] == 0xF) && (i[2] == 0x1F) && (i[3] == 0x40)) {
            switch(i[4]) {
                case JIT_CMD_RELOC: {
                    int delta = *(int *)(i - 4);
                    char *template_target = i + delta;

                    int code_del = template_target - (jit_mem);
                    *(int *)(jit_mem - 4) = code_del;
                    i += 5;

                    continue;
                }
                case JIT_CMD_RELOC_S: {
                    int delta = *(i - 1);
                    char *template_target = i + delta;

                    coa_jit_kv *label = coa_kv_find(label_list, template_target);
                    if(label) {
                        int code_del = label->v - (jit_mem);
                        *(char *)(jit_mem - 1) = (char)code_del;
                    } else {
                        coa_kv_insert(&reloc_list_s, template_target, jit_mem - 1);
                    }
                    i += 5;

                    continue;
                }
                case JIT_CMD_RELOC_L: {
                    int delta = *(int *)(i - 4) - 0x40000000;
                    char *template_target = i + delta;

                    coa_jit_kv *label = coa_kv_find(label_list, template_target);
                    if(label) {
                        int code_del = label->v - (jit_mem);
                        *(int *)(jit_mem - 4) = code_del;
                    } else {
                        coa_kv_insert(&reloc_list, template_target, jit_mem - 4);
                    }
                    i += 5;

                    continue;
                }
                case JIT_CMD_LABEL: {
                    i += 5;
                    coa_kv_insert(&label_list, i, jit_mem);
                    for(coa_jit_kv *j = reloc_list;j;j = j->next) {
                        if(j->k == i) {
                            *(int *)j->v = jit_mem - (j->v + 4);
                        }
                    }
                    for(coa_jit_kv *j = reloc_list_s;j;j = j->next) {
                        if(j->k == i) {
                            *j->v = jit_mem - (j->v + 1);
                        }
                    }

                    continue;
                }
                case JIT_XCHG_NVS: {
                    i += 5;
                    jit_mem = coa_jit_emit_nvrs_code(jit_mem, probe1, probe2);
                    continue;
                }
                case JIT_CMD_END: {
                    return jit_mem;
                }
                case JIT_CMD_FSIZE_RW: {
                    i += 5;
                    *(int *)(jit_mem - 4) = info->frame_size;
                    continue;
                }
            }
        }
        *(jit_mem++) = *(i++);
    }
    __builtin_unreachable();
}

void cotask_setprotect(char *jit_mem, size_t length, int en_wr) {
    char *jit_mem_end_unaligned = (jit_mem + length);
    char *jit_mem_start_aligned = ptr_align_floor(jit_mem, 4096);
    char *jit_mem_end_aligned = ptr_align_ceil(jit_mem_end_unaligned, 4096);

    mprotect(jit_mem_start_aligned, jit_mem_end_aligned - jit_mem_start_aligned, PROT_READ | (en_wr * PROT_WRITE) | PROT_EXEC);
}

void cotask_gerneate_asm(cotask_info *info, cotask_probe *probe, cotask_probe *probe2) {

    cotask_setprotect(info->proc_yield, COA_JIT_MEMORY_SIZE, 1);
    cotask_setprotect(info->proc_resume, COA_JIT_MEMORY_SIZE, 1);
    printf("Jit memory (0x%lx, 0x%lx)\n", info->proc_yield, info->proc_resume);


    coa_jit_compile(info->proc_yield, (char *)cotask_yield_template, probe, probe2, info);
    coa_jit_compile(info->proc_resume, (char *)cotask_resume_template, probe, probe2, info);

    cotask_setprotect(info->proc_yield, COA_JIT_MEMORY_SIZE, 0);
    cotask_setprotect(info->proc_resume, COA_JIT_MEMORY_SIZE, 0);

    return;
}


cotask_ctrl *cotask_init(int mp_size, cotask_info *info, void *proc, void *sp, void **psp) {
    if(__builtin_expect(info->frame_size <= 0, 0)) {
        cotask_ctrl ctx;
        int frame_size = info->frame_size = cotask_probe_frame_size(&ctx, proc);


        char *probe_stack1 = malloc(info->frame_size + 40);
        char *probe_stack2 = malloc(info->frame_size + 40);
        cotask_probe_stack_structure(probe_stack1 + frame_size + 24, proc, 0x1926);
        cotask_probe_stack_structure(probe_stack2 + frame_size + 24, proc, 0x0817);

        cotask_probe probe1 = { probe_stack1 + frame_size + 24,frame_size,0x1926 };
        cotask_probe probe2 = { probe_stack2 + frame_size + 24,frame_size,0x0817 };

        cotask_gerneate_asm(info, &probe1, &probe2);

        free(probe_stack1);
        free(probe_stack2);
    }
    char *ctx_buffer = malloc(mp_size + sizeof(cotask_ctrl) - 0x8 + info->frame_size + info->yield_type);
    memset(ctx_buffer, 0, info->yield_type);
    cotask_ctrl *ctx = (cotask_ctrl *)(ctx_buffer + info->yield_type);
    char *ret_addr = ((char *)ctx) + info->frame_size + sizeof(cotask_ctrl) - 0x8;
    memcpy(ret_addr, sp, mp_size);
    ctx->continuation_sp = (void *)LCotaskEnter;
    ctx->ext_size = -1;
    ctx->save_size = 0;
    ctx->ext_frame = ((char *)sp) + 0x8 - info->frame_size;
    ctx->proc_resume = info->proc_resume;
    psp[0] = ret_addr;
    return ctx;
}

