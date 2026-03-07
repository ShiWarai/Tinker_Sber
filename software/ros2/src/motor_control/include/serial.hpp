/*************************************************************
    FileName : serialport.h
    FileFunc : Определение заголовочного файла
    Version  : V0.1
    Author   : Sunrier
    Date     : 2012-06-13
    Descp    : Реализация работы с последовательным портом в Linux
*************************************************************/
#ifndef   _SERIALPORT_H_
#define   _SERIALPORT_H_


int open_port(int iPortNumber);
int set_port(int fd,int iBaudRate,int iDataSize,char cParity,int iStopBit);
int read_port(int fd,void *buf,int iByte);
int write_port(int fd,void *buf,int iByte);
int close_port(int fd);


#endif
