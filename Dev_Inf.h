/*
 * Dev_Inf.h
 *
 *  Created on: Apr 23, 2025
 *      Author: nazimrobanni
 */

#ifndef INC_DEV_INF_H_
#define INC_DEV_INF_H_

#define  MCU_FLASH    1
#define  NAND_FLASH   2
#define  NOR_FLASH    3
#define  SRAM         4
#define  PSRAM        5
#define  PC_CARD      6
#define  SPI_FLASH    7
#define  I2C_FLASH    8
#define  SDRAM        9
#define  I2C_EEPROM   10

#define SECTOR_NUM    10   // Max number of distinct sector‐types

struct DeviceSectors {
    unsigned long SectorNum;    // Number of sectors of this size
    unsigned long SectorSize;   // Size of each sector in bytes
};

struct StorageInfo {
    char             DeviceName[100];      // Device name / description
    unsigned short   DeviceType;           // One of the above #defines
    unsigned long    DeviceStartAddress;   // Base address (0 for raw SPI)
    unsigned long    DeviceSize;           // Total device size in bytes
    unsigned long    PageSize;             // Program-page size in bytes
    unsigned char    EraseValue;           // Value of a freshly erased byte (0xFF)
    struct DeviceSectors sectors[SECTOR_NUM];
};


#endif /* INC_DEV_INF_H_ */
