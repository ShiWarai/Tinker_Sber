#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "comm.h"
#include "spi_node.h"
#include "spi.h"
#include "sys_time.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>


// ps -ef | grep hardware_task
// kill －9 324
_MEMS mems;

// Глобальные переменные для многопоточности
pthread_mutex_t lock;
volatile int running = 1;  // Флаг для управления потоками

#define SPI_TEST 0

#define USE_USB 0
#define USE_SERIAL 0

#define EN_SPI_BIG 1
#define CAN_LINK_COMM_VER1 0
#define CAN_LINK_COMM_VER2 1//3 BLDC Param DIV
#if EN_SPI_BIG
#if CAN_LINK_COMM_VER1
    #define SPI_SEND_MAX  85
#else
    #define SPI_SEND_MAX  120+20+20//equal to stm32 spi send cnt
    // #define SPI_SEND_MAX 254
#endif
#else
#define SPI_SEND_MAX  40
#endif
#define EN_MULTI_THREAD 1
#define NO_THREAD 0
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MEM_SPI 0001
#define MEM_SIZE 2048

#if NO_THREAD&&!EN_MULTI_THREAD
static uint32_t speed = 150000*4;//3Mhz 死机
#define DELAY_SPI 250//us
#else
static uint32_t speed = 20000000;//odroid 20M  upbord 2M  raspberrypi 5M
#define DELAY_SPI 250//us
#endif
float spi_loss_cnt = 0;
int spi_connect = 0;
float mems_usb_loss_cnt = 0;
int mems_usb_connect = 0;

using namespace std;

_SPI_RX spi_rx;
_SPI_TX spi_tx;

uint8_t spi_tx_buf[SPI_BUF_SIZE] = {0};
uint8_t spi_rx_buf[SPI_BUF_SIZE] = {0};
int spi_tx_cnt_show=0;
int spi_tx_cnt = 0;

uint8_t usb_tx_buf[SPI_BUF_SIZE] = {0};
uint8_t usb_rx_buf[SPI_BUF_SIZE] = {0};
int usb_tx_cnt = 0;
int usb_rx_cnt = 0;

uint8_t tx[SPI_BUF_SIZE] = {};
uint8_t rx[ARRAY_SIZE(tx)] = {};


static void setDataInt_spi(int i)
{
    spi_tx_buf[spi_tx_cnt++] = ((i << 24) >> 24);
    spi_tx_buf[spi_tx_cnt++] = ((i << 16) >> 24);
    spi_tx_buf[spi_tx_cnt++] = ((i << 8) >> 24);
    spi_tx_buf[spi_tx_cnt++] = (i >> 24);
}

static void setDataFloat_spi(float f)
{
    int i = *(int *)&f;
    spi_tx_buf[spi_tx_cnt++] = ((i << 24) >> 24);
    spi_tx_buf[spi_tx_cnt++] = ((i << 16) >> 24);
    spi_tx_buf[spi_tx_cnt++] = ((i << 8) >> 24);
    spi_tx_buf[spi_tx_cnt++] = (i >> 24);
}

static void setDataFloat_spi_int(float f,float size)
{
    int16_t _temp;
    _temp=f*size;
    spi_tx_buf[spi_tx_cnt++] = BYTE1(_temp);
    spi_tx_buf[spi_tx_cnt++] = BYTE0(_temp);
}

static float floatFromData_spi(unsigned char *data, int *anal_cnt)
{
    int i = 0x00;
    i |= (*(data + *anal_cnt + 3) << 24);
    i |= (*(data + *anal_cnt + 2) << 16);
    i |= (*(data + *anal_cnt + 1) << 8);
    i |= (*(data + *anal_cnt + 0));

    *anal_cnt += 4;
    return *(float *)&i;
}

static float floatFromData_spi_int(unsigned char *data, int *anal_cnt, float size)
{
    float temp=0;
    temp=(float)((int16_t)(*(data + *anal_cnt + 0)<<8)|*(data + *anal_cnt + 1))/size;
    *anal_cnt += 2;
    return temp;
}

static char charFromData_spi(unsigned char *data, int *anal_cnt)
{
    int temp = *anal_cnt;
    *anal_cnt += 1;
    return *(data + temp);
}

static int intFromData_spi(unsigned char *data, int *anal_cnt)
{
    int i = 0x00;
    i |= (*(data + *anal_cnt + 3) << 24);
    i |= (*(data + *anal_cnt + 2) << 16);
    i |= (*(data + *anal_cnt + 1) << 8);
    i |= (*(data + *anal_cnt + 0));
    *anal_cnt += 4;
    return i;
}

float To_180_degrees(float x)
{
    return (x>180?(x-360):(x<-180?(x+360):x));
}

int slave_rx(uint8_t *data_buf, int num)//接收解码--------------from stm32
{
    static int cnt_err_sum=0;
    uint8_t id;
    uint8_t sum = 0;
    uint8_t i;
    uint8_t temp;
    int anal_cnt = 4;


    for (i = 0; i < (num - 1); i++) {
        sum += *(data_buf + i);
    }

    // for(i=0;i<num;i++)
    // {
    //     printf("0x%02x ",*(data_buf + i));
    // }
    // printf("\n");

    if (!(sum == *(data_buf + num - 1))){
        cnt_err_sum++;
        printf("SPI ERROR: sum err=%d sum_cal=0x%X sum=0x%X\n",
               cnt_err_sum, sum, *(data_buf + num - 1));
        return 0;
    }

    if (!(*(data_buf) == 0xFF && *(data_buf + 1) == 0xFB)){
        printf("SPI ERROR: Invalid header! Expected 0xFF 0xFB, got 0x%02X 0x%02X\n", 
               *(data_buf), *(data_buf + 1));
        return 0;
    }

    //printf("spi_rx_buf[2]=%d\n", *(data_buf + 2));
    if (*(data_buf + 2) == 26) //------------------old version---------- use now=======================2022/4/16
    {
        spi_loss_cnt = 0;
        if (spi_connect==0)
        {
            printf("Hardware::Hardware SPI-STM32 Link3-Sbus Yunzhuo!!!=%d!!!\n",spi_connect);
            spi_connect = 1;
        }

        // Защита доступа к spi_rx данным
        pthread_mutex_lock(&lock);
        
        spi_rx.att[0] = floatFromData_spi(spi_rx_buf, &anal_cnt);
        spi_rx.att[1] = floatFromData_spi(spi_rx_buf, &anal_cnt);
        spi_rx.att[2] = floatFromData_spi(spi_rx_buf, &anal_cnt);

        spi_rx.att_rate[0] = floatFromData_spi(spi_rx_buf, &anal_cnt);
        spi_rx.att_rate[1] = floatFromData_spi(spi_rx_buf, &anal_cnt);
        spi_rx.att_rate[2] = floatFromData_spi(spi_rx_buf, &anal_cnt);

        spi_rx.acc_b[0] = floatFromData_spi(spi_rx_buf, &anal_cnt);
        spi_rx.acc_b[1] = floatFromData_spi(spi_rx_buf, &anal_cnt);
        spi_rx.acc_b[2] = floatFromData_spi(spi_rx_buf, &anal_cnt);

        printf("Roll=%.2f, Pitch=%.2f, Yaw=%.2f, GyroX=%.2f, GyroY=%.2f, GyroZ=%.2f, AccX=%.2f, AccY=%.2f, AccZ=%.2f\n",
            spi_rx.att[0], spi_rx.att[1], spi_rx.att[2],
            spi_rx.att_rate[0], spi_rx.att_rate[1], spi_rx.att_rate[2],
            spi_rx.acc_b[0], spi_rx.acc_b[1], spi_rx.acc_b[2]);

        for (int i = 0; i < 10; i++)
        {
            // Проверка на переполнение буфера
            if (anal_cnt + 6 > num) { // 6 байт для q, dq, tau (по 2 байта) + 1 байт для temp
                printf("SPI ERROR: Buffer overflow in motor data parsing! anal_cnt=%d, num=%d, motor=%d\n", 
                       anal_cnt, num, i);
                break;
            }
            
            spi_rx.q[i] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,CAN_POS_DIV);
            spi_rx.dq[i] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,CAN_POS_DIV);
            spi_rx.tau[i] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,CAN_T_DIV);

            temp = charFromData_spi(spi_rx_buf, &anal_cnt);
            // printf("temp = %d\n", temp);
            spi_rx.connect_motor[i] = (temp % 100) / 10;
            spi_rx.ready[i] = temp % 10;

            if(spi_rx.connect_motor[i] == 1){
                printf("q[%d] = %f, dq[%d] = %f, tau[%d] = %f\n", i, spi_rx.q[i], i, spi_rx.dq[i], i, spi_rx.tau[i]);
            }
        }
        pthread_mutex_unlock(&lock);

    } 
    else {
        // Неизвестный ID пакета - игнорируем
        return 0;
    }
    // else if (*(data_buf + 2) == 36) { // Пока не используем
    //     spi_rx.bat_v[0] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.bat_v[0] =spi_rx.bat_v[1] =spi_rx.bat_v[2];

    //     //Sbus from STM32
    //     spi_rx.ocu.sbus_ch[0] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.ocu.sbus_ch[1] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.ocu.sbus_ch[2] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.ocu.sbus_ch[3] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.ocu.sbus_ch[4] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.ocu.sbus_ch[5] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);

    //     temp= charFromData_spi(spi_rx_buf, &anal_cnt);
    //     spi_rx.ocu.sbus_conncect = temp/100;
    //     spi_rx.ocu.sbus_aux[0] = (temp-int(temp/100) *100)/10;
    //     spi_rx.ocu.sbus_aux[1] = temp%10;

    //     temp= charFromData_spi(spi_rx_buf, &anal_cnt);
    //     // spi_rx.ocu.sbus_aux[0] = temp/100;
    //     spi_rx.ocu.sbus_aux[2] = (temp-int(temp/100) *100)/10;
    //     spi_rx.ocu.sbus_aux[3] = temp%10;

    //     temp= charFromData_spi(spi_rx_buf, &anal_cnt);
    //     // spi_rx.ocu.sbus_aux[0] = temp/100;
    //     spi_rx.ocu.sbus_aux[4] = (temp-int(temp/100) *100)/10;
    //     spi_rx.ocu.sbus_aux[5] = temp%10;

    //     spi_rx.aoa.dis= floatFromData_spi_int(spi_rx_buf, &anal_cnt,100);
    //     spi_rx.aoa.angle= floatFromData_spi_int(spi_rx_buf, &anal_cnt,10);
    //     spi_rx.aoa.rssi= charFromData_spi(spi_rx_buf, &anal_cnt);

    //     //--arm
    //     spi_rx.arm_cmd_s.pos_now.x = floatFromData_spi_int(spi_rx_buf, &anal_cnt,20);
    //     spi_rx.arm_cmd_s.pos_now.y = floatFromData_spi_int(spi_rx_buf, &anal_cnt,20);
    //     spi_rx.arm_cmd_s.pos_now.z = floatFromData_spi_int(spi_rx_buf, &anal_cnt,20);
    //     spi_rx.arm_cmd_s.att_now[0] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,20);
    //     spi_rx.arm_cmd_s.att_now[1] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,20);
    //     spi_rx.arm_cmd_s.att_now[2] = floatFromData_spi_int(spi_rx_buf, &anal_cnt,20);
    //      #if 0
    //     printf("%f %f %d\n",spi_rx.aoa.dis,spi_rx.aoa.angle,spi_rx.aoa.rssi);
    //     #endif
    //     #if 0
    //       printf("connect=%d st=%d back=%d x=%d a=%d\n",
    //           spi_rx.ocu.connect ,  spi_rx.ocu.key_st,spi_rx.ocu.key_back, spi_rx.ocu.key_x,spi_rx.ocu.key_a );
    //       printf("b=%d y=%d ll=%d rr=%d lr=%d up=%d\n",
    //           spi_rx.ocu.key_b ,  spi_rx.ocu.key_y,spi_rx.ocu.key_ll, spi_rx.ocu.key_rr,spi_rx.ocu.key_lr ,spi_rx.ocu.key_ud);
    //       printf("spd0=%f spd1=%f att0=%f att1=%f yaw=%f\n",
    //           spi_rx.ocu.rc_spd_w[0],spi_rx.ocu.rc_spd_w[1],spi_rx.ocu.rc_att_w[0],spi_rx.ocu.rc_att_w[1],spi_rx.ocu.rate_yaw_w);
    //     #endif
    //     #if 0
    //       printf("connect=%d st=%d back=%d x=%d a=%d\n",
    //           spi_rx.ocu.sbus_conncect ,  spi_rx.ocu.sbus_aux[0],spi_rx.ocu.sbus_aux[1], spi_rx.ocu.sbus_aux[2],spi_rx.ocu.sbus_aux[3] );
    //       printf("rc0=%f rc1=%f rc2=%f rc3=%f\n",
    //           spi_rx.ocu.sbus_ch[0],spi_rx.ocu.sbus_ch[1],spi_rx.ocu.sbus_ch[2],spi_rx.ocu.sbus_ch[3]);
    //     #endif
    // }
    return 1;
}

void can_board_send(char sel)//发送到单片机
{
    //printf("can_board_send sel=%d\n",sel);
    int i;
    static float t_temp = 0;
    char id = 0;
    char sum_t = 0, _cnt = 0;
    static char bldc_id_sel=0;
    spi_tx_cnt = 0;

    spi_tx_buf[spi_tx_cnt++] = 0xFE;
    spi_tx_buf[spi_tx_cnt++] = 0xFC;
    spi_tx_buf[spi_tx_cnt++] = sel;
    spi_tx_buf[spi_tx_cnt++] = 0;

    // Защита доступа к spi_tx и mems данным
    pthread_mutex_lock(&lock);

    switch (sel)
    {
        case 45:
            spi_tx_buf[spi_tx_cnt++] = spi_tx.en_motor*100+spi_tx.reset_q*10+spi_tx.reset_err; // Включение двигателей, установка нуля, сброс ошибки 

            spi_tx_buf[spi_tx_cnt++] =  mems.Acc_CALIBRATE*100+mems.Gyro_CALIBRATE*10+mems.Mag_CALIBRATE; // Калибровка

            spi_tx_buf[spi_tx_cnt++] =  spi_tx.beep_state;

            //printf("mems.en_motor=%d\n",spi_tx.en_motor);
            // printf("mems.reset_q=%d\n",spi_tx.reset_q);
            // printf("mems.reset_err=%d\n",spi_tx.reset_err);
            // printf("mems.Acc_CALIBRATE=%d\n",mems.Acc_CALIBRATE);
            // printf("mems.Gyro_CALIBRATE=%d\n",mems.Gyro_CALIBRATE);
            // printf("mems.Mag_CALIBRATE=%d\n",mems.Mag_CALIBRATE);
            // printf("spi_tx.beep_state=%d\n",spi_tx.beep_state);
            // spi_tx_buf[spi_tx_cnt++] = bldc_id_sel*100+spi_tx.led_enable[0]*10+spi_tx.led_enable[1];//bldc id sel

            //setDataFloat_spi_int( mems.imu_att.z,CAN_POS_DIV);
            
            

            //setDataFloat_spi_int(spi_tx.t_to_i,1000);

            for (int id = 0; id < 10; id++)
            {
                setDataFloat_spi_int(spi_tx.q_set[id], CAN_POS_DIV);
                setDataFloat_spi_int(spi_tx.dq_set[id],CAN_DPOS_DIV);
                setDataFloat_spi_int(spi_tx.tau_ff[id],CAN_T_DIV);
                setDataFloat_spi_int(spi_tx.kp,CAN_GAIN_DIV_P);
                setDataFloat_spi_int(spi_tx.kd,CAN_GAIN_DIV_D);
            }
        break;
        // case 50://发送OCU配置
        //     //--------------------Param From OCU-------------
        //     setDataFloat_spi_int( mems.imu_pos.x,1000);
        //     setDataFloat_spi_int( mems.imu_pos.y,1000);
        //     setDataFloat_spi_int( mems.imu_pos.z,1000);
        //     setDataFloat_spi_int( mems.imu_att.x,CAN_POS_DIV);
        //     setDataFloat_spi_int( mems.imu_att.y,CAN_POS_DIV);
        //     setDataFloat_spi_int( mems.imu_att.z,CAN_POS_DIV);
        //     setDataFloat_spi_int( mems.gps_pos.x,1000);
        //     setDataFloat_spi_int( mems.gps_pos.y,1000);
        //     setDataFloat_spi_int( mems.gps_pos.z,1000);
        //     spi_tx_buf[spi_tx_cnt++] =  mems.Acc_CALIBRATE;
        //     spi_tx_buf[spi_tx_cnt++] =  mems.Gyro_CALIBRATE;
        //     spi_tx_buf[spi_tx_cnt++] =  mems.Mag_CALIBRATE;

        // break;
        default:
            for (int id = 0; id < 10; id++)
            {
                setDataFloat_spi(0);
                setDataFloat_spi(0);
                setDataFloat_spi(0);
            }
        break;
    }

    spi_tx_buf[3] = (spi_tx_cnt)-4;
    for (i = 0; i < spi_tx_cnt; i++)
        sum_t += spi_tx_buf[i];
    spi_tx_buf[spi_tx_cnt++] = sum_t;

    // printf("spi_tx_cnt=%d\n",spi_tx_cnt);
    if(spi_tx_cnt>SPI_SEND_MAX)
       printf("spi_tx_cnt=%d over flow!!!\n",spi_tx_cnt);
    spi_tx_cnt_show=spi_tx_cnt;
    pthread_mutex_unlock(&lock);
}

void transfer(int fd, int sel)//发送
{
    static uint8_t state, rx_cnt;
    static uint8_t _data_len2 = 0, _data_cnt2 = 0;
    static int parser_timeout = 0;
    int ret;
    uint8_t data = 0;

    can_board_send(sel);
    
    // for(int i=0;i<spi_tx_cnt;i++)
    //     printf("0x%X ",spi_tx_buf[i]);
    // printf("\n");

    // printf("spi_tx_cnt=%d\n",spi_tx_cnt);
    ret=SPIDataRW(0,spi_tx_buf,rx,SPI_SEND_MAX); //向总线中写入7个数据
    //printf("after1\n");
    //printf("ret=%d cnt=%d\n",ret,spi_tx_cnt);
    

    if (ret < 1){
       printf("SPI ERROR: Reopen! ret=%d\n", ret);
       SPISetup(0,speed);//printf("can't send spi message\n");
    }
    else
    {
        //printf("ret=%d\n",ret);
        for (int i = 0; i < SPI_SEND_MAX; i++)
        {
            data = rx[i];
            parser_timeout++;
            
            // Таймаут парсера - сбрасываем состояние если слишком долго парсим
            if (parser_timeout > 1000) {
                state = 0;
                parser_timeout = 0;
            }
            
            if (state == 0 && data == 0xFF)
            {
                state = 1;
                spi_rx_buf[0] = data;
                parser_timeout = 0;
            }
            else if (state == 1 && data == 0xFB)
            {
                //printf("state 0 0x%X ",spi_rx_buf[0]);
                state = 2;
                spi_rx_buf[1] = data;
                parser_timeout = 0;
                //printf("state 1 0x%X ",data);
            }
            else if (state == 1 && data == 0xFF)
            {
                // Если получили еще один 0xFF, остаемся в состоянии 1
                // Это нормально, так как может быть много 0xFF подряд
                spi_rx_buf[0] = data;
                parser_timeout = 0;
            }
            else if (state == 2 && data > 0 && data < 0XF1)
            {
                state = 3;
                spi_rx_buf[2] = data;
                parser_timeout = 0;
                //printf("state 2 0x%X ",data);
            }
            else if (state == 3 && data < SPI_BUF_SIZE)
            {
                // Проверяем разумный размер пакета (должен быть ~111 байт)
                if (data < 50 || data > 150) {
                    state = 0;
                    parser_timeout = 0;
                    continue;
                }
                
                state = 4;
                spi_rx_buf[3] = data;
                _data_len2 = data;
                _data_cnt2 = 0;
                parser_timeout = 0;
                
                //printf("state 3 0x%X ",data);
            }
            else if (state == 4 && _data_len2 > 0)
            {
                _data_len2--;
                spi_rx_buf[4 + _data_cnt2++] = data;
                if (_data_len2 == 0) {
                    state = 5;
                    parser_timeout = 0;
                }
                //printf("state 4 0x%X ",data);
            }
            else if (state == 5)
            {
                state = 0;
                spi_rx_buf[4 + _data_cnt2] = data;
                parser_timeout = 0;
                
                slave_rx(spi_rx_buf, _data_cnt2 + 5);
                //printf("state 5 0x%X ",data);
            }
            else {
                
                // Если получили 0xFF, сразу переходим в состояние 1
                if (data == 0xFF) {
                    state = 1;
                    spi_rx_buf[0] = data;
                } else {
                    state = 0;
                }
            }
            // printf("%02x ",rx[i]); 
        }
        // printf("\n");
    }
    //printf("after2\n");
}

//----------------------------------------------------------------------------
int mem_connect=0;
float mem_loss_cnt=0;
struct shareMemory
{
    int  flag=0;  //作为一个标志，非0：表示可读，0表示可写
    unsigned char szMsg[MEM_SIZE];
};
struct shareMemory shareMemory_spi;

unsigned char mem_write_buf[MEM_SIZE];
unsigned char mem_read_buf[MEM_SIZE];
int mem_write_cnt=0;
static void setDataFloat_mem(float f)
{
    int i = *(int *)&f;
    mem_write_buf[mem_write_cnt++] = ((i << 24) >> 24);
    mem_write_buf[mem_write_cnt++] = ((i << 16) >> 24);
    mem_write_buf[mem_write_cnt++] = ((i << 8) >> 24);
    mem_write_buf[mem_write_cnt++] = (i >> 24);
}

static void setDataChar_mem(char f)
{
    mem_write_buf[mem_write_cnt++] = (f);
}

void memory_write(void)
{
    static float temp=0;
    mem_write_cnt=0;
    
    // Защита доступа к spi_rx данных
    pthread_mutex_lock(&lock);
    setDataFloat_mem( spi_rx.att[0]);
    setDataFloat_mem( spi_rx.att[1]);
    setDataFloat_mem( spi_rx.att[2]);
    setDataFloat_mem( spi_rx.att_rate[0]);
    setDataFloat_mem( spi_rx.att_rate[1]);
    setDataFloat_mem( spi_rx.att_rate[2]);
    setDataFloat_mem( spi_rx.acc_b[0]);
    setDataFloat_mem( spi_rx.acc_b[1]);
    setDataFloat_mem( spi_rx.acc_b[2]);

    // 10 двигателей вместо 4 ног × 3 мотора
    for (int i = 0; i < 10; i++)
    {
        setDataFloat_mem(spi_rx.q[i]);
        setDataFloat_mem(spi_rx.dq[i]);
        setDataFloat_mem(spi_rx.tau[i]);
    }

    // Напряжения батарей (оставляем 4, так как это системные)
    for (int i = 0; i < 4; i++)
    {
        setDataFloat_mem(spi_rx.bat_v[i]);
    }

    // Статус подключения для 10 двигателей
    for (int i = 0; i < 10; i++)
    {
        setDataChar_mem(spi_rx.connect_motor[i]);
    }

    // Готовность для 10 двигателей
    for (int i = 0; i < 10; i++)
    {
        setDataChar_mem(spi_rx.ready[i]);
    }
    pthread_mutex_unlock(&lock);
}

void memory_read(void){
    int mem_read_cnt=MEM_SIZE/2;

    printf("mem_read_cnt=%d\n",mem_read_cnt);

    // Защита доступа к spi_tx и mems данным
    pthread_mutex_lock(&lock);
    // 10 двигателей вместо 4 ног × 3 мотора
    for (int i = 0; i < 10; i++)
    {
        spi_tx.q_set[i] = floatFromData_spi(mem_read_buf, &mem_read_cnt);
        printf("q_set[%d]=%f\n",i,spi_tx.q_set[i]);

        spi_tx.dq_set[i] = floatFromData_spi(mem_read_buf, &mem_read_cnt);
        printf("dq_set[%d]=%f\n",i,spi_tx.dq_set[i]);

        spi_tx.tau_ff[i] = floatFromData_spi(mem_read_buf, &mem_read_cnt);
        printf("tau_ff[%d]=%f\n",i,spi_tx.tau_ff[i]);
    }

    spi_tx.kp= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    printf("kp=%f\n",spi_tx.kp);
    spi_tx.kd= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    printf("kd=%f\n",spi_tx.kd);

    
    spi_tx.en_motor= mem_read_buf[mem_read_cnt++];
    spi_tx.reset_q= mem_read_buf[mem_read_cnt++];
    spi_tx.reset_err= mem_read_buf[mem_read_cnt++];
    printf("en_motor=%d\n",spi_tx.en_motor);

    mems.Acc_CALIBRATE=charFromData_spi(mem_read_buf, &mem_read_cnt);
    mems.Gyro_CALIBRATE=charFromData_spi(mem_read_buf, &mem_read_cnt);
    mems.Mag_CALIBRATE=charFromData_spi(mem_read_buf, &mem_read_cnt);

    spi_tx.beep_state=charFromData_spi(mem_read_buf, &mem_read_cnt);
    pthread_mutex_unlock(&lock);

    // mems.imu_pos.x= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.imu_pos.y= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.imu_pos.z= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.imu_att.x= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.imu_att.y= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.imu_att.z= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.gps_pos.x= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.gps_pos.y= floatFromData_spi(mem_read_buf, &mem_read_cnt);
    // mems.gps_pos.z= floatFromData_spi(mem_read_buf, &mem_read_cnt);
}


// Обработчик сигналов для корректного завершения
void signal_handler(int sig) {
    printf("Hardware::Received signal %d, shutting down...\n", sig);
    running = 0;
}

void* Thread_Mem(void*)//内存管理线程
{
    static int cnt = 0;
    static int mem_init_cnt=0;
    float sys_dt = 0;
    int i=0,memory_update=0;
    int flag = 0;
    int link_cnt=0;
    
    printf("Hardware::Thread_Mem started\n");
    //共享内存
    int shmid_rx = shmget((key_t)MEM_SPI, sizeof(shareMemory_spi), 0666|IPC_CREAT);
    void *shm_rx = shmat(shmid_rx, (void*)0, 0);
    
    shareMemory *pshm_rx = (shareMemory*)shm_rx;
    pshm_rx->flag = 0;
    printf("Hardware::Memory Hardware attached at %p\n",shm_rx);

    while (running)
    {
        //共享内存写
        if(pshm_rx->flag == 0)
        {
            if(!mem_connect){
            mem_init_cnt++;
                if(mem_init_cnt>3){
                printf("Hardware::Memery Control Link=%d!!!\n",link_cnt++);
                mem_connect=1;
                }
            }
            mem_loss_cnt=0;
            printf("memory_write\n");
            memory_write();
            for(int k=0;k<MEM_SIZE/2-1;k++)
                pshm_rx->szMsg[k]=mem_write_buf[k];
            for(int k=MEM_SIZE/2;k<MEM_SIZE;k++)
                mem_read_buf[k]=pshm_rx->szMsg[k];
            memory_read();
            pshm_rx->flag = 1;
        }else{
            mem_loss_cnt+=sys_dt;
            mem_init_cnt=0;
        }

        if(mem_loss_cnt>1&&mem_connect==1){
            mem_connect=0;
            mem_loss_cnt=0;
            mem_init_cnt=0;
            pthread_mutex_lock(&lock);
            for (int i = 0; i < 10; i++){
                spi_tx.q_set[i] = spi_rx.q[i];
                spi_tx.tau_ff[i] = 0;
            }
            spi_tx.kp= spi_tx.ki= spi_tx.kd= spi_tx.en_motor= 0;
            spi_tx.kp_sw= spi_tx.ki_sw= spi_tx.kd_sw= spi_tx.en_motor= 0;
            spi_tx.kp_st= spi_tx.ki_st= spi_tx.kd_st= spi_tx.en_motor= 0;
            pthread_mutex_unlock(&lock);
            printf("Hardware::Memery Control Loss!!!\n");
        }

        usleep(500);
    }
    shmdt(shm_rx);  //失败返回-1，假设成功
    shmctl(shmid_rx, IPC_RMID, 0);  //
    return 0;
}


void* Thread_SPI(void*)//SPI管理线程
{
    static float timer_spi1 = 0,timer_spi2=0;
    static int cnt = 0;
    static int timer_1s=0;
    static int timer_1m=0;
    static int timer_1h=0;
    static int consecutive_errors = 0;
    static int max_consecutive_errors = 10;
    // printf("SDFDSFDFS");
    static float timer_cnt=0;
    float sys_dt = 0;
    int i=0,memory_update=0;
    int flag = 0;
    int fd = 0;
    char buf_mem[MEM_SIZE]={1,2};
    
    printf("Hardware::Thread_SPI started\n");
    Cycle_Time_Init();
    fd=SPISetup(0,speed); //初始化SPI通道0，并设置为最大速度500000hz
    if(fd==-1)
        printf("init spi failed!\n");

    while (running)
    {
        sys_dt = Get_Cycle_T(15);
        timer_cnt+=sys_dt;
        if(timer_cnt>1){
            timer_cnt=0;
            timer_1s++;
            if(timer_1s>60){timer_1s=0;
                timer_1m++;
            }
            if(timer_1m>60){timer_1m=0;
                timer_1h++;}

            printf("Hardware::SPI Still Online at hour-%d min-%d sec-%d spi_cnt=%d\n",timer_1h,timer_1m,timer_1s,spi_tx_cnt_show);
            // printf("\nIMUData_Packet.accelerometer_x is %f\n", IMUData_Packet.accelerometer_x);
        }

        spi_loss_cnt += sys_dt;
        if (spi_loss_cnt > 1.5&& spi_connect==1)
        {
            spi_loss_cnt = 0;
            spi_connect = 0;
            consecutive_errors++;
            printf("Hardware::Hardware SPI-STM32 Loss!!! consecutive_errors=%d\n", consecutive_errors);
            
            // Если слишком много ошибок подряд, попробуем переинициализировать SPI
            if (consecutive_errors >= max_consecutive_errors) {
                printf("Hardware::Too many consecutive errors, reinitializing SPI...\n");
                close(fd);
                usleep(100000); // 100ms задержка
                fd = SPISetup(0, speed);
                if (fd == -1) {
                    printf("Hardware::SPI reinitialization failed!\n");
                } else {
                    printf("Hardware::SPI reinitialized successfully\n");
                    consecutive_errors = 0;
                }
            }
        }
        else if (spi_connect == 1) {
            consecutive_errors = 0; // Сброс счетчика при успешной связи
        }
        
        //-------SPI CAN发送
        timer_spi1+= sys_dt;
        timer_spi2+= sys_dt;

        transfer(fd, 45);//3 bldc div 41
        
        usleep(DELAY_SPI);
    }
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t tida, tidb;
    int ret;
    
    // Установка обработчика сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Инициализация мьютекса
    pthread_mutex_init(&lock, NULL);

    // Создание потоков
    pthread_create(&tida, NULL, Thread_Mem, NULL);
    pthread_create(&tidb, NULL, Thread_SPI, NULL);

    printf("Hardware::Threads created successfully\n");

    // Ожидание завершения потоков
    printf("Hardware::Waiting for threads to finish...\n");
    pthread_join(tida, NULL);
    pthread_join(tidb, NULL);

    // Очистка ресурсов
    pthread_mutex_destroy(&lock);
    printf("Hardware::All threads finished, program exiting\n");

    return 0;
}
