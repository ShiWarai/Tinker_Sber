/**
 * @file motor_control.cpp
 * @brief Протокол SPI и формирование пакетов для моторов и IMU
 */

#include "motor_control.hpp"
#include "spi.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EN_SPI_BIG 1
#define CAN_LINK_COMM_VER1 0
#define CAN_LINK_COMM_VER2 1

#if EN_SPI_BIG
#if CAN_LINK_COMM_VER1
#define SPI_SEND_MAX 85
#else
#define SPI_SEND_MAX 160  /* 120 + 20 + 20 байт */
#endif
#else
#define SPI_SEND_MAX 40
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#if !defined(NO_THREAD) || defined(EN_MULTI_THREAD)
static uint32_t g_speed = 20000000;
#else
static uint32_t g_speed = 150000 * 4;
#endif

static float spi_loss_cnt = 0;
static int spi_connect = 0;

static uint8_t spi_tx_buf[SPI_BUF_SIZE] = {0};
static uint8_t spi_rx_buf[SPI_BUF_SIZE] = {0};
static int spi_tx_cnt = 0;
static uint8_t rx_buf[SPI_BUF_SIZE] = {0};

static void setDataFloat_spi(float f)
{
    int i = *reinterpret_cast<int *>(&f);
    spi_tx_buf[spi_tx_cnt++] = ((i << 24) >> 24);
    spi_tx_buf[spi_tx_cnt++] = ((i << 16) >> 24);
    spi_tx_buf[spi_tx_cnt++] = ((i << 8) >> 24);
    spi_tx_buf[spi_tx_cnt++] = (i >> 24);
}

static void setDataFloat_spi_int(float f, float size)
{
    int16_t t = static_cast<int16_t>(f * size);
    spi_tx_buf[spi_tx_cnt++] = BYTE1(t);
    spi_tx_buf[spi_tx_cnt++] = BYTE0(t);
}

static float floatFromData_spi(unsigned char *data, int *anal_cnt)
{
    int i = 0;
    i |= (*(data + *anal_cnt + 3) << 24);
    i |= (*(data + *anal_cnt + 2) << 16);
    i |= (*(data + *anal_cnt + 1) << 8);
    i |= (*(data + *anal_cnt + 0));
    *anal_cnt += 4;
    return *reinterpret_cast<float *>(&i);
}

static float floatFromData_spi_int(unsigned char *data, int *anal_cnt, float size)
{
    float temp = static_cast<float>((static_cast<int16_t>(*(data + *anal_cnt + 0) << 8) | *(data + *anal_cnt + 1))) / size;
    *anal_cnt += 2;
    return temp;
}

static char charFromData_spi(unsigned char *data, int *anal_cnt)
{
    int temp = *anal_cnt;
    *anal_cnt += 1;
    return *(data + temp);
}

namespace motor_control {

int slave_rx(uint8_t *data_buf, int num, _SPI_RX &rx_out)
{
    static int cnt_err_sum = 0;
    uint8_t sum = 0;
    uint8_t temp;
    int anal_cnt = 4;

    for (uint8_t i = 0; i < static_cast<uint8_t>(num - 1); i++)
        sum += *(data_buf + i);

    if (sum != *(data_buf + num - 1))
    {
        cnt_err_sum++;
        printf("SPI ERROR: sum err=%d sum_cal=0x%X sum=0x%X\n", cnt_err_sum, sum, *(data_buf + num - 1));
        return 0;
    }

    if (*(data_buf) != 0xFF || *(data_buf + 1) != 0xFB)
    {
        printf("SPI ERROR: Invalid header! Expected 0xFF 0xFB, got 0x%02X 0x%02X\n", *(data_buf), *(data_buf + 1));
        return 0;
    }

    if (*(data_buf + 2) == 26)
    {
        spi_loss_cnt = 0;
        spi_connect = 1;

        rx_out.att[0] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.att[1] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.att[2] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.att_rate[0] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.att_rate[1] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.att_rate[2] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.acc_b[0] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.acc_b[1] = floatFromData_spi(data_buf, &anal_cnt);
        rx_out.acc_b[2] = floatFromData_spi(data_buf, &anal_cnt);

        for (int i = 0; i < 10; i++)
        {
            if (anal_cnt + 6 > num)
            {
                printf("SPI ERROR: Buffer overflow in motor data parsing! anal_cnt=%d, num=%d, motor=%d\n", anal_cnt, num, i);
                break;
            }
            rx_out.q[i] = floatFromData_spi_int(data_buf, &anal_cnt, CAN_POS_DIV);
            rx_out.dq[i] = floatFromData_spi_int(data_buf, &anal_cnt, CAN_POS_DIV);
            rx_out.tau[i] = floatFromData_spi_int(data_buf, &anal_cnt, CAN_T_DIV);
            temp = charFromData_spi(data_buf, &anal_cnt);
            // Статус-байт: connect*100 + connect_motor*10 + ready (см. MotorState.msg)
            rx_out.connect_motor[i] = (temp % 100) / 10;
            rx_out.ready[i] = temp % 10;
        }
    }
    else
        return 0;

    return 1;
}

void can_board_send(char sel, const _SPI_TX &tx_data, const _MEMS &mems_data)
{
    int i;
    char sum_t = 0;
    spi_tx_cnt = 0;

    spi_tx_buf[spi_tx_cnt++] = 0xFE;
    spi_tx_buf[spi_tx_cnt++] = 0xFC;
    spi_tx_buf[spi_tx_cnt++] = sel;
    spi_tx_buf[spi_tx_cnt++] = 0;

    spi_tx_buf[spi_tx_cnt++] = tx_data.en_motor * 100 + (tx_data.reset_q * 2) * 10 + tx_data.reset_err;
    spi_tx_buf[spi_tx_cnt++] = mems_data.Acc_CALIBRATE * 100 + mems_data.Gyro_CALIBRATE * 10 + mems_data.Mag_CALIBRATE;
    spi_tx_buf[spi_tx_cnt++] = tx_data.beep_state;
    for (int id = 0; id < 10; id++)
    {
        setDataFloat_spi_int(tx_data.q_set[id], CAN_POS_DIV);
        setDataFloat_spi_int(tx_data.dq_set[id], CAN_DPOS_DIV);
        setDataFloat_spi_int(tx_data.tau_ff[id], CAN_T_DIV);
        setDataFloat_spi_int(tx_data.kp[id], CAN_GAIN_DIV_P);
        setDataFloat_spi_int(tx_data.kd[id], CAN_GAIN_DIV_D);
    }

    spi_tx_buf[3] = static_cast<uint8_t>(spi_tx_cnt - 4);
    for (i = 0; i < spi_tx_cnt; i++)
        sum_t += spi_tx_buf[i];
    spi_tx_buf[spi_tx_cnt++] = sum_t;

    if (spi_tx_cnt > SPI_SEND_MAX)
        printf("spi_tx_cnt=%d over flow!!!\n", spi_tx_cnt);
}

int spi_transfer_and_parse(int sel, const _SPI_TX &tx_data, const _MEMS &mems_data, _SPI_RX &rx_out)
{
    static uint8_t state = 0;
    static uint8_t _data_len2 = 0;
    static uint8_t _data_cnt2 = 0;
    static int parser_timeout = 0;

    can_board_send(sel, tx_data, mems_data);
    int ret = SPIDataRW(0, spi_tx_buf, rx_buf, SPI_SEND_MAX);

    if (ret >= 1)
    {
        for (int i = 0; i < SPI_SEND_MAX; i++)
        {
            uint8_t data = rx_buf[i];
            parser_timeout++;

            if (parser_timeout > 1000)
            {
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
                state = 2;
                spi_rx_buf[1] = data;
                parser_timeout = 0;
            }
            else if (state == 1 && data == 0xFF)
            {
                spi_rx_buf[0] = data;
                parser_timeout = 0;
            }
            else if (state == 2 && data > 0 && data < 0xF1)
            {
                state = 3;
                spi_rx_buf[2] = data;
                parser_timeout = 0;
            }
            else if (state == 3 && data < SPI_BUF_SIZE)
            {
                if (data < 50 || data > 150)
                {
                    state = 0;
                    parser_timeout = 0;
                    continue;
                }
                state = 4;
                spi_rx_buf[3] = data;
                _data_len2 = data;
                _data_cnt2 = 0;
                parser_timeout = 0;
            }
            else if (state == 4 && _data_len2 > 0)
            {
                _data_len2--;
                spi_rx_buf[4 + _data_cnt2++] = data;
                if (_data_len2 == 0)
                {
                    state = 5;
                    parser_timeout = 0;
                }
            }
            else if (state == 5)
            {
                state = 0;
                spi_rx_buf[4 + _data_cnt2] = data;
                parser_timeout = 0;
                slave_rx(spi_rx_buf, _data_cnt2 + 5, rx_out);
            }
            else
            {
                if (data == 0xFF)
                {
                    state = 1;
                    spi_rx_buf[0] = data;
                }
                else
                    state = 0;
            }
        }
    }

    return ret;
}

uint32_t get_spi_speed()
{
    return g_speed;
}

} // namespace motor_control
