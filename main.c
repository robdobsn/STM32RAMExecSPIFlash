/**
 * Minimal STM32WL55JC LED blink example - Pure RAM Execution
 */

// We'll use standard register definitions
#include <stdint.h>

// Define IO macro since we're not using HAL
#define __IO volatile

// Define register addresses directly
#define RCC_BASE            (0x58000000UL)
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB2ENR_GPIOAEN (1UL << 0)
#define RCC_AHB2ENR_GPIOBEN (1UL << 1)
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_APB2ENR_SPI1EN  (1UL << 12)

// GPIO registers
#define GPIOA_BASE          (0x48000000UL)
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OTYPER        (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_OSPEEDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_PUPDR         (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define GPIOA_ODR           (*(volatile uint32_t *)(GPIOA_BASE + 0x14))
#define GPIOA_BSRR          (*(volatile uint32_t *)(GPIOA_BASE + 0x18))
#define GPIOA_AFRL          (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_AFRH          (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

#define GPIOB_BASE          (0x48000400UL)
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x14))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18))

// SPI1 registers
#define SPI1_BASE           (0x40013000UL)
#define SPI1_CR1            (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_CR2            (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI1_SR             (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR             (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

// SPI CR1 bits
#define SPI_CR1_SPE         (1UL << 6)    // SPI Enable
#define SPI_CR1_MSTR        (1UL << 2)    // Master Mode
#define SPI_CR1_BR_DIV8     (0x2UL << 3)  // Baud Rate = fPCLK/8
#define SPI_CR1_SSM         (1UL << 9)    // Software Slave Management
#define SPI_CR1_SSI         (1UL << 8)    // Internal Slave Select

// SPI CR2 bits
#define SPI_CR2_FRXTH       (1UL << 12)   // FIFO reception threshold
#define SPI_CR2_DS_8BIT     (0x7UL << 8)  // 8-bit data size

// SPI SR bits
#define SPI_SR_TXE          (1UL << 1)    // Transmit buffer empty
#define SPI_SR_RXNE         (1UL << 0)    // Receive buffer not empty
#define SPI_SR_BSY          (1UL << 7)    // Busy flag

// SCB (System Control Block) register for vector table relocation
#define SCB_BASE            (0xE000ED00UL)
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))

// LED is on PB15
#define LED_PIN             (15U)
#define LED_PIN_MASK        (1UL << LED_PIN)

// SPI pin definitions
#define SPI_SCK_PIN         (5U)  // PA5
#define SPI_MISO_PIN        (6U)  // PA6
#define SPI_MOSI_PIN        (7U)  // PA7
#define SPI_CS_PIN          (5U)  // PB5
#define SPI_CS_PIN_MASK     (1UL << SPI_CS_PIN)

// GPIO Alternate Function for SPI1
#define GPIO_AF5            (5UL)

// No operation instruction
#define __NOP()             __asm volatile ("nop")

// Forward declarations
void Reset_Handler(void);
int main(void);
static void LED_Init(void);
static void SPI_Init(void);
static void SPI_CS_Select(void);
static void SPI_CS_Deselect(void);
static uint8_t SPI_TransmitReceive(uint8_t data);
static void SPI_ReadJEDEC_ID(uint8_t *buf);
static void delay(uint32_t count);

// Stack top (defined in linker script)
extern uint32_t _estack;

// Memory section symbols from linker script
extern uint32_t _sbss;      // Start of BSS
extern uint32_t _ebss;      // End of BSS

// Vector table - this is placed at the beginning of RAM
typedef void (*vector_table_entry_t)(void);
__attribute__((section(".vectors")))
const vector_table_entry_t g_pfnVectors[] = {
    (vector_table_entry_t)&_estack, // Initial stack pointer value
    Reset_Handler,                  // Reset handler
    0,                              // NMI_Handler
    0,                              // HardFault_Handler
    0,                              // MemManage_Handler
    0,                              // BusFault_Handler
    0,                              // UsageFault_Handler
    0, 0, 0, 0,                     // Reserved
    0,                              // SVC_Handler
    0,                              // DebugMon_Handler
    0,                              // Reserved
    0,                              // PendSV_Handler
    0,                              // SysTick_Handler
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
 * Initialize SPI1 with GPIO pins
 */
static void SPI_Init(void) {
    // Enable GPIOA and GPIOB clocks
    RCC_AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);
    
    // Enable SPI1 clock
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;
    
    // Configure SPI pins: PA5 (SCK), PA6 (MISO), PA7 (MOSI) as Alternate Function
    // PA5 (SCK)
    GPIOA_MODER &= ~(3UL << (SPI_SCK_PIN * 2));
    GPIOA_MODER |= (2UL << (SPI_SCK_PIN * 2));  // Alternate function
    GPIOA_AFRL &= ~(0xFUL << (SPI_SCK_PIN * 4));
    GPIOA_AFRL |= (GPIO_AF5 << (SPI_SCK_PIN * 4)); // AF5 for SPI1
    
    // PA6 (MISO)
    GPIOA_MODER &= ~(3UL << (SPI_MISO_PIN * 2));
    GPIOA_MODER |= (2UL << (SPI_MISO_PIN * 2));  // Alternate function
    GPIOA_AFRL &= ~(0xFUL << (SPI_MISO_PIN * 4));
    GPIOA_AFRL |= (GPIO_AF5 << (SPI_MISO_PIN * 4)); // AF5 for SPI1
    
    // PA7 (MOSI)
    GPIOA_MODER &= ~(3UL << (SPI_MOSI_PIN * 2));
    GPIOA_MODER |= (2UL << (SPI_MOSI_PIN * 2));  // Alternate function
    GPIOA_AFRL &= ~(0xFUL << (SPI_MOSI_PIN * 4));
    GPIOA_AFRL |= (GPIO_AF5 << (SPI_MOSI_PIN * 4)); // AF5 for SPI1
    
    // Configure PB5 as output for CS (Mode = 01)
    GPIOB_MODER &= ~(3UL << (SPI_CS_PIN * 2));
    GPIOB_MODER |= (1UL << (SPI_CS_PIN * 2));
    
    // Configure CS as push-pull
    GPIOB_OTYPER &= ~(1UL << SPI_CS_PIN);
    
    // Configure CS with no pull-up/pull-down
    GPIOB_PUPDR &= ~(3UL << (SPI_CS_PIN * 2));
    
    // Configure CS as high speed
    GPIOB_OSPEEDR &= ~(3UL << (SPI_CS_PIN * 2));
    GPIOB_OSPEEDR |= (3UL << (SPI_CS_PIN * 2));
    
    // Initialize CS to HIGH (inactive)
    GPIOB_BSRR = (1UL << SPI_CS_PIN);
    
    // Configure SPI1
    // CR1: 
    // - Master mode
    // - Baud rate = fPCLK/8
    // - CPOL=0, CPHA=0 (SPI mode 0)
    // - Software slave management (SSM=1, SSI=1)
    // - MSB first
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV8 | SPI_CR1_SSM | SPI_CR1_SSI;
    
    // CR2:
    // - 8-bit data size
    // - Set FIFO reception threshold to 1/4 (8-bit)
    SPI1_CR2 = SPI_CR2_FRXTH | SPI_CR2_DS_8BIT;
    
    // Enable SPI1
    SPI1_CR1 |= SPI_CR1_SPE;
}

/**
 * Set CS pin low (active)
 */
static void SPI_CS_Select(void) {
    GPIOB_BSRR = (1UL << (SPI_CS_PIN + 16)); // Reset bit (CS LOW)
}

/**
 * Set CS pin high (inactive)
 */
static void SPI_CS_Deselect(void) {
    GPIOB_BSRR = (1UL << SPI_CS_PIN); // Set bit (CS HIGH)
}

/**
 * Transmit and receive one byte over SPI
 */
static uint8_t SPI_TransmitReceive(uint8_t data) {
    // Wait until TXE flag is set (Transmit buffer empty)
    while (!(SPI1_SR & SPI_SR_TXE)) {}
    
    // Send data
    *(volatile uint8_t *)&SPI1_DR = data;
    
    // Wait until RXNE flag is set (Receive buffer not empty)
    while (!(SPI1_SR & SPI_SR_RXNE)) {}
    
    // Return received data
    return *(volatile uint8_t *)&SPI1_DR;
}

/**
 * Read JEDEC ID (3 bytes) from Flash
 */
static void SPI_ReadJEDEC_ID(uint8_t *buf) {
    SPI_CS_Select();
    
    // Send 0x9F command (Read JEDEC ID)
    SPI_TransmitReceive(0x9F);
    
    // Read 3 bytes of data
    buf[0] = SPI_TransmitReceive(0xFF); // Manufacturer ID
    buf[1] = SPI_TransmitReceive(0xFF); // Memory Type
    buf[2] = SPI_TransmitReceive(0xFF); // Capacity
    
    SPI_CS_Deselect();
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
    // Storage for JEDEC ID
    uint8_t jedecID[3] = {0};
    
    // Initialize LED GPIO
    LED_Init();
    
    // Initialize SPI
    SPI_Init();
    
    // Main loop - this runs from RAM
    while (1) {
        // LED ON
        GPIOB_BSRR = LED_PIN_MASK;
        
        // Read JEDEC ID
        SPI_ReadJEDEC_ID(jedecID);
        
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