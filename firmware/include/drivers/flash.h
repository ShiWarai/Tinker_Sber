#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "include.h"
#include "stm32f4xx.h"

extern void READ_PARM(void);
extern void WRITE_PARM(void);
 
extern float SONAR_HEIGHT;
extern u16 LENGTH_OF_DRONE;
extern int H_INT;
extern u8 need_init_mems;
extern float k_sensitivity[3];//?§Ø?

extern u16 SBUS_MIN ;//=954;
extern u16 SBUS_MAX ;//=2108;
extern u16 SBUS_MID ;//=1524;
extern u16 SBUS_MIN_A ;//=954;
extern u16 SBUS_MAX_A ;//=2108;
extern u16 SBUS_MID_A ;//=1524;

//W25X???/Q???§à??§Ò?	   
#define W25Q80 	0XEF13 	
#define W25Q16 	0XEF14
#define W25Q32 	0XEF15
#define W25Q64 	0XEF16
#define W25Q128	0XEF17

extern u16 W25QXX_TYPE;					//????W25QXX§à????		   
#define	W25QXX_CS 		PCout(5)  		//W25QXX???????
////////////////////////////////////////////////////////////////////////////////// 
//????
#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_BlockErase			0xD8 
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 

void W25QXX_Init(void);
u16  W25QXX_ReadID(void);  	    		//¶ÁÈ¡FLASH ID
u8	 W25QXX_ReadSR(void);        		//¶ÁÈ¡×´Ì¬¼Ä´æÆ÷ 
void W25QXX_Write_SR(u8 sr);  			//Ð´×´Ì¬¼Ä´æÆ÷
void W25QXX_Write_Enable(void);  		//Ð´Ê¹ÄÜ 
void W25QXX_Write_Disable(void);		//Ð´±£»¤
void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);
void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead);   //¶ÁÈ¡flash
void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);//Ð´Èëflash
void W25QXX_Erase_Chip(void);    	  	//ÕûÆ¬²Á³ý
void W25QXX_Erase_Sector(u32 Dst_Addr);	//ÉÈÇø²Á³ý
void W25QXX_Wait_Busy(void);           	//µÈ´ý¿ÕÏÐ
void W25QXX_PowerDown(void);        	//½øÈëµôµçÄ£Ê½
void W25QXX_WAKEUP(void);				//»½ÐÑ

#endif
