#include "drivers/rpi_proto.h"
#include "drivers/can.h"
#include "drivers/icm20602.h"
#include "flash.h"
#include <string.h>

extern VMC_ALL vmc_all;
extern motor_measure_t motor_chassis[NUM_MOTORS];
extern _MEMS mems;

uint8_t spi_tx_buf[SPI_TX_BUF_SIZE];
uint8_t spi_rx_buf[SPI_RX_BUF_SIZE];
uint16_t spi_tx_cnt = 0;

uint32_t spi_rx_cnt_all = 0;
uint32_t sum_spi_err = 0;
uint8_t spi_master_connect_pi = 0;

static void setDataFloat_spi(float f)
{
  union {
    uint32_t i;
    float f;
  } u;

  u.f = f;
  spi_tx_buf[spi_tx_cnt++] = (u.i >> 0) & 0xFF;
  spi_tx_buf[spi_tx_cnt++] = (u.i >> 8) & 0xFF;
  spi_tx_buf[spi_tx_cnt++] = (u.i >> 16) & 0xFF;
  spi_tx_buf[spi_tx_cnt++] = (u.i >> 24) & 0xFF;
}

static float floatFromData_spi_int(uint8_t *data, int *anal_cnt, float size)
{
  int16_t val = (int16_t)((data[*anal_cnt] << 8) | data[*anal_cnt + 1]);
  *anal_cnt += 2;
  return (float)val / size;
}

static void setDataFloat_spi_int(float f, float size)
{
  int16_t temp = (int16_t)(f * size);
  spi_tx_buf[spi_tx_cnt++] = (temp >> 8) & 0xFF;
  spi_tx_buf[spi_tx_cnt++] = temp & 0xFF;
}

static uint8_t motor_en_last = 0;

void RPI_Protocol_LinkLost(void)
{
  spi_master_connect_pi = 0;
  motor_en_last = 0;
  leg_motor.motor_en = 0;
}

void RPI_Protocol_Parse(uint8_t *data, uint16_t num)
{
  uint8_t sum = 0;

  if (num < 5U) {
    return;
  }

  for (uint16_t i = 0U; i < (num - 1U); i++) {
    sum += data[i];
  }
  if (sum != data[num - 1U]) {
    sum_spi_err++;
    return;
  }

  if (!((data[0] == 0xFEU) && (data[1] == 0xFCU))) {
    return;
  }

  if (data[2] == 45U) {
    int anal_cnt = 4;
    uint8_t mode_bits;
    uint8_t motor_en_cmd;
    uint8_t reset_q_cmd;
    uint8_t cal_bits;

    spi_master_connect_pi = 1;
    spi_rx_cnt_all++;

    mode_bits = data[anal_cnt++];
    motor_en_cmd = mode_bits / 100U;
    reset_q_cmd = (uint8_t)((mode_bits - motor_en_cmd * 100U) / 10U);

    leg_motor.motor_en = (char)motor_en_cmd;
    leg_motor.reset_q = (char)reset_q_cmd;
    leg_motor.reset_err = (char)(mode_bits % 10U);

    if (motor_en_cmd != motor_en_last) {
      for (uint8_t i = 0U; i < NUM_MOTORS; i++) {
        (void)CAN_MIT_Motor_Mode_En(i, motor_en_cmd);
      }
      motor_en_last = motor_en_cmd;
    }

    if ((reset_q_cmd == 2U) && (motor_en_cmd == 0U)) {
      for (uint8_t i = 0U; i < NUM_MOTORS; i++) {
        if ((motor_chassis[i].reset_q == 0) && (motor_chassis[i].reset_q_lock == 0)) {
          motor_chassis[i].reset_q = 1;
        }
      }
    }

    cal_bits = data[anal_cnt++];
    lis3mdl.Acc_CALIBRATE = (char)(cal_bits / 100U);
    lis3mdl.Gyro_CALIBRATE = (char)((cal_bits - (uint8_t)(lis3mdl.Acc_CALIBRATE * 100)) / 10U);

    vmc_all.beep_state = data[anal_cnt++];

    for (uint8_t i = 0U; i < NUM_MOTORS; i++) {
      motor_chassis[i].set_q = floatFromData_spi_int(data, &anal_cnt, CAN_POS_DIV);
      motor_chassis[i].set_qd = floatFromData_spi_int(data, &anal_cnt, CAN_DPOS_DIV);
      motor_chassis[i].set_t = floatFromData_spi_int(data, &anal_cnt, CAN_T_DIV);
      motor_chassis[i].kp = floatFromData_spi_int(data, &anal_cnt, CAN_GAIN_DIV_P);
      motor_chassis[i].kd = floatFromData_spi_int(data, &anal_cnt, CAN_GAIN_DIV_D);
      motor_chassis[i].param.usb_cmd_mode = 1;
    }
  }
}

void RPI_Protocol_PrepareTX(uint8_t sel)
{
  spi_tx_cnt = 0;
  spi_tx_buf[spi_tx_cnt++] = 0xFF;
  spi_tx_buf[spi_tx_cnt++] = 0xFB;
  spi_tx_buf[spi_tx_cnt++] = sel;
  spi_tx_buf[spi_tx_cnt++] = 0;

  if (sel == 26U) {
    setDataFloat_spi(vmc_all.att[0]);
    setDataFloat_spi(vmc_all.att[1]);
    setDataFloat_spi(vmc_all.att[2]);

    setDataFloat_spi(vmc_all.rate[0]);
    setDataFloat_spi(vmc_all.rate[1]);
    setDataFloat_spi(vmc_all.rate[2]);

    setDataFloat_spi(vmc_all.acc_b[0]);
    setDataFloat_spi(vmc_all.acc_b[1]);
    setDataFloat_spi(vmc_all.acc_b[2]);

    for (uint8_t i = 0U; i < NUM_MOTORS; i++) {
      uint8_t motor_state;

      setDataFloat_spi_int(leg_motor.q_now[i], CAN_POS_DIV);
      setDataFloat_spi_int(leg_motor.qd_now[i], CAN_DPOS_DIV);
      setDataFloat_spi_int(leg_motor.t_now[i], CAN_T_DIV);

      motor_state = (uint8_t)(leg_motor.connect * 100 + leg_motor.connect_motor[i] * 10 + leg_motor.ready[i]);
      spi_tx_buf[spi_tx_cnt++] = motor_state;
    }
  }

  spi_tx_buf[3] = (uint8_t)(spi_tx_cnt - 4U);

  uint8_t sum = 0;
  for (uint16_t i = 0U; i < spi_tx_cnt; i++) {
    sum += spi_tx_buf[i];
  }
  spi_tx_buf[spi_tx_cnt++] = sum;
}
