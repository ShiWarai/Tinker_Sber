/**
 * @file Motor_control.cpp
 * @brief ROS2 узел для управления моторами через SPI интерфейс
 *
 * @details
 * Реализует взаимодействие между ROS2 и аппаратным обеспечением через SPI.
 * Обеспечивает:
 * - Прием команд управления моторами из ROS2 топиков
 * - Передачу данных на контроллер двигателей через SPI
 * - Прием данных с IMU и энкодеров двигателей
 * - Публикацию состояний двигателей и IMU данных
 * - Визуализацию в RViz через JointState
 */

#include <memory>
#include <atomic>
#include <functional>
#include <chrono>
#include <mutex>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/bool.hpp"
// hardware_msg messages
#include "hardware_msg/msg/board_parameters.hpp"
#include "hardware_msg/msg/imu.hpp"
#include "hardware_msg/msg/imu_parameters.hpp"
#include "hardware_msg/msg/motor_data.hpp"
#include "hardware_msg/msg/motor_parameters.hpp"
#include "hardware_msg/msg/motors_commands.hpp"
#include "hardware_msg/msg/motors_states.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/time.h>
#include <math.h>
#include <time.h>
#include "comm.h"
#include "spi_node.h"
#include "spi.h"
#include "sys_time.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define SPI_TEST 0
#define USE_USB 0
#define USE_SERIAL 0
#define EN_SPI_BIG 1
#define CAN_LINK_COMM_VER1 0
#define CAN_LINK_COMM_VER2 1

#if EN_SPI_BIG
#if CAN_LINK_COMM_VER1
#define SPI_SEND_MAX 85
#else
#define SPI_SEND_MAX 160 // 120 + 20 + 20
#endif
#else
#define SPI_SEND_MAX 40
#endif

#define EN_MULTI_THREAD 1
#define NO_THREAD 0
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MEM_SPI 0001
#define MEM_SIZE 2048

#if NO_THREAD && !EN_MULTI_THREAD
static uint32_t speed = 150000 * 4;
#define DELAY_SPI 250
#else
static uint32_t speed = 20000000;
#define DELAY_SPI 250
#endif

float spi_loss_cnt = 0;
int spi_connect = 0;
float mems_usb_loss_cnt = 0;
int mems_usb_connect = 0;

using namespace std;

// Глобальные буферы SPI (остаются, т.к. используются в низкоуровневом SPI)
uint8_t spi_tx_buf[SPI_BUF_SIZE] = {0};
uint8_t spi_rx_buf[SPI_BUF_SIZE] = {0};
int spi_tx_cnt_show = 0;
int spi_tx_cnt = 0;

uint8_t usb_tx_buf[SPI_BUF_SIZE] = {0};
uint8_t usb_rx_buf[SPI_BUF_SIZE] = {0};
int usb_tx_cnt = 0;
int usb_rx_cnt = 0;

uint8_t tx[SPI_BUF_SIZE] = {};
uint8_t rx[ARRAY_SIZE(tx)] = {};

// Вспомогательные функции сериализации 
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

static void setDataFloat_spi_int(float f, float size)
{
    int16_t _temp = static_cast<int16_t>(f * size);
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
    float temp = (float)((int16_t)(*(data + *anal_cnt + 0) << 8) | *(data + *anal_cnt + 1)) / size;
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
    return (x > 180 ? (x - 360) : (x < -180 ? (x + 360) : x));
}

// Обновлённая функция: принимает выходной буфер
int slave_rx(uint8_t *data_buf, int num, _SPI_RX &rx_out)
{
    static int cnt_err_sum = 0;
    uint8_t sum = 0;
    uint8_t i;
    uint8_t temp;
    int anal_cnt = 4;

    for (i = 0; i < (num - 1); i++)
    {
        sum += *(data_buf + i);
    }

    if (!(sum == *(data_buf + num - 1)))
    {
        cnt_err_sum++;
        printf("SPI ERROR: sum err=%d sum_cal=0x%X sum=0x%X\n",
               cnt_err_sum, sum, *(data_buf + num - 1));
        return 0;
    }

    if (!(*(data_buf) == 0xFF && *(data_buf + 1) == 0xFB))
    {
        printf("SPI ERROR: Invalid header! Expected 0xFF 0xFB, got 0x%02X 0x%02X\n",
               *(data_buf), *(data_buf + 1));
        return 0;
    }

    if (*(data_buf + 2) == 26)
    {
        spi_loss_cnt = 0;
        if (spi_connect == 0)
        {
            printf("Hardware::Hardware SPI-STM32 Link3-Sbus Yunzhuo!!!=%d!!!\n", spi_connect);
            spi_connect = 1;
        }

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
                printf("SPI ERROR: Buffer overflow in motor data parsing! anal_cnt=%d, num=%d, motor=%d\n",
                       anal_cnt, num, i);
                break;
            }

            rx_out.q[i] = floatFromData_spi_int(data_buf, &anal_cnt, CAN_POS_DIV);
            rx_out.dq[i] = floatFromData_spi_int(data_buf, &anal_cnt, CAN_POS_DIV);
            rx_out.tau[i] = floatFromData_spi_int(data_buf, &anal_cnt, CAN_T_DIV);

            temp = charFromData_spi(data_buf, &anal_cnt);
            rx_out.connect_motor[i] = (temp % 100) / 10;
            rx_out.ready[i] = temp % 10;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

// Обновлённая функция: принимает tx_data и mems_data
void can_board_send(char sel, const _SPI_TX &tx_data, const _MEMS &mems_data)
{
    int i;
    char sum_t = 0;
    spi_tx_cnt = 0;

    spi_tx_buf[spi_tx_cnt++] = 0xFE;
    spi_tx_buf[spi_tx_cnt++] = 0xFC;
    spi_tx_buf[spi_tx_cnt++] = sel;
    spi_tx_buf[spi_tx_cnt++] = 0;

    switch (sel)
    {
    case 45:
        spi_tx_buf[spi_tx_cnt++] = tx_data.en_motor * 100 + (tx_data.reset_q * 2)* 10 + tx_data.reset_err;
        spi_tx_buf[spi_tx_cnt++] = mems_data.Acc_CALIBRATE * 100 + mems_data.Gyro_CALIBRATE * 10 + mems_data.Mag_CALIBRATE;
        spi_tx_buf[spi_tx_cnt++] = tx_data.beep_state;

        for (int id = 0; id < 10; id++)
        {
            setDataFloat_spi_int(tx_data.q_set[id], CAN_POS_DIV);
            setDataFloat_spi_int(tx_data.dq_set[id], CAN_DPOS_DIV);
            setDataFloat_spi_int(tx_data.tau_ff[id], CAN_T_DIV);
            setDataFloat_spi_int(tx_data.kp, CAN_GAIN_DIV_P);
            setDataFloat_spi_int(tx_data.kd, CAN_GAIN_DIV_D);
        }
        break;

    default:
        // Заполняем нулями, если нужно
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

    if (spi_tx_cnt > SPI_SEND_MAX)
        printf("spi_tx_cnt=%d over flow!!!\n", spi_tx_cnt);
    spi_tx_cnt_show = spi_tx_cnt;
}

class Motor_control : public rclcpp::Node
{
public:
    Motor_control()
        : rclcpp::Node("dual_io_node"),
          mems_{} // инициализация нулями
    {
        RCLCPP_INFO(this->get_logger(), "Hardware::Thread_SPI started");

        Cycle_Time_Init();
        fd = SPISetup(0, speed);

        if (fd == -1)
        {
            RCLCPP_ERROR(this->get_logger(), "init spi failed!");
            return;
        }

        // Инициализация издателей
        imu_pub_ = this->create_publisher<hardware_msg::msg::Imu>("imu/data", 10);
        motors_states_pub_ = this->create_publisher<hardware_msg::msg::MotorsStates>("motors/states", 10);
        motor_data_pub_ = this->create_publisher<hardware_msg::msg::MotorData>("motor/data", 10);
        joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("/robot_joints", 10);
        motors_cmd_pub_ = this->create_publisher<hardware_msg::msg::MotorsCommands>("motors/commands", 10);

        joint_names_ = {
            "joint_l_yaw", "joint_l_roll", "joint_l_pitch", "joint_l_knee", "joint_l_ankle",
            "joint_r_yaw", "joint_r_roll", "joint_r_pitch", "joint_r_knee", "joint_r_ankle"};

        // Подписки
        motors_cmd_sub_ = this->create_subscription<hardware_msg::msg::MotorsCommands>(
            "motors/commands", 10,
            std::bind(&Motor_control::on_motors_commands, this, std::placeholders::_1));
        board_params_sub_ = this->create_subscription<hardware_msg::msg::BoardParameters>(
            "control_board/commands", 10,
            std::bind(&Motor_control::on_board_parameters, this, std::placeholders::_1));
        imu_params_sub_ = this->create_subscription<hardware_msg::msg::ImuParameters>(
            "imu/commands", 10,
            std::bind(&Motor_control::on_imu_parameters, this, std::placeholders::_1));
        motor_params_sub_ = this->create_subscription<hardware_msg::msg::MotorParameters>(
            "motor/params", 10,
            std::bind(&Motor_control::on_motor_parameters, this, std::placeholders::_1));

        using namespace std::chrono_literals;
        timer_ = this->create_wall_timer(1ms, std::bind(&Motor_control::on_timer, this));

        // Инициализация массивов
        std::fill_n(spi_tx_.q_set, 10, 0.0f);
        std::fill_n(spi_tx_.dq_set, 10, 0.0f);
        std::fill_n(spi_tx_.tau_ff, 10, 0.0f);
    }

private:
    // Подписчики
    rclcpp::Subscription<hardware_msg::msg::MotorsCommands>::SharedPtr motors_cmd_sub_;
    rclcpp::Subscription<hardware_msg::msg::BoardParameters>::SharedPtr board_params_sub_;
    rclcpp::Subscription<hardware_msg::msg::ImuParameters>::SharedPtr imu_params_sub_;
    rclcpp::Subscription<hardware_msg::msg::MotorParameters>::SharedPtr motor_params_sub_;

    // Издатели
    rclcpp::Publisher<hardware_msg::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<hardware_msg::msg::MotorsStates>::SharedPtr motors_states_pub_;
    rclcpp::Publisher<hardware_msg::msg::MotorData>::SharedPtr motor_data_pub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
    rclcpp::Publisher<hardware_msg::msg::MotorsCommands>::SharedPtr motors_cmd_pub_;
    std::vector<std::string> joint_names_;

    // Защищённые данные
    _SPI_TX spi_tx_;
    _SPI_RX spi_rx_;
    _MEMS mems_;

    std::mutex motor_cmd_mutex_;
    std::mutex motor_params_mutex_;
    std::mutex board_params_mutex_;
    std::mutex imu_params_mutex_;

    int fd = 0;
    rclcpp::TimerBase::SharedPtr timer_;

    void on_motors_commands(const hardware_msg::msg::MotorsCommands::SharedPtr msg)
    {
        if (msg->target_pos.size() != 10 || msg->target_vel.size() != 10 || msg->target_trq.size() != 10)
        {
            RCLCPP_WARN(this->get_logger(), "MotorsCommands must contain exactly 10 elements!");
            return;
        }

        std::lock_guard<std::mutex> lock(motor_cmd_mutex_);
        for (int i = 0; i < 10; ++i)
        {
            spi_tx_.q_set[i] = msg->target_pos[i];
            spi_tx_.dq_set[i] = msg->target_vel[i];
            spi_tx_.tau_ff[i] = msg->target_trq[i];
        }

        RCLCPP_DEBUG(this->get_logger(), "Updated motor commands for 10 motors");
    }

    void on_board_parameters(const hardware_msg::msg::BoardParameters::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(board_params_mutex_);
        spi_tx_.beep_state = msg->beep_state;
    }

    void on_imu_parameters(const hardware_msg::msg::ImuParameters::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(imu_params_mutex_);
        mems_.Acc_CALIBRATE = msg->acc_calibrate;
        mems_.Gyro_CALIBRATE = msg->gyro_calibrate;
        mems_.Mag_CALIBRATE = msg->mag_calibrate;
    }

    void on_motor_parameters(const hardware_msg::msg::MotorParameters::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(motor_params_mutex_);
        spi_tx_.kp = msg->kp;
        spi_tx_.kd = msg->kd;
        spi_tx_.en_motor = msg->enable;
        spi_tx_.reset_q = msg->reset_zero;
        spi_tx_.reset_err = msg->reset_error;
    }

    void send_zero_commands()
    {

        auto zero_commands = std::make_shared<hardware_msg::msg::MotorsCommands>();
        std::fill(zero_commands->target_pos.begin(), zero_commands->target_pos.end(), 0.0f);
        std::fill(zero_commands->target_vel.begin(), zero_commands->target_vel.end(), 0.0f);
        std::fill(zero_commands->target_trq.begin(), zero_commands->target_trq.end(), 0.0f);
        motors_cmd_pub_->publish(*zero_commands);

        RCLCPP_INFO(this->get_logger(), "Motor zero position command sent");
    }

    void transfer_with_tx(int sel, const _SPI_TX &tx_data, const _MEMS &mems_data)
    {
        static uint8_t state = 0;
        static uint8_t _data_len2 = 0, _data_cnt2 = 0;
        static int parser_timeout = 0;
        int ret;
        uint8_t data = 0;

        can_board_send(sel, tx_data, mems_data);

        ret = SPIDataRW(0, spi_tx_buf, rx, SPI_SEND_MAX);

        if (ret < 1)
        {
            RCLCPP_ERROR(this->get_logger(), "SPI ERROR: Reopen! ret=%d", ret);
            SPISetup(0, speed);
        }
        else
        {
            for (int i = 0; i < SPI_SEND_MAX; i++)
            {
                data = rx[i];
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

                    // Парсим в локальную структуру
                    _SPI_RX temp_rx;
                    if (slave_rx(spi_rx_buf, _data_cnt2 + 5, temp_rx))
                    {
                        spi_rx_ = temp_rx; // обновляем состояние
                    }
                }
                else
                {
                    if (data == 0xFF)
                    {
                        state = 1;
                        spi_rx_buf[0] = data;
                    }
                    else
                    {
                        state = 0;
                    }
                }
            }
        }
    }

    void on_timer()
    {
        static int counter = 0;
        static auto last_time = this->now();

        _SPI_TX local_tx;
        _MEMS local_mems;
        {
            std::lock_guard<std::mutex> lock1(motor_cmd_mutex_);
            std::lock_guard<std::mutex> lock2(motor_params_mutex_);
            std::lock_guard<std::mutex> lock3(board_params_mutex_);
            std::lock_guard<std::mutex> lock4(imu_params_mutex_);
            local_tx = spi_tx_;
            local_mems = mems_;
        }

        transfer_with_tx(45, local_tx, local_mems);

        counter++;
        if (counter % 1000 == 0)
        {
            auto now = this->now();
            auto dt = (now - last_time).seconds();
            RCLCPP_INFO(this->get_logger(), "SPI frequency: %.1f Hz", 1000.0 / dt);
            last_time = now;

            RCLCPP_INFO(this->get_logger(), "Motor 0 command: pos=%.3f, vel=%.3f, trq=%.3f",
                        local_tx.q_set[0], local_tx.dq_set[0], local_tx.tau_ff[0]);
        }

        // Публикация IMU
        hardware_msg::msg::Imu imu_msg;
        imu_msg.roll = spi_rx_.att[0];
        imu_msg.pitch = spi_rx_.att[1];
        imu_msg.yaw = spi_rx_.att[2];
        imu_pub_->publish(imu_msg);

        hardware_msg::msg::MotorsStates motors_states_msg;
        for (int i = 0; i < 10; ++i)
        {
            motors_states_msg.current_pos[i] = spi_rx_.q[i];
            motors_states_msg.current_vel[i] = spi_rx_.dq[i];
            motors_states_msg.current_trq[i] = spi_rx_.tau[i];
        }
        motors_states_pub_->publish(motors_states_msg);

        // JointState
        sensor_msgs::msg::JointState js;
        js.header.stamp = this->get_clock()->now();
        js.name = joint_names_;
        js.position.resize(10);
        for (int i = 0; i < 10; ++i)
        {
            js.position[i] = spi_rx_.q[i];
        }
        joint_state_pub_->publish(js);
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Motor_control>());
    rclcpp::shutdown();
    return 0;
}