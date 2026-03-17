/**
 * @file motor_control_node.cpp
 * @brief Реализация ROS2 узла для управления моторами через SPI
 */

#include "motor_control_node.hpp"
#include "spi.hpp"
#include <algorithm>
#include <cmath>

MotorControlNode::MotorControlNode()
    : rclcpp::Node("dual_io_node"),
      mems_{}
{
    RCLCPP_INFO(this->get_logger(), "Hardware::Thread_SPI started");

    this->declare_parameter<double>("limits.position.min", -3.14159);
    this->declare_parameter<double>("limits.position.max", 3.14159);
    this->declare_parameter<double>("limits.velocity.min", -20.0);
    this->declare_parameter<double>("limits.velocity.max", 20.0);
    this->declare_parameter<double>("limits.torque.min", -12.0);
    this->declare_parameter<double>("limits.torque.max", 12.0);
    this->declare_parameter<double>("limits.kp.min", 0.0);
    this->declare_parameter<double>("limits.kp.max", 500.0);
    this->declare_parameter<double>("limits.kd.min", 0.0);
    this->declare_parameter<double>("limits.kd.max", 5.0);

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

    RCLCPP_INFO(this->get_logger(),
                "Motor limits loaded: position=[%.3f, %.3f] rad, velocity=[%.3f, %.3f] rad/s, torque=[%.3f, %.3f] Nm, kp=[%.3f, %.3f], kd=[%.3f, %.3f]",
                limits_.min_position, limits_.max_position, limits_.min_velocity, limits_.max_velocity,
                limits_.min_torque, limits_.max_torque, limits_.min_kp, limits_.max_kp,
                limits_.min_kd, limits_.max_kd);

    int fd = SPISetup(0, static_cast<int>(motor_control::get_spi_speed()));
    if (fd == -1)
    {
        RCLCPP_ERROR(this->get_logger(), "init spi failed!");
        return;
    }

    imu_pub_ = this->create_publisher<tinker_msgs::msg::IMUState>("/imu_state", 10);
    low_state_pub_ = this->create_publisher<tinker_msgs::msg::LowState>("/low_level_state", 10);
    joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("/robot_joints", 10);

    joint_names_ = {
        "joint_l_yaw", "joint_l_roll", "joint_l_pitch", "joint_l_knee", "joint_l_ankle",
        "joint_r_yaw", "joint_r_roll", "joint_r_pitch", "joint_r_knee", "joint_r_ankle"};

    low_cmd_sub_ = this->create_subscription<tinker_msgs::msg::LowCmd>(
        "/low_level_command", 10, std::bind(&MotorControlNode::on_motors_commands, this, std::placeholders::_1));
    control_cmd_sub_ = this->create_subscription<tinker_msgs::msg::ControlCmd>(
        "/control_command", 10, std::bind(&MotorControlNode::on_board_parameters, this, std::placeholders::_1));
    one_motor_sub_ = this->create_subscription<tinker_msgs::msg::OneMotorCmd>(
        "/single_motor_command", 10, std::bind(&MotorControlNode::on_one_motor_command, this, std::placeholders::_1));

    using namespace std::chrono_literals;
    timer_ = this->create_wall_timer(1ms, std::bind(&MotorControlNode::on_timer, this));

    std::fill_n(spi_tx_.q_set, 10, 0.0f);
    std::fill_n(spi_tx_.dq_set, 10, 0.0f);
    std::fill_n(spi_tx_.tau_ff, 10, 0.0f);
    std::fill_n(spi_tx_.kp, 10, 0.0f);
    std::fill_n(spi_tx_.kd, 10, 0.0f);
}

MotorControlNode::LimitedMotorParams MotorControlNode::apply_limits(int motor_id, float position, float velocity, float torque, float kp, float kd)
{
    (void)motor_id;
    LimitedMotorParams result;
    result.position = std::clamp(position, static_cast<float>(limits_.min_position), static_cast<float>(limits_.max_position));
    result.velocity = std::clamp(velocity, static_cast<float>(limits_.min_velocity), static_cast<float>(limits_.max_velocity));
    result.torque = std::clamp(torque, static_cast<float>(limits_.min_torque), static_cast<float>(limits_.max_torque));
    result.kp = std::clamp(kp, static_cast<float>(limits_.min_kp), static_cast<float>(limits_.max_kp));
    result.kd = std::clamp(kd, static_cast<float>(limits_.min_kd), static_cast<float>(limits_.max_kd));
    return result;
}

void MotorControlNode::on_motors_commands(const tinker_msgs::msg::LowCmd::SharedPtr msg)
{
    if (msg->motor_cmd.size() != 10)
    {
        RCLCPP_WARN(this->get_logger(), "LowCmd.motor_cmd must contain exactly 10 elements!");
        return;
    }
    std::lock_guard<std::mutex> lock(motor_cmd_mutex_);
    for (int i = 0; i < 10; ++i)
    {
        LimitedMotorParams limited = apply_limits(i,
            msg->motor_cmd[i].position,
            msg->motor_cmd[i].velocity,
            msg->motor_cmd[i].torque,
            msg->motor_cmd[i].kp,
            msg->motor_cmd[i].kd);
        spi_tx_.q_set[i] = static_cast<float>(limited.position);
        spi_tx_.dq_set[i] = static_cast<float>(limited.velocity);
        spi_tx_.tau_ff[i] = limited.torque;
        spi_tx_.kp[i] = limited.kp;
        spi_tx_.kd[i] = limited.kd;
    }
}

void MotorControlNode::on_board_parameters(const tinker_msgs::msg::ControlCmd::SharedPtr msg)
{
    std::lock_guard<std::mutex> lock(board_params_mutex_);
    if (msg->cmd == tinker_msgs::msg::ControlCmd::ENABLE)
        en_motor_atomic.store(1);
    else if (msg->cmd == tinker_msgs::msg::ControlCmd::DISABLE)
        en_motor_atomic.store(0);
    else if (msg->cmd == tinker_msgs::msg::ControlCmd::SET_ZERO_POSITION)
    {
        if (en_motor_atomic.load() != 0)
        {
            RCLCPP_WARN(this->get_logger(), "SET_ZERO_POSITION: Отклонено - двигатели включены. Сначала отключите двигатели (DISABLE)");
            return;
        }
        reset_q_atomic.store(1);
        reset_q_set_time = std::chrono::steady_clock::now();
        reset_q_timer_active.store(true);
        std::lock_guard<std::mutex> lock2(motor_cmd_mutex_);
        for (int i = 0; i < 10; ++i)
        {
            spi_tx_.q_set[i] = 0.0f;
            spi_tx_.dq_set[i] = 0.0f;
            spi_tx_.tau_ff[i] = 0.0f;
            spi_tx_.kp[i] = 0.0f;
            spi_tx_.kd[i] = 0.0f;
        }
    }
    else if (msg->cmd == tinker_msgs::msg::ControlCmd::CLEAR_ERROR)
        reset_err_atomic.store(1);
    else if (msg->cmd == tinker_msgs::msg::ControlCmd::IMU_CALIBRATE)
    {
        std::lock_guard<std::mutex> imu_lock(imu_params_mutex_);
        mems_.Acc_CALIBRATE = 1;
        mems_.Gyro_CALIBRATE = 1;
        mems_.Mag_CALIBRATE = 1;
        RCLCPP_INFO(this->get_logger(), "IMU calibration requested: acc/gyro/mag set");
    }
}

void MotorControlNode::on_one_motor_command(const tinker_msgs::msg::OneMotorCmd::SharedPtr msg)
{
    int id = static_cast<int>(msg->motor_id);
    if (id < 0 || id >= 10)
    {
        RCLCPP_WARN(this->get_logger(), "OneMotorCmd: motor_id %d out of range", id);
        return;
    }
    std::lock_guard<std::mutex> lock(motor_cmd_mutex_);
    LimitedMotorParams limited = apply_limits(id, msg->position, msg->velocity, msg->torque, msg->kp, msg->kd);
    spi_tx_.q_set[id] = static_cast<float>(limited.position);
    spi_tx_.dq_set[id] = static_cast<float>(limited.velocity);
    spi_tx_.tau_ff[id] = limited.torque;
    spi_tx_.kp[id] = limited.kp;
    spi_tx_.kd[id] = limited.kd;
}

void MotorControlNode::on_timer()
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
        mems_.Acc_CALIBRATE = 0;
        mems_.Gyro_CALIBRATE = 0;
        mems_.Mag_CALIBRATE = 0;
    }

    if (reset_q_timer_active.load())
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - reset_q_set_time).count();
        if (elapsed >= 1000)
        {
            reset_q_atomic.store(0);
            reset_q_timer_active.store(false);
        }
    }

    local_tx.en_motor = static_cast<int>(en_motor_atomic.load());
    local_tx.reset_q = static_cast<int>(reset_q_atomic.load());
    local_tx.reset_err = static_cast<int>(reset_err_atomic.load());

    int ret = motor_control::spi_transfer_and_parse(45, local_tx, local_mems, spi_rx_);

    if (ret < 1)
    {
        RCLCPP_ERROR(this->get_logger(), "SPI ERROR: Reopen! ret=%d", ret);
        SPISetup(0, static_cast<int>(motor_control::get_spi_speed()));
    }

    counter++;
    if (counter % 1000 == 0)
    {
        auto now = this->now();
        auto dt = (now - last_time).seconds();
        RCLCPP_INFO(this->get_logger(), "SPI frequency: %.1f Hz", 1000.0 / dt);
        RCLCPP_INFO(this->get_logger(), "  id | position | velocity |  torque  |");
        for (int i = 0; i < 10; ++i)
        {
            RCLCPP_INFO(this->get_logger(), " %2d  | %8.3f | %8.3f | %8.4f |",
                        i + 1, static_cast<double>(spi_rx_.q[i]), static_cast<double>(spi_rx_.dq[i]), static_cast<double>(spi_rx_.tau[i]));
        }
        last_time = now;
    }

    tinker_msgs::msg::LowState low_state_msg;
    low_state_msg.timestamp_state = this->now();
    low_state_msg.tick = static_cast<uint32_t>(counter);
    low_state_msg.imu_state.timestamp_state = this->now();
    low_state_msg.imu_state.rpy[0] = spi_rx_.att[0];
    low_state_msg.imu_state.rpy[1] = spi_rx_.att[1];
    low_state_msg.imu_state.rpy[2] = spi_rx_.att[2];
    low_state_msg.imu_state.quaternion = {0.0f, 0.0f, 0.0f, 0.0f};
    low_state_msg.imu_state.gyroscope[0] = spi_rx_.att_rate[0];
    low_state_msg.imu_state.gyroscope[1] = spi_rx_.att_rate[1];
    low_state_msg.imu_state.gyroscope[2] = spi_rx_.att_rate[2];
    low_state_msg.imu_state.accelerometer[0] = spi_rx_.acc_b[0];
    low_state_msg.imu_state.accelerometer[1] = spi_rx_.acc_b[1];
    low_state_msg.imu_state.accelerometer[2] = spi_rx_.acc_b[2];
    low_state_msg.imu_state.temperature = 0;

    for (int i = 0; i < 10; ++i)
    {
        low_state_msg.motor_state[i].timestamp_state = this->now();
        low_state_msg.motor_state[i].position = static_cast<float>(spi_rx_.q[i]);
        low_state_msg.motor_state[i].velocity = static_cast<float>(spi_rx_.dq[i]);
        low_state_msg.motor_state[i].torque = spi_rx_.tau[i];
        low_state_msg.motor_state[i].temperature_mosfet = 0;
        low_state_msg.motor_state[i].temperature_rotor = 0;
        low_state_msg.motor_state[i].error = static_cast<uint8_t>(spi_rx_.connect_motor[i]);
    }
    low_state_pub_->publish(low_state_msg);

    if (imu_pub_)
    {
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

    sensor_msgs::msg::JointState js;
    js.header.stamp = this->get_clock()->now();
    js.name = joint_names_;
    js.position.resize(10);
    for (int i = 0; i < 10; ++i)
        js.position[i] = static_cast<double>(spi_rx_.q[i]);
    joint_state_pub_->publish(js);
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorControlNode>());
    rclcpp::shutdown();
    return 0;
}
