#include "button_control/button_control.hpp"

namespace button_control {

namespace {

/** Standing: left leg 0–4; right leg 5–9 uses the same targets (mirrored leg pose). */
constexpr std::array<float, 10> kStandingTargets{
    0.0f, 0.0f, -0.2f, -0.9f, -0.5f,
    0.0f, 0.0f, 0.2f, 0.9f, 0.5f};

constexpr std::array<float, 10> kZeroTargets{};

constexpr float kStandingPoseKp = 15.0f;
constexpr float kStandingPoseKd = 0.65f;
constexpr float kLyingPoseKp = 15.0f;
constexpr float kLyingPoseKd = 0.65f;

static void fill_low_cmd_motor_fields(
    tinker_msgs::msg::LowCmd& cmd, int motor_index, float position, ButtonControl::PoseMotionKind pose_motion)
{
    cmd.motor_cmd[motor_index].position = position;
    cmd.motor_cmd[motor_index].velocity = 0.0f;
    cmd.motor_cmd[motor_index].torque = 0.0f;
    const int i = motor_index;
    switch (pose_motion) {
        case ButtonControl::PoseMotionKind::Standing:
            cmd.motor_cmd[i].kp = kStandingPoseKp;
            cmd.motor_cmd[i].kd = kStandingPoseKd;
            break;
        case ButtonControl::PoseMotionKind::Lying:
            cmd.motor_cmd[i].kp = kLyingPoseKp;
            cmd.motor_cmd[i].kd = kLyingPoseKd;
            break;
        case ButtonControl::PoseMotionKind::None:
        default:
            cmd.motor_cmd[i].kp = (i == 0 || i == 4 || i == 5 || i == 9) ? 13.0f : 15.0f;
            cmd.motor_cmd[i].kd = (i == 0 || i == 4 || i == 5 || i == 9) ? 0.3f : 0.65f;
            break;
    }
}

}  // namespace

float ButtonControl::interpolate(float start, float target, float progress, bool smooth) {
    
    progress = std::clamp(progress, 0.0f, 1.0f);
    
    if (smooth) {
        // Sinusoidal ease-in-out: плавный старт и остановка

        float smooth_progress = 0.5f * (1.0f - std::cos(M_PI * progress));
        // float smooth_progress = 3 * progress * progress - 2 * progress * progress * progress;
        // float smooth_progress = progress;
        return start + (target - start) * smooth_progress;
    }
    // Линейная интерполяция 
    return start + (target - start) * progress;
}

ButtonControl::ButtonControl() 
: Node("button_control"), count_(0) {

    // Callback для обратной связи
    auto low_state_callback = [this](tinker_msgs::msg::LowState::SharedPtr msg) -> void {
        lowStateCallback(msg);
    };

    control_cmd_publisher_ = this->create_publisher<tinker_msgs::msg::ControlCmd>(
        "/control_command", 20);
    low_cmd_publisher_ = this->create_publisher<tinker_msgs::msg::LowCmd>(
        "/low_level_command", 20);
    low_state_subscriber_ = this->create_subscription<tinker_msgs::msg::LowState>(
        "/low_level_state", 20, low_state_callback);
    
    // Таймер для обновления плавного движения (100 Гц)
    motion_timer_ = this->create_wall_timer(
        std::chrono::milliseconds(5), [this]() { 
        updateSmoothMotion();
    });
}

// Callback: сохраняем текущие позиции двигателей
void ButtonControl::lowStateCallback(const tinker_msgs::msg::LowState::SharedPtr msg) {
    for (int i = 0; i < 10; i++) {
        last_known_positions_[i] = msg->motor_state[i].position;
    }
    positions_initialized_ = true;
}

void ButtonControl::beginSmoothMotionToTargets(const std::array<float, 10>& targets, PoseMotionKind pose_motion)
{
    if (!positions_initialized_) {
        RCLCPP_WARN(this->get_logger(),
                    "Positions not initialized yet, waiting for feedback...");
        return;
    }

    if (motion_params_.active) {
        RCLCPP_WARN(this->get_logger(), "Motion already in progress!");
        return;
    }

    motion_params_.active = true;
    motion_params_.start_positions = last_known_positions_;
    motion_params_.target_positions = targets;
    motion_params_.pose_motion = pose_motion;

    motion_params_.duration_sec = 2.0f;
    motion_params_.control_freq_hz = 200.0f;
    motion_params_.total_steps =
        static_cast<int>(motion_params_.duration_sec * motion_params_.control_freq_hz);
    motion_params_.elapsed_steps = 1;
}

void ButtonControl::publishControlCmdForAllMotors(uint8_t cmd)
{
    tinker_msgs::msg::ControlCmd control_msg;
    for (int i = 0; i < 10; i++) {
        control_msg.motor_id = static_cast<uint8_t>(i);
        control_msg.cmd = cmd;
        control_cmd_publisher_->publish(control_msg);
        RCLCPP_INFO(this->get_logger(),
                    "Published: motor_id=%d, cmd=%d",
                    control_msg.motor_id, control_msg.cmd);
    }
}

void ButtonControl::publishStartMotorsCmdMessage()
{
    publishControlCmdForAllMotors(252);
}

void ButtonControl::publishStopMotorsCmdMessage()
{
    publishControlCmdForAllMotors(253);
}

void ButtonControl::publishStandingPose()
{
    beginSmoothMotionToTargets(kStandingTargets, PoseMotionKind::Standing);
}

void ButtonControl::publishLyingPose()
{
    beginSmoothMotionToTargets(kZeroTargets, PoseMotionKind::Lying);
}


// Обновление траектории (вызывается таймером)
void ButtonControl::updateSmoothMotion() {

    if (!motion_params_.active) {
        return;  // Движение не активно
    }
    
    // Вычисляем прогресс [0, 1]
    float progress = static_cast<float>(motion_params_.elapsed_steps) / 
                     static_cast<float>(motion_params_.total_steps);
    
    if (progress >= 1.0f) {
        // Движение завершено
        motion_params_.active = false;
        RCLCPP_INFO(this->get_logger(), "Smooth motion completed!");
        
        // Финальная команда: зафиксировать целевые позиции траектории
        tinker_msgs::msg::LowCmd final_cmd;
        for (int i = 0; i < 10; i++) {
            fill_low_cmd_motor_fields(final_cmd, i, motion_params_.target_positions[i],
                                      motion_params_.pose_motion);
        }
        low_cmd_publisher_->publish(final_cmd);
        return;
    }
    
    // Формируем команду с интерполированными позициями
    tinker_msgs::msg::LowCmd cmd;
    
    for (int i = 0; i < 10; i++) {
        // Интерполяция позиции с плавным easing
        const float pos = interpolate(
            motion_params_.start_positions[i],
            motion_params_.target_positions[i],
            progress,
            true  // использовать smooth easing
        );
        fill_low_cmd_motor_fields(cmd, i, pos, motion_params_.pose_motion);
    }
    
    low_cmd_publisher_->publish(cmd);
    motion_params_.elapsed_steps++;
    
    // // Лог прогресса каждые 10%
    // if (motion_params_.elapsed_steps % 10 == 0) {
    //     RCLCPP_INFO(this->get_logger(), 
    //                "Motion progress: %.1f%% (step %d/%d)",
    //                progress * 100.0f,
    //                motion_params_.elapsed_steps,
    //                motion_params_.total_steps);
    // }
}


void ButtonControl::publishSetZeroCmdMessage()
{
    publishControlCmdForAllMotors(253);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    publishControlCmdForAllMotors(254);
}

} // namespace button_control
