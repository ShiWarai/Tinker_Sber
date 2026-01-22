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
 *  Порядок mutex:
 *      - motor_cmd_mutex_;
        - board_params_mutex_;
        - imu_params_mutex_;
 */

#include <memory>
#include <atomic>
#include <functional>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/bool.hpp"
#include "tinker_msgs/msg/low_cmd.hpp"
#include "tinker_msgs/msg/low_state.hpp"
#include "tinker_msgs/msg/imu_state.hpp"
#include "tinker_msgs/msg/motor_cmd.hpp"
#include "tinker_msgs/msg/motor_state.hpp"
#include "tinker_msgs/msg/control_cmd.hpp"
#include "tinker_msgs/msg/one_motor_cmd.hpp"
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

using namespace std;

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

// Константы преобразования единиц
constexpr double RAD_TO_DEG = 180.0 / M_PI;
constexpr double DEG_TO_RAD = M_PI / 180.0;

// Структура для хранения лимитов параметров моторов
struct MotorLimits {
    double min_position;
    double max_position;
    double min_velocity;
    double max_velocity;
    double min_torque;
    double max_torque;
    double min_kp;
    double max_kp;
    double min_kd;
    double max_kd;
};

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

        // Объявление и чтение параметров лимитов

        //this->declare_parameter<double>("limits.position.min", -3.14159);
        //this->declare_parameter<double>("limits.position.max", 3.14159);
        this->declare_parameter<double>("limits.position.min", -6.28308);
        this->declare_parameter<double>("limits.position.max", 3.14159);

        this->declare_parameter<double>("limits.velocity.min", -20.0);
        this->declare_parameter<double>("limits.velocity.max", 20.0);
        this->declare_parameter<double>("limits.torque.min", -12.0);
        this->declare_parameter<double>("limits.torque.max", 12.0);
        this->declare_parameter<double>("limits.kp.min", 0.0);
        this->declare_parameter<double>("limits.kp.max", 1000.0);
        this->declare_parameter<double>("limits.kd.min", 0.0);
        this->declare_parameter<double>("limits.kd.max", 100.0);

        // Чтение параметров из launch файла
        limits_.min_position = this->get_parameter("limits.position.min").as_double();
        limits_.max_position = this->get_parameter("limits.position.max").as_double();
        limits_.min_velocity = this->get_parameter("limits.velocity.min").as_double();
        limits_.max_velocity = this->get_parameter("limits.velocity.max").as_double();
        limits_.min_torque = this->get_parameter("limits.torque.min").as_double();
        limits_.max_torque = this->get_parameter("limits.torque.max").as_double();
        limits_.min_kp = this->get_parameter("limits.kp.min").as_double();
        limits_.max_kp = this->get_parameter("limits.kp.max").as_double();
        limits_.min_kd = this->get_parameter("limits.kd.min").as_double();
        limits_.max_kd = this->get_parameter("limits.kd.max").as_double();

        RCLCPP_INFO(this->get_logger(), "Motor limits loaded: position=[%.3f, %.3f] rad, velocity=[%.3f, %.3f] rad/s, torque=[%.3f, %.3f] Nm, kp=[%.3f, %.3f], kd=[%.3f, %.3f]",
                    limits_.min_position, limits_.max_position, limits_.min_velocity, limits_.max_velocity,
                    limits_.min_torque, limits_.max_torque, limits_.min_kp, limits_.max_kp,
                    limits_.min_kd, limits_.max_kd);

        Cycle_Time_Init();
        fd = SPISetup(0, speed);

        if (fd == -1)
        {
            RCLCPP_ERROR(this->get_logger(), "init spi failed!");
            return;
        }

        // Инициализация издателей (используем абсолютные имена, чтобы совпадало с GUI)
        imu_pub_ = this->create_publisher<tinker_msgs::msg::IMUState>("/imu_state", 10);
        low_state_pub_ = this->create_publisher<tinker_msgs::msg::LowState>("/tinker_msgs/lowstate", 10);
        joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("/robot_joints", 10);

        joint_names_ = {
            "joint_l_yaw", "joint_l_roll", "joint_l_pitch", "joint_l_knee", "joint_l_ankle",
            "joint_r_yaw", "joint_r_roll", "joint_r_pitch", "joint_r_knee", "joint_r_ankle"};

        // Подписки
        low_cmd_sub_ = this->create_subscription<tinker_msgs::msg::LowCmd>(
            "/tinker_msgs/lowcmd", 10,
            std::bind(&Motor_control::on_motors_commands, this, std::placeholders::_1));
        control_cmd_sub_ = this->create_subscription<tinker_msgs::msg::ControlCmd>(
            "/tinker_msgs/controlcmd", 10,
            std::bind(&Motor_control::on_board_parameters, this, std::placeholders::_1));
        one_motor_sub_ = this->create_subscription<tinker_msgs::msg::OneMotorCmd>(
            "/single_motor_command", 10,
            std::bind(&Motor_control::on_one_motor_command, this, std::placeholders::_1));

        using namespace std::chrono_literals;
        timer_ = this->create_wall_timer(1ms, std::bind(&Motor_control::on_timer, this));

        // Инициализация массивов
        std::fill_n(spi_tx_.q_set, 10, 0.0f);
        std::fill_n(spi_tx_.dq_set, 10, 0.0f);
        std::fill_n(spi_tx_.tau_ff, 10, 0.0f);
        std::fill_n(kp_cmd_, 10, 0.0f);
        std::fill_n(kd_cmd_, 10, 0.0f);
    }

private:
    // Подписчики
    rclcpp::Subscription<tinker_msgs::msg::LowCmd>::SharedPtr low_cmd_sub_;
    rclcpp::Subscription<tinker_msgs::msg::ControlCmd>::SharedPtr control_cmd_sub_;
    rclcpp::Subscription<tinker_msgs::msg::OneMotorCmd>::SharedPtr one_motor_sub_;

    // Издатели
    rclcpp::Publisher<tinker_msgs::msg::IMUState>::SharedPtr imu_pub_;
    rclcpp::Publisher<tinker_msgs::msg::LowState>::SharedPtr low_state_pub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
    std::vector<std::string> joint_names_;

    // Защищённые данные — теперь ВСЕ внутри класса
    _SPI_TX spi_tx_;
    _SPI_RX spi_rx_;
    _MEMS mems_;

    // Добавляем хранение per-motor kp/kd
    float kp_cmd_[10];
    float kd_cmd_[10];

    // Лимиты параметров моторов
    MotorLimits limits_;

    // Структура для возврата ограниченных значений
    struct LimitedMotorParams {
        float position;
        float velocity;
        float torque;
        float kp;
        float kd;
    };

    std::mutex motor_cmd_mutex_;
    std::mutex board_params_mutex_;
    std::mutex imu_params_mutex_;

    // Заменяем отдельные флаги на атомики (вместо spi_tx_.en_motor / reset_q / reset_err)
    std::atomic<uint8_t> en_motor_atomic{0};
    std::atomic<uint8_t> reset_q_atomic{0};
    std::atomic<uint8_t> reset_err_atomic{0};

    // Время установки reset_q для автоматического сброса через 1 секунду
    std::chrono::steady_clock::time_point reset_q_set_time;
    std::atomic<bool> reset_q_timer_active{false};

    int fd = 0;
    rclcpp::TimerBase::SharedPtr timer_;

    // Функция применения ограничений к параметрам мотора
    LimitedMotorParams apply_limits(int motor_id, float position, float velocity, float torque, float kp, float kd)
    {
        LimitedMotorParams result;
        float original_value;

        // Ограничение position
        original_value = position;
        result.position = std::clamp(position, static_cast<float>(limits_.min_position), static_cast<float>(limits_.max_position));
        if (original_value != result.position)
        {
            RCLCPP_WARN(this->get_logger(), "Motor %d: position clamped from %.3f to %.3f rad", 
                        motor_id, original_value, result.position);
        }

        // Ограничение velocity
        original_value = velocity;
        result.velocity = std::clamp(velocity, static_cast<float>(limits_.min_velocity), static_cast<float>(limits_.max_velocity));
        if (original_value != result.velocity)
        {
            RCLCPP_WARN(this->get_logger(), "Motor %d: velocity clamped from %.3f to %.3f rad/s", 
                        motor_id, original_value, result.velocity);
        }

        // Ограничение torque
        original_value = torque;
        result.torque = std::clamp(torque, static_cast<float>(limits_.min_torque), static_cast<float>(limits_.max_torque));
        if (original_value != result.torque)
        {
            RCLCPP_WARN(this->get_logger(), "Motor %d: torque clamped from %.3f to %.3f Nm", 
                        motor_id, original_value, result.torque);
        }

        // Ограничение kp
        original_value = kp;
        result.kp = std::clamp(kp, static_cast<float>(limits_.min_kp), static_cast<float>(limits_.max_kp));
        if (original_value != result.kp)
        {
            RCLCPP_WARN(this->get_logger(), "Motor %d: kp clamped from %.3f to %.3f", 
                        motor_id, original_value, result.kp);
        }

        // Ограничение kd
        original_value = kd;
        result.kd = std::clamp(kd, static_cast<float>(limits_.min_kd), static_cast<float>(limits_.max_kd));
        if (original_value != result.kd)
        {
            RCLCPP_WARN(this->get_logger(), "Motor %d: kd clamped from %.3f to %.3f", 
                        motor_id, original_value, result.kd);
        }

        return result;
    }

    void on_motors_commands(const tinker_msgs::msg::LowCmd::SharedPtr msg)
    {
        if (msg->motor_cmd.size() != 10)
        {
            RCLCPP_WARN(this->get_logger(), "LowCmd.motor_cmd must contain exactly 10 elements!");
            return;
        }

        std::lock_guard<std::mutex> lock(motor_cmd_mutex_);
        for (int i = 0; i < 10; ++i)
        {
            // Применяем ограничения (входные значениon_board_parametersя в радианах)
            LimitedMotorParams limited = apply_limits(i, 
                msg->motor_cmd[i].position,
                msg->motor_cmd[i].velocity,
                msg->motor_cmd[i].torque,
                msg->motor_cmd[i].kp,
                msg->motor_cmd[i].kd);

            // Преобразуем position и velocity из радиан в градусы для SPI
            spi_tx_.q_set[i] = static_cast<float>(limited.position * RAD_TO_DEG);
            spi_tx_.dq_set[i] = static_cast<float>(limited.velocity * RAD_TO_DEG);
            spi_tx_.tau_ff[i] = limited.torque;
            kp_cmd_[i] = limited.kp;
            kd_cmd_[i] = limited.kd;
        }

        spi_tx_.kp = kp_cmd_[0];
        spi_tx_.kd = kd_cmd_[0];
        RCLCPP_DEBUG(this->get_logger(), "Updated LowCmd -> SPI motor commands (kp/kd from motor 0)");
    }

    void on_board_parameters(const tinker_msgs::msg::ControlCmd::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(board_params_mutex_);
        // ControlCmd: cmd codes:
        // ENABLE = 252
        // DISABLE = 253
        // SET_ZERRO_POSITION = 254
        // CLEAR_ERROR = 251
        // IMU_CALIBRATE = 250
        // Если необходимо применить команду к конкретному мотору — можно проверять msg->motor_id
        if (msg->cmd == tinker_msgs::msg::ControlCmd::ENABLE)
        {
            RCLCPP_INFO(this->get_logger(), "Motors ENABLED");
            en_motor_atomic.store(1);
        }
        else if (msg->cmd == tinker_msgs::msg::ControlCmd::DISABLE)
        {
            RCLCPP_INFO(this->get_logger(), "Motors DISABLED");
            en_motor_atomic.store(0);

        }   
        else if (msg->cmd == tinker_msgs::msg::ControlCmd::SET_ZERO_POSITION)
        {
            // Защита: обнуление можно делать только при отключенных двигателях
            if (en_motor_atomic.load() != 0)
            {
                RCLCPP_WARN(this->get_logger(), "SET_ZERO_POSITION: Отклонено - двигатели включены. Сначала отключите двигатели (DISABLE)");
                return;
            }
            RCLCPP_INFO(this->get_logger(), "Set zero position");
            reset_q_atomic.store(1);
            reset_q_set_time = std::chrono::steady_clock::now();
            reset_q_timer_active.store(true);
            
            // Обнуляем все значения при начале обнуления позиции, чтобы не передавались старые значения
            {
                std::lock_guard<std::mutex> lock(motor_cmd_mutex_);
                for (int i = 0; i < 10; ++i)
                {
                    spi_tx_.q_set[i] = 0.0f;
                    spi_tx_.dq_set[i] = 0.0f;
                    spi_tx_.tau_ff[i] = 0.0f;
                    kp_cmd_[i] = 0.0f;
                    kd_cmd_[i] = 0.0f;
                }
                spi_tx_.kp = 0.0f;
                spi_tx_.kd = 0.0f;
            }
        }
        else if (msg->cmd == tinker_msgs::msg::ControlCmd::CLEAR_ERROR)
        {
            reset_err_atomic.store(1);
        }
        else if (msg->cmd == tinker_msgs::msg::ControlCmd::IMU_CALIBRATE)
        {
            std::lock_guard<std::mutex> imu_lock(imu_params_mutex_);
            mems_.Acc_CALIBRATE = 1;
            mems_.Gyro_CALIBRATE = 1;
            mems_.Mag_CALIBRATE = 1;
            RCLCPP_INFO(this->get_logger(), "IMU calibration requested: acc/gyro/mag set");
        }
    }

    // Обработчик одиночной команды от GUI — обновляет только указанный мотор
    void on_one_motor_command(const tinker_msgs::msg::OneMotorCmd::SharedPtr msg)
    {
        int id = static_cast<int>(msg->motor_id);
        if (id < 0 || id >= 10)
        {
            RCLCPP_WARN(this->get_logger(), "OneMotorCmd: motor_id %d out of range", id);
            return;
        }

        std::lock_guard<std::mutex> lock(motor_cmd_mutex_);
        
        // Применяем ограничения (входные значения в радианах)
        LimitedMotorParams limited = apply_limits(id,
            msg->position,
            msg->velocity,
            msg->torque,
            msg->kp,
            msg->kd);

        // Преобразуем position и velocity из радиан в градусы для SPI
        spi_tx_.q_set[id] = static_cast<float>(limited.position * RAD_TO_DEG);
        spi_tx_.dq_set[id] = static_cast<float>(limited.velocity * RAD_TO_DEG);
        spi_tx_.tau_ff[id] = limited.torque;
        kp_cmd_[id] = limited.kp;
        kd_cmd_[id] = limited.kd;
        
        // Обновляем глобальные kp/kd, которые отправляются по SPI (протокол поддерживает одну пару)
        spi_tx_.kp = limited.kp;
        spi_tx_.kd = limited.kd;
        RCLCPP_DEBUG(this->get_logger(), "OneMotorCmd applied to motor %d", id);
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
            std::lock_guard<std::mutex> lock2(board_params_mutex_);
            std::lock_guard<std::mutex> lock3(imu_params_mutex_);
            local_tx = spi_tx_;
            local_mems = mems_;
            // Сбрасываем флаги калибровки в основной структуре так, чтобы они отправились только один раз
            mems_.Acc_CALIBRATE = 0;
            mems_.Gyro_CALIBRATE = 0;
            mems_.Mag_CALIBRATE = 0;
        }

        // Автоматический сброс reset_q через 1 секунду (проверяем ДО копирования в local_tx)
        if (reset_q_timer_active.load())
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - reset_q_set_time).count();
            if (elapsed >= 10000)  // 1 секунда = 1000 мс
            {
                reset_q_atomic.store(0);
                reset_q_timer_active.store(false);
            }
        }

        // Синхронизируем простые флаги из atomics в local_tx (атомарно, без блокировки)
        local_tx.en_motor = static_cast<int>(en_motor_atomic.load());
        local_tx.reset_q = static_cast<int>(reset_q_atomic.load());
        local_tx.reset_err = static_cast<int>(reset_err_atomic.load());

        transfer_with_tx(45, local_tx, local_mems);

        counter++;

        // Формируем LowState, включаем внутрь IMU и состояния моторов (GUI ожидает LowState на /low_level_state)
        tinker_msgs::msg::LowState low_state_msg;
        low_state_msg.timestamp_state = this->now();
        low_state_msg.tick = static_cast<uint32_t>(counter);

        // Заполняем imu_state внутри LowState
        low_state_msg.imu_state.timestamp_state = this->now();
        low_state_msg.imu_state.rpy[0] = spi_rx_.att[0];
        low_state_msg.imu_state.rpy[1] = spi_rx_.att[1];
        low_state_msg.imu_state.rpy[2] = spi_rx_.att[2];
        // quaternion может быть неизвестен здесь — оставляем нулями
        low_state_msg.imu_state.quaternion = {0.0f, 0.0f, 0.0f, 0.0f};
        low_state_msg.imu_state.gyroscope[0] = spi_rx_.att_rate[0];
        low_state_msg.imu_state.gyroscope[1] = spi_rx_.att_rate[1];
        low_state_msg.imu_state.gyroscope[2] = spi_rx_.att_rate[2];
        low_state_msg.imu_state.accelerometer[0] = spi_rx_.acc_b[0];
        low_state_msg.imu_state.accelerometer[1] = spi_rx_.acc_b[1];
        low_state_msg.imu_state.accelerometer[2] = spi_rx_.acc_b[2];
        low_state_msg.imu_state.temperature = 0;

        // Заполняем состояния 10 моторов (преобразуем из градусов в радианы)
        for (int i = 0; i < 10; ++i)
        {
            low_state_msg.motor_state[i].timestamp_state = this->now();
            low_state_msg.motor_state[i].position  = static_cast<float>(spi_rx_.q[i] * DEG_TO_RAD);
            low_state_msg.motor_state[i].velocity = static_cast<float>(spi_rx_.dq[i] * DEG_TO_RAD);
            //low_state_msg.motor_state[i].velocity = static_cast<float>(spi_rx_.dq[i] * DEG_TO_RAD);
            low_state_msg.motor_state[i].torque = spi_rx_.tau[i];
            low_state_msg.motor_state[i].temperature_mosfet = 0;
            low_state_msg.motor_state[i].temperature_rotor = 0;
            low_state_msg.motor_state[i].error = static_cast<uint8_t>(spi_rx_.connect_motor[i]);
        }
                if (counter % 1000 == 0)
        {
            auto now = this->now();
            auto dt = (now - last_time).seconds();
            RCLCPP_INFO(this->get_logger(), "SPI frequency: %.1f Hz", 1000.0 / dt);
            last_time = now;

            RCLCPP_INFO(this->get_logger(), "RPY 1 =%.3f, RPY 2 =%.3f, RPY 3 =%.3f",
                        spi_rx_.att[0], spi_rx_.att[2], spi_rx_.att[2]);

            RCLCPP_INFO(this->get_logger(), "Motor 0 command: pos=%.3f, vel=%.3f, trq=%.3f",
                        local_tx.q_set[0], local_tx.dq_set[0], local_tx.tau_ff[0]);
            RCLCPP_INFO(this->get_logger(), "Motor 0 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[0].position,  low_state_msg.motor_state[0].velocity, low_state_msg.motor_state[0].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 1 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[1].position,  low_state_msg.motor_state[1].velocity, low_state_msg.motor_state[1].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 2 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[2].position,  low_state_msg.motor_state[2].velocity, low_state_msg.motor_state[2].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 3 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[3].position,  low_state_msg.motor_state[3].velocity, low_state_msg.motor_state[3].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 4 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[4].position,  low_state_msg.motor_state[4].velocity, low_state_msg.motor_state[4].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 5 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[5].position,  low_state_msg.motor_state[5].velocity, low_state_msg.motor_state[5].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 6 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[6].position,  low_state_msg.motor_state[6].velocity, low_state_msg.motor_state[6].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 7 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[7].position,  low_state_msg.motor_state[7].velocity, low_state_msg.motor_state[7].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 8 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[8].position,  low_state_msg.motor_state[8].velocity, low_state_msg.motor_state[8].torque);
            RCLCPP_INFO(this->get_logger(), "Motor 9 status: pos=%.3f, vel=%.3f, trq=%.3f",
                        low_state_msg.motor_state[9].position,  low_state_msg.motor_state[9].velocity, low_state_msg.motor_state[9].torque);    

        }
        // Публикуем LowState для GUI
        low_state_pub_->publish(low_state_msg);

        // Опционально: публикуем отдельный IMU topic (оставлено для совместимости)
        if (imu_pub_) {
            tinker_msgs::msg::IMUState imu_msg;
            imu_msg.timestamp_state = this->now();
            imu_msg.rpy[0] = spi_rx_.att[0];
            imu_msg.rpy[1] = spi_rx_.att[1];
            imu_msg.rpy[2] = spi_rx_.att[2];
            imu_msg.quaternion = {0.0f, 0.0f, 0.0f, 0.0f};
            imu_msg.gyroscope[0] = spi_rx_.att_rate[0];
            imu_msg.gyroscope[1] = spi_rx_.att_rate[1];
            imu_msg.gyroscope[2] = spi_rx_.att_rate[2];
            imu_msg.accelerometer[0] = spi_rx_.acc_b[0];
            imu_msg.accelerometer[1] = spi_rx_.acc_b[1];
            imu_msg.accelerometer[2] = spi_rx_.acc_b[2];
            imu_msg.temperature = 0;
            imu_pub_->publish(imu_msg);
        }

        // JointState (преобразуем из градусов в радианы)
        sensor_msgs::msg::JointState js;
        js.header.stamp = this->get_clock()->now();
        js.name = joint_names_;
        js.position.resize(10);
        for (int i = 0; i < 10; ++i)
        {
            js.position[i] = static_cast<double>(spi_rx_.q[i] * DEG_TO_RAD);
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