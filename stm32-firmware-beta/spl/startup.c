/* Minimal startup for STM32F103 */

extern unsigned int _etext, _sdata, _edata, _sbss, _ebss, _estack;

void Reset_Handler(void);
void Default_Handler(void);

void SystemInit(void) __attribute__((weak));

// Vector table
__attribute__((section(".isr_vector")))
void (* const vectors[])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler,
    Default_Handler,  // NMI
    Default_Handler,  // HardFault
    Default_Handler,  // MemManage
    Default_Handler,  // BusFault
    Default_Handler,  // UsageFault
};

void Reset_Handler(void) {
    // Copy data section
    unsigned int *src = &_etext;
    unsigned int *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    
    // Zero BSS
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;
    
    // Call SystemInit if provided
    if (SystemInit) SystemInit();
    
    // Call main
    extern int main(void);
    main();
    
    // Hang if main returns
    while (1);
}

void Default_Handler(void) {
    while (1);
}

// Minimal heap implementation for sprintf
static unsigned char heap_memory[4096];  // 4KB heap
void *_sbrk(int incr) {
    static unsigned char *heap_ptr = 0;
    unsigned char *prev_heap;
    
    if (heap_ptr == 0) {
        heap_ptr = heap_memory;
    }
    
    prev_heap = heap_ptr;
    heap_ptr += incr;
    
    // Simple overflow check
    if (heap_ptr >= heap_memory + sizeof(heap_memory)) {
        heap_ptr = prev_heap;  // Revert
        return (void *)-1;      // Error
    }
    
    return prev_heap;
}
