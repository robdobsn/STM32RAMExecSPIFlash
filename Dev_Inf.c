/*
 * Dev_Inf.c
 *
 *  Created on: Apr 23, 2025
 *      Author: nazimrobanni
 */


// Dev_Inf.c

#include "Dev_Inf.h"

// Total size: 16 Mbit = 2 MiB = 0x200000 bytes
#define MX25R1635_TOTAL_SIZE   0x00200000UL
// Page size: 256 B
#define MX25R1635_PAGE_SIZE    0x00000100UL
// Sector size: 4 KiB
#define MX25R1635_SECTOR_SIZE  0x00001000UL
// Number of sectors: total / sector
#define MX25R1635_SECTOR_COUNT (MX25R1635_TOTAL_SIZE / MX25R1635_SECTOR_SIZE)

#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
struct StorageInfo __attribute__((section(".Dev_info"))) StorageInfo = {
#endif
    "MX25R1635FZUIL0_STM32WL55CCU6",  // DeviceName
    SPI_FLASH,                        // DeviceType
    0x00000000UL,                     // DeviceStartAddress
    MX25R1635_TOTAL_SIZE,             // DeviceSize
    MX25R1635_PAGE_SIZE,              // PageSize
    0xFF,                             // EraseValue

    // sectors[]: {count, size}, then a terminating {0,0}
    {
        { MX25R1635_SECTOR_COUNT, MX25R1635_SECTOR_SIZE },
        { 0,                    0                  }
    }
};
