#ifndef _H_COA_DEF
#define _H_COA_DEF

asm(".equ CTL_CONT_OFFSET, 0x10");

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


#endif