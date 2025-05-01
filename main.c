////////////////////////////////////////////////////////////
// STM32WL55JC SPI Flash Loader - Pure RAM Execution
// Copyright (C) Infersens 2025
// Nazim Robbani / Rob Dobson
////////////////////////////////////////////////////////////

// We'll use standard register definitions
#include <stdint.h>

////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////

// Define IO macro since we're not using HAL
#define __IO volatile

// Loader return codes
#define LOADER_OK 1
#define LOADER_FAIL 0

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

// LED is on PB15 on STM32WL55JC Nucleo
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

// Op-codes for MX25R1635FZUIL0 / 16MBit Flash
#define CMD_READ        0x03
#define CMD_FAST_READ   0x0B
#define CMD_WRITE_EN    0x06
#define CMD_PAGE_PROG   0x02
#define CMD_SECTOR_ER   0x20  // 4 KB
#define CMD_CHIP_ER     0xC7
#define CMD_RDSR        0x05
#define CMD_CHIP_ID     0x9F
#define MX_CMD_RSTEN    0x66
#define MX_CMD_RST      0x99

////////////////////////////////////////////////////////////
// Forward declarations & externals
////////////////////////////////////////////////////////////

void Reset_Handler(void);
int Init(void);
static void LED_Init(void);
static void SPI_Init(void);
static void SPI_CS_Select(void);
static void SPI_CS_Deselect(void);
static uint8_t SPI_TransmitReceive(uint8_t data);
static int SPIFlash_WriteEnable(void);
static void SPIFlash_ReadJEDEC_ID(uint8_t *buf);
static void delay(uint32_t count);

// Stack top (defined in linker script)
extern uint32_t _estack;

// Memory section symbols from linker script
extern uint32_t _sbss;      // Start of BSS
extern uint32_t _ebss;      // End of BSS

////////////////////////////////////////////////////////////
// Vector table - this is placed at the beginning of RAM
////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////
// LED Functions
////////////////////////////////////////////////////////////

/// @brief Initialize LED GPIO
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

////////////////////////////////////////////////////////////
// SPI Functions
////////////////////////////////////////////////////////////

/// @brief Initialize SPI1 with GPIO pins
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

/// @brief Set CS pin low (active)
static void SPI_CS_Select(void) {
    GPIOB_BSRR = (1UL << (SPI_CS_PIN + 16)); // Reset bit (CS LOW)
}

/// @brief Set CS pin high (inactive)
static void SPI_CS_Deselect(void) {
    GPIOB_BSRR = (1UL << SPI_CS_PIN); // Set bit (CS HIGH)
}

/// @brief Transmit and receive one byte over SPI
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

////////////////////////////////////////////////////////////
// SPI Flash Utility Functions
////////////////////////////////////////////////////////////

/// @brief Wait until the Flash is not busy
/// @return LOADER_OK on success, LOADER_FAIL on error
static int SPIFlash_WaitWhileBusy(void)
{
    uint8_t status = 0;
    do {
        SPI_CS_Select();
        // Send RDSR command (0x05)
        SPI_TransmitReceive(CMD_RDSR);
        // Read status register
        status = SPI_TransmitReceive(0xFF);
        SPI_CS_Deselect();
    } while (status & 0x01);  // WIP = bit0
    return LOADER_OK;
}
/// @brief Enable write operations on the flash
/// @return LOADER_OK on success, LOADER_FAIL on error
static int SPIFlash_WriteEnable(void)
{
    SPI_CS_Select();
    // Send write enable command (0x06)
    SPI_TransmitReceive(CMD_WRITE_EN);
    SPI_CS_Deselect();
    return LOADER_OK;
}

/// @brief Read the status register
/// @return Status register value
static uint8_t SPIFlash_ReadStatus(void)
{
    uint8_t status = 0;
    SPI_CS_Select();
    // Send RDSR command (0x05)
    SPI_TransmitReceive(CMD_RDSR);
    // Read status
    status = SPI_TransmitReceive(0xFF);
    SPI_CS_Deselect();
    return status;
}

/// @brief Clear write protection
/// @return LOADER_OK on success, LOADER_FAIL on error
static int SPIFlash_ClearProtection(void) {
    if (SPIFlash_WriteEnable() != LOADER_OK)
        return LOADER_FAIL;
    SPI_CS_Select();
    // Write status register command (0x01)
    SPI_TransmitReceive(0x01);
    // Clear all protection bits
    SPI_TransmitReceive(0x00);
    SPI_CS_Deselect();
    return SPIFlash_WaitWhileBusy();
}

/// @brief Reset Macronix chip
/// @return LOADER_OK on success, LOADER_FAIL on error
static int SPIFlash_MxChipReset(void)
{
    SPI_CS_Select();
    // Send reset enable command
    SPI_TransmitReceive(MX_CMD_RSTEN);
    SPI_CS_Deselect();
    SPI_CS_Select();
    // Send reset command
    SPI_TransmitReceive(MX_CMD_RST);
    SPI_CS_Deselect();
    // Delay for reset to complete (approx 1ms)
    delay(50000);
    return LOADER_OK;
}

/// @brief Read JEDEC ID (3 bytes) from Flash
/// @param buf Buffer to store JEDEC ID
static void SPIFlash_ReadJEDEC_ID(uint8_t *buf) {
    SPI_CS_Select();
    // Send 0x9F command (Read JEDEC ID)
    SPI_TransmitReceive(0x9F);
    // Read 3 bytes of data
    buf[0] = SPI_TransmitReceive(0xFF); // Manufacturer ID
    buf[1] = SPI_TransmitReceive(0xFF); // Memory Type
    buf[2] = SPI_TransmitReceive(0xFF); // Capacity
    SPI_CS_Deselect();
}

////////////////////////////////////////////////////////////
// STM32 External Loader Flash Functions
////////////////////////////////////////////////////////////

/// @brief Write data to flash, handling page boundaries
/// @param Address Flash address to write to
/// @param Size Number of bytes to write
/// @param buffer Data to write
/// @return LOADER_OK on success, LOADER_FAIL on error
int Write(uint32_t Address, uint32_t Size, uint8_t *buffer)
{
    while (Size > 0)
    {
        uint32_t page_offset = Address & 0xFF;
        uint32_t chunk = 256 - page_offset;         // bytes left in this page
        if (chunk > Size) 
            chunk = Size;
        if (SPIFlash_WriteEnable() != LOADER_OK) 
            return LOADER_FAIL;
        SPI_CS_Select();
        // Send page program command and 3-byte address
        SPI_TransmitReceive(CMD_PAGE_PROG);
        SPI_TransmitReceive((uint8_t)(Address >> 16));
        SPI_TransmitReceive((uint8_t)(Address >> 8));
        SPI_TransmitReceive((uint8_t)(Address));
        // Send data
        for (uint32_t i = 0; i < chunk; i++) {
            SPI_TransmitReceive(buffer[i]);
        }
        SPI_CS_Deselect();
        if (SPIFlash_WaitWhileBusy() != LOADER_OK) 
            return LOADER_FAIL;
        Address += chunk;
        buffer  += chunk;
        Size    -= chunk;
    }
    return LOADER_OK;
}

/// @brief Read data from flash
/// @param Address Flash address to read from
/// @param Size Number of bytes to read
/// @param Buffer Buffer to store read data
/// @return LOADER_OK on success, LOADER_FAIL on error
int Read(uint32_t Address, uint32_t Size, uint8_t *Buffer)
{
    SPI_CS_Select();
    // Send read command and 3-byte address
    SPI_TransmitReceive(CMD_READ);
    SPI_TransmitReceive((uint8_t)(Address >> 16));
    SPI_TransmitReceive((uint8_t)(Address >> 8));
    SPI_TransmitReceive((uint8_t)(Address));
    // Read data
    for (uint32_t i = 0; i < Size; i++) {
        Buffer[i] = SPI_TransmitReceive(0xFF);
    }
    SPI_CS_Deselect();
    return LOADER_OK;
}

/// @brief Erase sectors covering the specified address range
/// @param EraseStartAddress Start address (will be aligned to 4KB boundary)
/// @param EraseEndAddress End address (will be aligned to 4KB boundary)
/// @return LOADER_OK on success, LOADER_FAIL on error
int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    // Align addresses to 4 KB boundaries
    EraseStartAddress &= ~(0xFFF);
    EraseEndAddress   = (EraseEndAddress + 0xFFF) & ~(0xFFF);
    for (uint32_t addr = EraseStartAddress; addr < EraseEndAddress; addr += 0x1000)
    {
        if (SPIFlash_WriteEnable() != LOADER_OK) 
            return LOADER_FAIL;
        SPI_CS_Select();
        // Send sector erase command and 3-byte address
        SPI_TransmitReceive(CMD_SECTOR_ER);
        SPI_TransmitReceive((uint8_t)(addr >> 16));
        SPI_TransmitReceive((uint8_t)(addr >> 8));
        SPI_TransmitReceive((uint8_t)(addr));
        SPI_CS_Deselect();
        if (SPIFlash_WaitWhileBusy() != LOADER_OK) 
            return LOADER_FAIL;
    }
    return LOADER_OK;
}

/// @brief Erase the entire flash chip
/// @return LOADER_OK on success, LOADER_FAIL on error
int MassErase(void)
{
    if (SPIFlash_WriteEnable() != LOADER_OK) 
        return LOADER_FAIL;
    SPI_CS_Select();
    // Send chip erase command
    SPI_TransmitReceive(CMD_CHIP_ER);
    SPI_CS_Deselect();
    if (SPIFlash_WaitWhileBusy() != LOADER_OK) 
        return LOADER_FAIL;
    return LOADER_OK;
}

////////////////////////////////////////////////////////////
// Utility Functions
////////////////////////////////////////////////////////////

/// @brief Simple delay function
/// @param count Number of cycles to delay
static void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __NOP();
    }
}

////////////////////////////////////////////////////////////
// Main Functions
////////////////////////////////////////////////////////////

/// @brief Reset handler - this is our real entry point
void Reset_Handler(void) {
    // Set the vector table to our RAM-based table
    SCB_VTOR = (uint32_t)g_pfnVectors;
    
    // Zero fill the BSS segment
    for (uint32_t *dest = &_sbss; dest < &_ebss; ) {
        *dest++ = 0;
    }
    
    // Jump to Init
    Init();
    
    // Should never reach here
    while (1) {}
}

/// @brief Init function
int Init(void) {
    // Storage for JEDEC ID
    uint8_t jedecID[3] = {0};

    // Set vector table location to RAM (0x20000000)
    SCB_VTOR = 0x20000000;

    // Initialize LED
    LED_Init();
    
    // Initialize SPI
    SPI_Init();
    
    // Infinite loop
    while (1) {
        // Read JEDEC ID (manufacturer, memory type, capacity)
        SPIFlash_ReadJEDEC_ID(jedecID);
        
        // Turn on LED (PB15)
        GPIOB_BSRR = (1UL << LED_PIN);
        delay(500000);
        
        // Turn off LED (PB15)
        GPIOB_BSRR = (1UL << (LED_PIN + 16));
        delay(500000);
    }
    
    // Should never reach here
    return 0;
}
