/**
 * Minimal STM32WL55JC LED blink example - Pure RAM Execution
 */

// We'll use standard register definitions
#include <stdint.h>

// Define register addresses directly
#define RCC_BASE            (0x58000000UL)
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB2ENR_GPIOBEN (1UL << 1)

#define GPIOB_BASE          (0x48000400UL)
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x14))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18))

// SCB (System Control Block) register for vector table relocation
#define SCB_BASE            (0xE000ED00UL)
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))

// LED is on PB15
#define LED_PIN             (15U)
#define LED_PIN_MASK        (1UL << LED_PIN)

// No operation instruction
#define __NOP()             __asm volatile ("nop")

// Forward declarations
void Reset_Handler(void);
int main(void);

// Stack top (defined in linker script)
extern uint32_t _estack;

// Memory section symbols from linker script
extern uint32_t _sbss;      // Start of BSS
extern uint32_t _ebss;      // End of BSS

// Vector table - this is placed at the beginning of RAM
__attribute__((section(".vectors")))
void (*const g_pfnVectors[])(void) = {
    (void (*)(void))&_estack,  // Initial stack pointer value
    Reset_Handler,             // Reset handler points to our Reset_Handler
    0,                         // NMI_Handler
    0,                         // HardFault_Handler
    0,                         // MemManage_Handler
    0,                         // BusFault_Handler
    0,                         // UsageFault_Handler
    0, 0, 0, 0,                // Reserved
    0,                         // SVC_Handler
    0,                         // DebugMon_Handler
    0,                         // Reserved
    0,                         // PendSV_Handler
    0,                         // SysTick_Handler
    // External interrupts are all NULL
};

/**
 * Initialize LED GPIO
 */
static void LED_Init(void) {
    // Enable GPIOB clock
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    
    // Configure PB15 as output (Mode = 01)
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |= (1UL << (LED_PIN * 2));
    
    // Configure as push-pull (default, 0)
    GPIOB_OTYPER &= ~(1UL << LED_PIN);
    
    // Configure with no pull-up/pull-down (00)
    GPIOB_PUPDR &= ~(3UL << (LED_PIN * 2));
    
    // Configure as low speed (00)
    GPIOB_OSPEEDR &= ~(3UL << (LED_PIN * 2));
    
    // Initialize to OFF state
    GPIOB_BSRR = (1UL << (LED_PIN + 16)); // Reset bit
}

/**
 * Simple delay function
 */
static void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __NOP();
    }
}

/**
 * Reset handler - this is our real entry point
 */
void Reset_Handler(void) {
    // Set the vector table to our RAM-based table
    SCB_VTOR = (uint32_t)g_pfnVectors;
    
    // Zero fill the BSS segment
    for (uint32_t *dest = &_sbss; dest < &_ebss; ) {
        *dest++ = 0;
    }
    
    // Jump to main program
    main();
    
    // Should never reach here
    while (1) {}
}

/**
 * Main function
 */
int main(void) {
    // Initialize LED GPIO
    LED_Init();
    
    // Main loop - this runs from RAM
    while (1) {
        // LED ON
        GPIOB_BSRR = LED_PIN_MASK;
        
        // Delay
        delay(1000000);
        
        // LED OFF
        GPIOB_BSRR = (LED_PIN_MASK << 16);
        
        // Delay
        delay(1000000);
    }
    
    // Never reached
    return 0;
} 