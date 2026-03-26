#include "flash_w25.h"
#include "include.h"
#include "spi.h"
#include "flash.h"
/* 坏把忘抄志快把 志扶快扮扶快抄 SPI NOR (Winbond W25Q 我 扼抉志技快扼找我技抑快). */
//////////////////////////////////////////////////////////////////////////////////

u16 W25QXX_TYPE = W25Q32; /* default until ReadID */

/* 4 KiB sector, 16 sectors per block; W25Q128: 16 MiB, 128 blocks, 4096 sectors */

void W25QXX_Init(void)
{

	SPI_CS(CS_FLASH, 1);
	W25QXX_TYPE = W25QXX_ReadID();
	/* Whitelist JEDEC IDs (cmd 0x90). Old code used "!= 0xFFFF" and accepted garbage on MISO. */
	module.flash = 0;
	switch (W25QXX_TYPE)
	{
	case W25Q80:
	case W25Q16:
	case W25Q32:
	case W25Q64:
	case W25Q128:
	case 0xC815:
		module.flash = 1;
		break;
	default:
		break;
	}
}

/* Status register W25QXX: SPR RV TB BP2 BP1 BP0 WEL BUSY */
u8 W25QXX_ReadSR(void)
{
	u8 byte = 0;
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_ReadStatusReg);
	byte = SPI3_RW(0Xff);
	SPI_CS(CS_FLASH, 1);
	return byte;
}
void W25QXX_Write_SR(u8 sr)
{
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_WriteStatusReg);
	SPI3_RW(sr);
	SPI_CS(CS_FLASH, 1);
}
void W25QXX_Write_Enable(void)
{
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_WriteEnable);
	SPI_CS(CS_FLASH, 1);
}
void W25QXX_Write_Disable(void)
{
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_WriteDisable);
	SPI_CS(CS_FLASH, 1);
}
/* JEDEC Manufacturer/Device ID */
u16 W25QXX_ReadID(void)
{
	u16 Temp = 0;
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(0x90);
	SPI3_RW(0x00);
	SPI3_RW(0x00);
	SPI3_RW(0x00);
	Temp |= SPI3_RW(0xFF) << 8;
	Temp |= SPI3_RW(0xFF);
	SPI_CS(CS_FLASH, 1);
	return Temp;
}
void W25QXX_Read(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
	u16 i;
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_ReadData);
	SPI3_RW((u8)((ReadAddr) >> 16));
	SPI3_RW((u8)((ReadAddr) >> 8));
	SPI3_RW((u8)ReadAddr);
	for (i = 0; i < NumByteToRead; i++)
	{
		pBuffer[i] = SPI3_RW(0XFF);
	}
	SPI_CS(CS_FLASH, 1);
}
void W25QXX_Write_Page(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u16 i;
	W25QXX_Write_Enable();
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_PageProgram);
	SPI3_RW((u8)((WriteAddr) >> 16));
	SPI3_RW((u8)((WriteAddr) >> 8));
	SPI3_RW((u8)WriteAddr);
	for (i = 0; i < NumByteToWrite; i++)
		SPI3_RW(pBuffer[i]);
	SPI_CS(CS_FLASH, 1);
	W25QXX_Wait_Busy();
}
void W25QXX_Write_NoCheck(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u16 pageremain;
	pageremain = 256 - WriteAddr % 256;
	if (NumByteToWrite <= pageremain)
		pageremain = NumByteToWrite;
	while (1)
	{
		W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
		if (NumByteToWrite == pageremain)
			break;
		else
		{
			pBuffer += pageremain;
			WriteAddr += pageremain;
			NumByteToWrite -= pageremain;
			if (NumByteToWrite > 256)
				pageremain = 256;
			else
				pageremain = NumByteToWrite;
		}
	};
}
u8 W25QXX_BUFFER[4096];
void W25QXX_Write(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u32 secpos;
	u16 secoff;
	u16 secremain;
	u16 i;
	u8 *W25QXX_BUF;
	W25QXX_BUF = W25QXX_BUFFER;
	secpos = WriteAddr / 4096;
	secoff = WriteAddr % 4096;
	secremain = 4096 - secoff;
	if (NumByteToWrite <= secremain)
		secremain = NumByteToWrite;
	while (1)
	{
		W25QXX_Read(W25QXX_BUF, secpos * 4096, 4096);
		for (i = 0; i < secremain; i++)
		{
			if (W25QXX_BUF[secoff + i] != 0XFF)
				break;
		}
		if (i < secremain)
		{
			W25QXX_Erase_Sector(secpos);
			for (i = 0; i < secremain; i++)
			{
				W25QXX_BUF[i + secoff] = pBuffer[i];
			}
			W25QXX_Write_NoCheck(W25QXX_BUF, secpos * 4096, 4096);
		}
		else
			W25QXX_Write_NoCheck(pBuffer, WriteAddr, secremain);
		if (NumByteToWrite == secremain)
			break;
		else
		{
			secpos++;
			secoff = 0;
			pBuffer += secremain;
			WriteAddr += secremain;
			NumByteToWrite -= secremain;
			if (NumByteToWrite > 4096)
				secremain = 4096;
			else
				secremain = NumByteToWrite;
		}
	};
}
void W25QXX_Erase_Chip(void)
{
	W25QXX_Write_Enable();
	W25QXX_Wait_Busy();
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_ChipErase);
	SPI_CS(CS_FLASH, 1);
	W25QXX_Wait_Busy();
}
void W25QXX_Erase_Sector(u32 Dst_Addr)
{
	Dst_Addr *= 4096;
	W25QXX_Write_Enable();
	W25QXX_Wait_Busy();
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_SectorErase);
	SPI3_RW((u8)((Dst_Addr) >> 16));
	SPI3_RW((u8)((Dst_Addr) >> 8));
	SPI3_RW((u8)Dst_Addr);
	SPI_CS(CS_FLASH, 1);
	W25QXX_Wait_Busy();
}
void W25QXX_Wait_Busy(void)
{
	while ((W25QXX_ReadSR() & 0x01) == 0x01)
		;
}
void W25QXX_PowerDown(void)
{
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_PowerDown);
	SPI_CS(CS_FLASH, 1);
	Delay_us(3);
}
void W25QXX_WAKEUP(void)
{
	SPI_CS(CS_FLASH, 0);
	SPI3_RW(W25X_ReleasePowerDown);
	SPI_CS(CS_FLASH, 1);
	Delay_us(3);
}
