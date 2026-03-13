#ifndef W25QXX_H
#define W25QXX_H

#include "stm32f4xx_hal.h"

// W25QXX IDs
#define W25Q80  0xEF13
#define W25Q16  0xEF14
#define W25Q32  0xEF15
#define W25Q64  0xEF16
#define W25Q128 0xEF17

extern uint16_t W25QXX_TYPE;

// Instructions
#define W25X_WriteEnable        0x06
#define W25X_WriteDisable       0x04
#define W25X_ReadStatusReg      0x05
#define W25X_WriteStatusReg     0x01
#define W25X_ReadData           0x03
#define W25X_FastReadData       0x0B
#define W25X_PageProgram        0x02
#define W25X_SectorErase        0x20
#define W25X_ChipErase          0xC7
#define W25X_DeviceID           0xAB
#define W25X_ManufactDeviceID   0x90
#define W25X_JedecDeviceID      0x9F

void W25QXX_Init(void);
uint16_t W25QXX_ReadID(void);
uint8_t W25QXX_ReadSR(void);
void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void W25QXX_Erase_Sector(uint32_t Dst_Addr);
void W25QXX_Wait_Busy(void);

#endif /* W25QXX_H */
