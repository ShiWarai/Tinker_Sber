#include "include.h"
#include "stm32f4xx_hal.h"
#include "system_init.h"
#include "led.h"
#include "beep.h"
#include "can.h"
#include "Custom_SPI_Device.h"
#include "app_main.h"
#include "flash_w25.h"
#include "flash.h"
#include "watch_dog.h"
#include "drivers/icm20602.h"
#include "rpi_proto.h"
#include "sensors/imu.h"
#include "math/gait_math.h"
#include "cmsis_os2.h"

#define APP_USE_IMU          0
#define APP_USE_FLASH_PARAM  0

extern void MX_SPI3_Init(void);

static void main_loop(void)
{
  uint32_t last_1ms_tick = HAL_GetTick();
  uint32_t last_10ms_tick = HAL_GetTick();

  for (;;) {
    uint32_t current_tick = HAL_GetTick();

    // 1ms Task: AHRS and Control
    if (current_tick - last_1ms_tick >= 1) {
      last_1ms_tick = current_tick;

    #if APP_USE_IMU
      icm20602_read();
      IMU_update(0.001f,
             lis3mdl.Gyro.x, lis3mdl.Gyro.y, lis3mdl.Gyro.z,
             lis3mdl.Acc.x, lis3mdl.Acc.y, lis3mdl.Acc.z);
    #else
      lis3mdl.Gyro.x = 0.0f; lis3mdl.Gyro.y = 0.0f; lis3mdl.Gyro.z = 0.0f;
      lis3mdl.Acc.x = 0.0f;  lis3mdl.Acc.y = 0.0f;  lis3mdl.Acc.z = 0.0f;
      mems.imu_att.x = 0.0f; mems.imu_att.y = 0.0f; mems.imu_att.z = 0.0f;
    #endif
                 
      // Filtered attitude for VMC (example cutoff 15Hz)
      static float pitch_f, roll_f, yaw_f;
      DigitalLPF(mems.imu_att.x, &pitch_f, 15.0f, 0.001f);
      DigitalLPF(mems.imu_att.y, &roll_f, 15.0f, 0.001f);
      DigitalLPF(mems.imu_att.z, &yaw_f, 15.0f, 0.001f);

      // Apply calibration offsets and feed target bias if necessary
      vmc_all.att[0] = pitch_f + vmc_all.tar_att_bias[0];
      vmc_all.att[1] = roll_f + vmc_all.tar_att_bias[1];
      vmc_all.att[2] = yaw_f + vmc_all.tar_att_bias[2];

      vmc_all.rate[0] = lis3mdl.Gyro.x;
      vmc_all.rate[1] = lis3mdl.Gyro.y;
      vmc_all.rate[2] = lis3mdl.Gyro.z;

      // Update Rotation Matrices (Body <-> World)
      update_rotation_matrices(vmc_all.att[0], vmc_all.att[1], vmc_all.att[2], 
                               vmc_all.Rn_b, vmc_all.Rb_n);
      
      // Transform Body Acceleration to World Frame
      vmc_all.acc_b[0] = lis3mdl.Acc.x;
      vmc_all.acc_b[1] = lis3mdl.Acc.y;
      vmc_all.acc_b[2] = lis3mdl.Acc.z;

      // Accel linear transform example: an[i] = Sum(Rb_n[i][j] * ab[j])
      for(int i=0; i<3; i++) {
        vmc_all.acc_n[i] = 0;
        for(int j=0; j<3; j++) {
            vmc_all.acc_n[i] += vmc_all.Rn_b[i][j] * vmc_all.acc_b[j];
        }
      }

      // Motor Control Bridge (RPI -> STM32 -> CAN)
      static uint32_t spi_loss_cnt = 0;
        static uint32_t last_spi_rx_cnt = 0;
      
        if (spi_master_connect_pi && (spi_rx_cnt_all != last_spi_rx_cnt)) {
          last_spi_rx_cnt = spi_rx_cnt_all;
          spi_loss_cnt = 0;
        } else if (spi_loss_cnt <= 100) {
          spi_loss_cnt++;
        }

        if (spi_master_connect_pi && (spi_loss_cnt <= 100)) {
          for(int i=0; i<10; i++) {
            CAN_Send_MIT(&motor_chassis[i]);
          }
        } else if (spi_loss_cnt > 100) {
          RPI_Protocol_LinkLost();
          for(int i=0; i<10; i++) {
            CAN_MIT_Motor_Mode_En(i, 0); // Disable motors for safety
          }
          spi_loss_cnt = 101;
      }

      // Таймауты CAN и стейт-машина сброса позиции
      CAN_motor_sm(0.001f);
    }

    // 10ms Task: System monitoring
    if (current_tick - last_10ms_tick >= 10) {
      last_10ms_tick = current_tick;
      static uint8_t last_beep_cmd = 0U;

      if ((vmc_all.beep_state != 0U) && (vmc_all.beep_state != last_beep_cmd)) {
        beep_play_event(vmc_all.beep_state);
        last_beep_cmd = vmc_all.beep_state;
      } else if (vmc_all.beep_state == 0U) {
        last_beep_cmd = 0U;
      }

      beep_update(0.01f);
      
      led_status_update(0.01f);
      button_dog_read();
      watchdog_feed();
    }
  }
}

void app_main(void)
{
  led_init();
  beep_init();
  MX_SPI3_Init();
#if APP_USE_FLASH_PARAM
  W25QXX_Init();
  READ_PARM();
#endif

#if APP_USE_IMU
  if (icm20602_init() != 0) {
    beep_play_event(MEMS_ERROR_BEEP);
  }
#endif

  can_init();
  custom_spi_init();
  custom_spi_start();
  beep_startup_melody();
  watchdog_init(4000); // 4 seconds timeout

  osKernelInitialize();

  const osThreadAttr_t main_attr = {
    .name       = "MainLoop",
    .priority   = osPriorityNormal,
    .stack_size = 1024
  };
  osThreadNew((osThreadFunc_t)main_loop, NULL, &main_attr);

  osKernelStart();
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  app_main();
  for (;;) {}
}
