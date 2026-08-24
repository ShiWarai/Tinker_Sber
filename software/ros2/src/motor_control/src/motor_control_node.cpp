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
                "Motor limits loaded: position=[%.3f, %.3f] rad, velocity=[%.3f, %.3f] rad/s, "
                "torque=[%.3f, %.3f] Nm, kp=[%.3f, %.3f], kd=[%.3f, %.3f]",
                limits_.min_position, limits_.max_position, limits_.min_velocity, limits_.max_velocity,
                limits_.min_torque, limits_.max_torque, limits_.min_kp, limits_.max_kp,
                limits_.min_kd, limits_.max_kd);

    int fd = SPISetup(0, static_cast<int>(motor_control::get_spi_speed()));
    if (fd == -1)
    {
        RCLCPP_ERROR(this->get_logger(), "init spi failed!");
        return;
    }

    imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("/imu_state", 10);
    imu_orientation_pub_ = this->create_publisher<geometry_msgs::msg::Quaternion>(
        "/imu_orientation", 10);
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

MotorControlNode::LimitedMotorParams MotorControlNode::apply_limits(
    int motor_id, float position, float velocity, float torque, float kp, float kd)
{
    LimitedMotorParams result;
    const float min_pos = static_cast<float>(limits_.min_position);
    const float max_pos = static_cast<float>(limits_.max_position);
    const float min_vel = static_cast<float>(limits_.min_velocity);
    const float max_vel = static_cast<float>(limits_.max_velocity);
    const float min_tau = static_cast<float>(limits_.min_torque);
    const float max_tau = static_cast<float>(limits_.max_torque);
    const float min_kp = static_cast<float>(limits_.min_kp);
    const float max_kp = static_cast<float>(limits_.max_kp);
    const float min_kd = static_cast<float>(limits_.min_kd);
    const float max_kd = static_cast<float>(limits_.max_kd);

    result.position = std::clamp(position, min_pos, max_pos);
    result.velocity = std::clamp(velocity, min_vel, max_vel);
    result.torque = std::clamp(torque, min_tau, max_tau);
    result.kp = std::clamp(kp, min_kp, max_kp);
    result.kd = std::clamp(kd, min_kd, max_kd);

    auto logger = this->get_logger();
    auto& clock = *this->get_clock();
    constexpr auto throttle_ms = 1000;

    if (result.position != position)
    {
        RCLCPP_WARN_THROTTLE(logger, clock, throttle_ms,
                             "Motor %d: position clamped from %.4f to %.4f (limits [%.4f, %.4f])",
                             motor_id, position, result.position, min_pos, max_pos);
    }
    if (result.velocity != velocity)
    {
        RCLCPP_WARN_THROTTLE(logger, clock, throttle_ms,
                             "Motor %d: velocity clamped from %.4f to %.4f (limits [%.4f, %.4f])",
                             motor_id, velocity, result.velocity, min_vel, max_vel);
    }
    if (result.torque != torque)
    {
        RCLCPP_WARN_THROTTLE(logger, clock, throttle_ms,
                             "Motor %d: torque clamped from %.4f to %.4f (limits [%.4f, %.4f])",
                             motor_id, torque, result.torque, min_tau, max_tau);
    }
    if (result.kp != kp)
    {
        RCLCPP_WARN_THROTTLE(logger, clock, throttle_ms,
                             "Motor %d: kp clamped from %.4f to %.4f (limits [%.4f, %.4f])",
                             motor_id, kp, result.kp, min_kp, max_kp);
    }
    if (result.kd != kd)
    {
        RCLCPP_WARN_THROTTLE(logger, clock, throttle_ms,
                             "Motor %d: kd clamped from %.4f to %.4f (limits [%.4f, %.4f])",
                             motor_id, kd, result.kd, min_kd, max_kd);
    }

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
        LimitedMotorParams limited = apply_limits(
            i,
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
    {
        en_motor_atomic.store(1);
    }
    else if (msg->cmd == tinker_msgs::msg::ControlCmd::DISABLE)
    {
        en_motor_atomic.store(0);
    }
    else if (msg->cmd == tinker_msgs::msg::ControlCmd::SET_ZERO_POSITION)
    {
        if (en_motor_atomic.load() != 0)
        {
            RCLCPP_WARN(this->get_logger(),
                        "SET_ZERO_POSITION: Отклонено - двигатели включены. Сначала отключите двигатели (DISABLE)");
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
        imu_filter_.reset();
        RCLCPP_INFO(this->get_logger(), "IMU calibration requested: acc/gyro/mag set; filter reset");
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
    LimitedMotorParams limited =
        apply_limits(id, msg->position, msg->velocity, msg->torque, msg->kp, msg->kd);
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
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - reset_q_set_time).count();
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

    // SPI att_rate is deg/s — convert to rad/s for the filter.
    // Accel: do not feed raw acc_b. Board convention / legacy offset:
    //   ay + 1, az - 1  (same as old imu_filter_usage_example).
    // With robot upright this yields ~[0, 0, 1] instead of ~[0, -1, 2].
    constexpr float dt = 0.001f;
    constexpr float kDegToRad = static_cast<float>(M_PI / 180.0);
    imu_filter::ImuSample sample;
    sample.gyro = {{
        spi_rx_.att_rate[0] * kDegToRad,
        spi_rx_.att_rate[1] * kDegToRad,
        spi_rx_.att_rate[2] * kDegToRad
    }};
    sample.accel = {{
        spi_rx_.acc_b[0],
        spi_rx_.acc_b[1] + 1.0f,
        spi_rx_.acc_b[2] - 1.0f
    }};
    std::array<float, 4> filt_quat;
    std::array<float, 3> filt_rpy;
    imu_filter_.update(sample, dt, filt_quat, filt_rpy);

    counter++;
    if (counter % 1000 == 0)
    {
        auto now = this->now();
        auto dt_log = (now - last_time).seconds();
        RCLCPP_INFO(this->get_logger(), "SPI frequency: %.1f Hz", 1000.0 / dt_log);
        RCLCPP_INFO(this->get_logger(), "  id | position | velocity |  torque  |");
        for (int i = 0; i < 10; ++i)
        {
            RCLCPP_INFO(this->get_logger(), " %2d  | %8.3f | %8.3f | %8.4f |",
                        i + 1, static_cast<double>(spi_rx_.q[i]),
                        static_cast<double>(spi_rx_.dq[i]), static_cast<double>(spi_rx_.tau[i]));
        }
        last_time = now;
    }

    // RPY from filtered quaternion (filt_rpy already from quatToEuler)
    if (counter % 1000 == 0)
    {
        RCLCPP_INFO(this->get_logger(),
                    "RPY (deg): [%7.2f, %7.2f, %7.2f]",
                    static_cast<double>(filt_rpy[0] * 180.0 / M_PI),
                    static_cast<double>(filt_rpy[1] * 180.0 / M_PI),
                    static_cast<double>(filt_rpy[2] * 180.0 / M_PI));
    }

    tinker_msgs::msg::LowState low_state_msg;
    low_state_msg.timestamp_state = this->now();
    low_state_msg.tick = static_cast<uint32_t>(counter);

    for (int i = 0; i < 10; ++i)
    {
        low_state_msg.motor_state[i].timestamp_state = this->now();
        low_state_msg.motor_state[i].position = static_cast<float>(spi_rx_.q[i]);
        low_state_msg.motor_state[i].velocity = static_cast<float>(spi_rx_.dq[i]);
        low_state_msg.motor_state[i].torque = spi_rx_.tau[i];
        low_state_msg.motor_state[i].temperature_mosfet = 0;  // прошивка пока не передаёт
        low_state_msg.motor_state[i].temperature_rotor = 0;
        // SPI статус-байт: десятки → connected, единицы → enabled (см. MotorState.msg)
        low_state_msg.motor_state[i].connected = spi_rx_.connect_motor[i] != 0;
        low_state_msg.motor_state[i].enabled = spi_rx_.ready[i] != 0;
    }
    low_state_pub_->publish(low_state_msg);

    const auto stamp = this->now();

    geometry_msgs::msg::Quaternion orientation;
    orientation.w = filt_quat[0];
    orientation.x = filt_quat[1];
    orientation.y = filt_quat[2];
    orientation.z = filt_quat[3];
    imu_orientation_pub_->publish(orientation);

    sensor_msgs::msg::Imu imu_msg;
    imu_msg.header.stamp = stamp;
    imu_msg.header.frame_id = "imu_link";
    imu_msg.orientation = orientation;
    // Non-negative → orientation is valid (RViz / estimators)
    imu_msg.orientation_covariance[0] = 0.01;
    imu_msg.orientation_covariance[4] = 0.01;
    imu_msg.orientation_covariance[8] = 0.01;
    // Gyro: rad/s (SPI deg/s already converted in sample)
    imu_msg.angular_velocity.x = sample.gyro[0];
    imu_msg.angular_velocity.y = sample.gyro[1];
    imu_msg.angular_velocity.z = sample.gyro[2];
    imu_msg.angular_velocity_covariance[0] = 0.01;
    imu_msg.angular_velocity_covariance[4] = 0.01;
    imu_msg.angular_velocity_covariance[8] = 0.01;
    // Accel same as filter input (board offset ay+1, az-1)
    imu_msg.linear_acceleration.x = sample.accel[0];
    imu_msg.linear_acceleration.y = sample.accel[1];
    imu_msg.linear_acceleration.z = sample.accel[2];
    imu_msg.linear_acceleration_covariance[0] = 0.01;
    imu_msg.linear_acceleration_covariance[4] = 0.01;
    imu_msg.linear_acceleration_covariance[8] = 0.01;
    imu_pub_->publish(imu_msg);

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
