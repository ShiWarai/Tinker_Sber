#ifndef MOTOR_CONTROL_MOTOR_CONTROL_NODE_HPP
#define MOTOR_CONTROL_MOTOR_CONTROL_NODE_HPP

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "geometry_msgs/msg/quaternion.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "tinker_msgs/msg/control_cmd.hpp"
#include "tinker_msgs/msg/low_cmd.hpp"
#include "tinker_msgs/msg/low_state.hpp"
#include "tinker_msgs/msg/one_motor_cmd.hpp"

#include "imu_filter/imu_filter.hpp"
#include "motor_control.hpp"
#include "spi_node.hpp"

struct MotorLimits {
    double min_position, max_position;
    double min_velocity, max_velocity;
    double min_torque, max_torque;
    double min_kp, max_kp;
    double min_kd, max_kd;
};

/**
 * ROS2 узел для управления моторами через SPI.
 * Подписки: /low_level_command, /control_command, /single_motor_command.
 * Публикации: /low_level_state, /imu_state (sensor_msgs/Imu),
 *             /imu_orientation (geometry_msgs/Quaternion), /robot_joints.
 */
class MotorControlNode : public rclcpp::Node
{
public:
    MotorControlNode();

private:
    struct LimitedMotorParams {
        float position, velocity, torque, kp, kd;
    };

    LimitedMotorParams apply_limits(int motor_id, float position, float velocity,
                                    float torque, float kp, float kd);

    void on_motors_commands(const tinker_msgs::msg::LowCmd::SharedPtr msg);
    void on_board_parameters(const tinker_msgs::msg::ControlCmd::SharedPtr msg);
    void on_one_motor_command(const tinker_msgs::msg::OneMotorCmd::SharedPtr msg);
    void on_timer();

    rclcpp::Subscription<tinker_msgs::msg::LowCmd>::SharedPtr low_cmd_sub_;
    rclcpp::Subscription<tinker_msgs::msg::ControlCmd>::SharedPtr control_cmd_sub_;
    rclcpp::Subscription<tinker_msgs::msg::OneMotorCmd>::SharedPtr one_motor_sub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<geometry_msgs::msg::Quaternion>::SharedPtr imu_orientation_pub_;
    rclcpp::Publisher<tinker_msgs::msg::LowState>::SharedPtr low_state_pub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
    std::vector<std::string> joint_names_;

    _SPI_TX spi_tx_;
    _SPI_RX spi_rx_;
    _MEMS mems_;
    MotorLimits limits_;

    std::mutex motor_cmd_mutex_;
    std::mutex board_params_mutex_;
    std::mutex imu_params_mutex_;
    std::atomic<uint8_t> en_motor_atomic{0};
    std::atomic<uint8_t> reset_q_atomic{0};
    std::atomic<uint8_t> reset_err_atomic{0};
    std::chrono::steady_clock::time_point reset_q_set_time;
    std::atomic<bool> reset_q_timer_active{false};
    rclcpp::TimerBase::SharedPtr timer_;

    imu_filter::ImuFilter imu_filter_;
};

#endif /* MOTOR_CONTROL_MOTOR_CONTROL_NODE_HPP */
