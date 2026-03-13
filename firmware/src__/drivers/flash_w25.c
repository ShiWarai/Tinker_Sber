#include "flash_w25.h"
#include "board_pins.h"
#include <string.h>

uint16_t W25QXX_TYPE = W25Q64; // Default to W25Q64
extern SPI_HandleTypeDef hspi3;

#define W25QXX_CS_L() HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_RESET)
#define W25QXX_CS_H() HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET)

static uint8_t SPI_ReadWriteByte(uint8_t tx_data) {
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi3, &tx_data, &rx_data, 1, 100);
    return rx_data;
}

void W25QXX_Init(void) {
    W25QXX_CS_H();
    W25QXX_TYPE = W25QXX_ReadID();
}

uint16_t W25QXX_ReadID(void) {
    uint16_t Temp = 0;
    W25QXX_CS_L();
    SPI_ReadWriteByte(W25X_ManufactDeviceID);
    SPI_ReadWriteByte(0x00);
    SPI_ReadWriteByte(0x00);
    SPI_ReadWriteByte(0x00);
    Temp |= SPI_ReadWriteByte(0xFF) << 8;
    Temp |= SPI_ReadWriteByte(0xFF);
    W25QXX_CS_H();
    return Temp;
}

void W25QXX_Wait_Busy(void) {
    while ((W25QXX_ReadSR() & 0x01) == 0x01);
}

uint8_t W25QXX_ReadSR(void) {
    uint8_t byte = 0;
    W25QXX_CS_L();
    SPI_ReadWriteByte(W25X_ReadStatusReg);
    byte = SPI_ReadWriteByte(0xFF);
    W25QXX_CS_H();
    return byte;
}

void W25QXX_Write_Enable(void) {
    W25QXX_CS_L();
    SPI_ReadWriteByte(W25X_WriteEnable);
    W25QXX_CS_H();
}

void W25QXX_Erase_Sector(uint32_t Dst_Addr) {
    W25QXX_Write_Enable();
    W25QXX_Wait_Busy();
    W25QXX_CS_L();
    SPI_ReadWriteByte(W25X_SectorErase);
    SPI_ReadWriteByte((uint8_t)((Dst_Addr) >> 16));
    SPI_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI_ReadWriteByte((uint8_t)Dst_Addr);
    W25QXX_CS_H();
    W25QXX_Wait_Busy();
}

void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead) {
    W25QXX_CS_L();
    SPI_ReadWriteByte(W25X_ReadData);
    SPI_ReadWriteByte((uint8_t)((ReadAddr) >> 16));
    SPI_ReadWriteByte((uint8_t)((ReadAddr) >> 8));
    SPI_ReadWriteByte((uint8_t)ReadAddr);
    for (uint16_t i = 0; i < NumByteToRead; i++) {
        pBuffer[i] = SPI_ReadWriteByte(0xFF);
    }
    W25QXX_CS_H();
}

void W25QXX_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    W25QXX_Write_Enable();
    W25QXX_CS_L();
    SPI_ReadWriteByte(W25X_PageProgram);
    SPI_ReadWriteByte((uint8_t)((WriteAddr) >> 16));
    SPI_ReadWriteByte((uint8_t)((WriteAddr) >> 8));
    SPI_ReadWriteByte((uint8_t)WriteAddr);
    for (uint16_t i = 0; i < NumByteToWrite; i++) {
        SPI_ReadWriteByte(pBuffer[i]);
    }
    W25QXX_CS_H();
    W25QXX_Wait_Busy();
}

// Simplified Write (assumes area is erased if needed, like in legacy)
void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    // For simplicity in this port, we don't implement the full cross-sector logic 
    // but follow the legacy's usage which was limited to a specific small range.
    // Legacy used W25QXX_Write directly for the parameter buffer.
    W25QXX_Erase_Sector(WriteAddr); // Minimum erase unit
    uint16_t pageremain = 256 - WriteAddr % 256;
    if (NumByteToWrite <= pageremain) pageremain = NumByteToWrite;
    while (1) {
        W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
        if (NumByteToWrite == pageremain) break;
        else {
            pBuffer += pageremain;
            WriteAddr += pageremain;
            NumByteToWrite -= pageremain;
            if (NumByteToWrite > 256) pageremain = 256;
            else pageremain = NumByteToWrite;
        }
    }
}
