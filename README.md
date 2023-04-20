# Ac-CoA: Adaptive context-saving coroutine almost-linear

Ac-CoA is a fast and lightweight experimental symmetric coroutine library. Design for implementing 
a javascript-like asynchronous framework in C (GNU/C)

**Warning!! Don't use in production environment**

**This program just happens to work on several compilers. Check [Assumptions](#Assumptions) for detail**

Name after Acetyl-CoemzymeA. And the abbrevation of "Adaptive context-saving coroutine almost-linear".



Features: 
- Neither stackful nor stackless. It doesn't allocate a large new stack with predefined size like stackful coroutine. And there is no need to manually collect local varibles into closure context.

- Extremely fast and memory-efficient. Ac-CoA automatically probe what is required to be saved for a coroutine procedure in runtime, and only save registers and stack frame that need to be saved.

  Fully utilizing the memory layout&local varible lifetime analysis capability of the compiler

  Benchmark shows a coroutine yield/resume only cost **11.5ns**

  *10,000,000* coroutines simultaneously to run cost only *993MB* memory (with glibc malloc)
- Creating a coroutine is as simple as calling a normal function. See usage part.

- High performance across platforms and ABIs. For example, on Windows x64 ABI 
  (9 Non-Volatile General Purpose Registers + 7 Non-Volatile 128bits Vector Registers + 32b register home),
  it works almost as fast as on System V x64 ABI (only 6 Non-Volatile General Purpose Registers) 

- Nearly linear time overhead. As the number of concurrent processes grows, the overhead of context switching remains almost constant (Sometimes even lower)

### Assumptions
Ac-CoA only work properly when the code compiler generated meet the requirements below.

- Compiler only use frame pointer to access the stack frame
- Compiler only use stack pointer to access dynamically allocated memory-passed parameters
- No dynamic allocation on stack frame escaped
- No VLA(Variable Length Array) on the coroutine stack
- Yield is called directly in coroutine procedure, or all fucntions below the coroutine stack frame
  can endure its stack frame to be moved (no stack reference in frames/registers)

A dynamic machine code rewriter may be implemented in later versions to automatically check and correct the code doesn't satisify the requirements above.

### Synposis
file `coa_main.c`
```c
#include <stdio.h>
#include <stdlib.h>
#include "./coa.h"

// define coroutine 'cotask1', takes a parameter 'n'
def_cotask(cotask1, int n) {
    cotask_entry(cotask1); // initialize context

    for(int i = 0;i < n;i++) {
        printf("Yield %d times\n", i);
        
        cotask_yield(); // go back to its caller
    }
    return ctx;
}

int main() {
    int n = 10;

    // start coroutine. Just wrap the invocation expression with cotask_start(..)
    // Multiple parameters and even variable length parameters are supported
    cotask_ctrl *ctx = cotask_start(cotask1(_, n));
    for(int i = 1;i < n;i++) {
        printf("Resume %d times\n", i);
        // resume executation form previous yield point
        cotask_resume(ctx);

    }
    // free the context
    free(ctx);
    return 0;
}
```
```bash
$ gcc coa_main.c coa.c coa_template.S -o coa -O2 -fstack-protector-all
$ ./coa
Jit memory (0x55cea78aa1c7, 0x55cea78aa2c7)
Yield to main 0 times
Resume to cotask 1 times
Yield to main 1 times
Resume to cotask 2 times
Yield to main 2 times
Resume to cotask 3 times
Yield to main 3 times
Resume to cotask 4 times
Yield to main 4 times
Resume to cotask 5 times
Yield to main 5 times
Resume to cotask 6 times
Yield to main 6 times
Resume to cotask 7 times
Yield to main 7 times
Resume to cotask 8 times
Yield to main 8 times
Resume to cotask 9 times
Yield to main 9 times
```

### Description
