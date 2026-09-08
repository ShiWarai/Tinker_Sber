#include "button_control/button_control.hpp"

#include <algorithm>
#include <cmath>

namespace button_control {

namespace {

constexpr std::array<float, 10> kStandingTargets{
    0.0f, 0.0f, -0.2f, -0.9f, -0.5f,
    0.0f, 0.0f, 0.2f, 0.9f, 0.5f};

constexpr std::array<float, 10> kZeroTargets{};

constexpr float kStandingPoseKp = 15.0f;
constexpr float kStandingPoseKd = 0.65f;
constexpr float kLyingPoseKp = 15.0f;
constexpr float kLyingPoseKd = 0.65f;

void fillLowCmdMotorFields(
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

float ButtonControl::interpolate(float start, float target, float progress, bool smooth)
{
    progress = std::clamp(progress, 0.0f, 1.0f);

    if (smooth) {
        const float smooth_progress = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * progress));
        return start + (target - start) * smooth_progress;
    }
    return start + (target - start) * progress;
}

ButtonControl::MotorSnapshot ButtonControl::getSnapshot() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return snapshot_;
}

bool ButtonControl::feedbackFresh(const MotorSnapshot& snapshot) const
{
    if (!snapshot.has_feedback) {
        return false;
    }
    const auto age = std::chrono::steady_clock::now() - snapshot.last_update;
    return age <= kFeedbackTimeout;
}

bool ButtonControl::allMotorsConnected(const MotorSnapshot& snapshot) const
{
    return std::all_of(snapshot.connected.begin(), snapshot.connected.end(), [](bool value) { return value; });
}

bool ButtonControl::allMotorsEnabled(const MotorSnapshot& snapshot) const
{
    return allMotorsConnected(snapshot) &&
           std::all_of(snapshot.enabled.begin(), snapshot.enabled.end(), [](bool value) { return value; });
}

bool ButtonControl::allMotorsDisabled(const MotorSnapshot& snapshot) const
{
    return allMotorsConnected(snapshot) &&
           !std::any_of(snapshot.enabled.begin(), snapshot.enabled.end(), [](bool value) { return value; });
}

bool ButtonControl::hasPartialMotorConnection(const MotorSnapshot& snapshot) const
{
    const bool any_connected = std::any_of(snapshot.connected.begin(), snapshot.connected.end(), [](bool v) { return v; });
    const bool any_disconnected =
        std::any_of(snapshot.connected.begin(), snapshot.connected.end(), [](bool v) { return !v; });
    return any_connected && any_disconnected;
}

bool ButtonControl::hasPartialMotorEnable(const MotorSnapshot& snapshot) const
{
    if (!allMotorsConnected(snapshot)) {
        return false;
    }
    const bool any_enabled = std::any_of(snapshot.enabled.begin(), snapshot.enabled.end(), [](bool v) { return v; });
    const bool any_disabled =
        std::any_of(snapshot.enabled.begin(), snapshot.enabled.end(), [](bool v) { return !v; });
    return any_enabled && any_disabled;
}

ButtonControl::ButtonControl() : Node("button_control")
{
    control_cmd_publisher_ = create_publisher<tinker_msgs::msg::ControlCmd>("/control_command", 20);
    low_cmd_publisher_ = create_publisher<tinker_msgs::msg::LowCmd>("/low_level_command", 20);
    low_state_subscriber_ = create_subscription<tinker_msgs::msg::LowState>(
        "/low_level_state", 20,
        [this](const tinker_msgs::msg::LowState::SharedPtr msg) { lowStateCallback(msg); });

    motion_timer_ = create_wall_timer(
        std::chrono::milliseconds(5), [this]() { updateSmoothMotion(); });
}

void ButtonControl::lowStateCallback(const tinker_msgs::msg::LowState::SharedPtr msg)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    for (int i = 0; i < kMotorCount; ++i) {
        snapshot_.positions[i] = msg->motor_state[i].position;
        snapshot_.connected[i] = msg->motor_state[i].connected;
        snapshot_.enabled[i] = msg->motor_state[i].enabled;
    }
    snapshot_.has_feedback = true;
    snapshot_.last_update = std::chrono::steady_clock::now();
}

bool ButtonControl::hasFeedback() const
{
    return feedbackFresh(getSnapshot());
}

bool ButtonControl::anyMotorEnabled() const
{
    const auto snapshot = getSnapshot();
    if (!feedbackFresh(snapshot)) {
        return false;
    }
    return std::any_of(snapshot.enabled.begin(), snapshot.enabled.end(), [](bool value) { return value; });
}

bool ButtonControl::motorsEnabled() const
{
    const auto snapshot = getSnapshot();
    return feedbackFresh(snapshot) && allMotorsEnabled(snapshot);
}

bool ButtonControl::motorsDisabled() const
{
    const auto snapshot = getSnapshot();
    return feedbackFresh(snapshot) && allMotorsDisabled(snapshot);
}

bool ButtonControl::hasMotorFault() const
{
    const auto snapshot = getSnapshot();
    if (!feedbackFresh(snapshot)) {
        return false;
    }
    return !allMotorsConnected(snapshot) && !hasPartialMotorConnection(snapshot);
}

bool ButtonControl::motionActive() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return motion_params_.active;
}

bool ButtonControl::isAtCompletedPose(PoseMotionKind pose) const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return completed_pose_ == pose;
}

ButtonControl::MotorIndicatorState ButtonControl::motorIndicatorState() const
{
    const auto snapshot = getSnapshot();
    if (!feedbackFresh(snapshot)) {
        return MotorIndicatorState::Red;
    }
    if (hasMotorFault()) {
        return MotorIndicatorState::Red;
    }
    if (hasPartialMotorConnection(snapshot) || hasPartialMotorEnable(snapshot)) {
        return MotorIndicatorState::Yellow;
    }
    if (allMotorsEnabled(snapshot) || allMotorsDisabled(snapshot)) {
        return MotorIndicatorState::Green;
    }
    return MotorIndicatorState::Red;
}

std::string ButtonControl::motorIndicatorTooltip() const
{
    if (motionActive()) {
        return "Motion in progress";
    }

    const auto snapshot = getSnapshot();
    if (!snapshot.has_feedback) {
        return "Waiting for /low_level_state";
    }
    if (!feedbackFresh(snapshot)) {
        return "Motor feedback stale";
    }
    if (hasPartialMotorConnection(snapshot)) {
        return "Partial motor connection";
    }
    if (hasPartialMotorEnable(snapshot)) {
        return "Mixed enable state (not all on or off)";
    }
    if (!allMotorsConnected(snapshot)) {
        return "Motors disconnected";
    }
    if (allMotorsEnabled(snapshot)) {
        return "All motors enabled";
    }
    if (allMotorsDisabled(snapshot)) {
        return "All motors disabled";
    }
    return "Motor state unknown";
}

void ButtonControl::beginSmoothMotionToTargets(
    const std::array<float, 10>& targets, PoseMotionKind pose_motion)
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (!snapshot_.has_feedback || !feedbackFresh(snapshot_)) {
        RCLCPP_WARN(get_logger(), "Cannot start motion: no fresh motor feedback");
        return;
    }

    if (motion_params_.active) {
        RCLCPP_WARN(get_logger(), "Motion already in progress");
        return;
    }

    motion_params_.active = true;
    motion_params_.start_positions = snapshot_.positions;
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
    for (int i = 0; i < kMotorCount; ++i) {
        control_msg.motor_id = static_cast<uint8_t>(i);
        control_msg.cmd = cmd;
        control_cmd_publisher_->publish(control_msg);
        RCLCPP_INFO(get_logger(), "Published: motor_id=%d, cmd=%d", control_msg.motor_id, control_msg.cmd);
    }
}

void ButtonControl::publishStartMotorsCmdMessage()
{
    if (!hasFeedback()) {
        RCLCPP_WARN(get_logger(), "Cannot start motors: no fresh feedback from /low_level_state");
        return;
    }
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // if (motorsEnabled()) {
    //     RCLCPP_WARN(get_logger(), "Motors are already enabled");
    //     return;
    // }
    if (motionActive()) {
        RCLCPP_WARN(get_logger(), "Cannot start motors during motion");
        return;
    }
    publishControlCmdForAllMotors(tinker_msgs::msg::ControlCmd::ENABLE);
}

void ButtonControl::publishStopMotorsCmdMessage()
{
    if (!hasFeedback()) {
        RCLCPP_WARN(get_logger(), "Cannot stop motors: no fresh feedback from /low_level_state");
        return;
    }
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // if (motorsDisabled()) {
    //     RCLCPP_WARN(get_logger(), "Motors are already disabled");
    //     return;
    // }
    if (motionActive()) {
        RCLCPP_WARN(get_logger(), "Cannot stop motors during motion");
        return;
    }
    publishControlCmdForAllMotors(tinker_msgs::msg::ControlCmd::DISABLE);
    std::lock_guard<std::mutex> lock(state_mutex_);
    completed_pose_ = PoseMotionKind::None;
}

void ButtonControl::publishStandingPose()
{
    if (!hasFeedback()) {
        RCLCPP_WARN(get_logger(), "Cannot move to standing pose: no fresh feedback");
        return;
    }
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // if (!motorsEnabled()) {
    //     RCLCPP_WARN(get_logger(), "Cannot move to standing pose: motors are not enabled");
    //     return;
    // }
    if (motionActive()) {
        RCLCPP_WARN(get_logger(), "Cannot move to standing pose: motion already active");
        return;
    }
    if (isAtCompletedPose(PoseMotionKind::Standing)) {
        RCLCPP_WARN(get_logger(), "Already at standing pose");
        return;
    }
    beginSmoothMotionToTargets(kStandingTargets, PoseMotionKind::Standing);
}

void ButtonControl::publishLyingPose()
{
    if (!hasFeedback()) {
        RCLCPP_WARN(get_logger(), "Cannot move to lying pose: no fresh feedback");
        return;
    }
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // if (!motorsEnabled()) {
    //     RCLCPP_WARN(get_logger(), "Cannot move to lying pose: motors are not enabled");
    //     return;
    // }
    if (motionActive()) {
        RCLCPP_WARN(get_logger(), "Cannot move to lying pose: motion already active");
        return;
    }
    if (isAtCompletedPose(PoseMotionKind::Lying)) {
        RCLCPP_WARN(get_logger(), "Already at lying pose");
        return;
    }
    beginSmoothMotionToTargets(kZeroTargets, PoseMotionKind::Lying);
}

void ButtonControl::publishSetZeroCmdMessage()
{
    if (!hasFeedback()) {
        RCLCPP_WARN(get_logger(), "Cannot set zero position: no fresh feedback");
        return;
    }
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // if (anyMotorEnabled()) {
    //     RCLCPP_WARN(get_logger(), "Cannot set zero position while motors are enabled");
    //     return;
    // }
    if (motionActive()) {
        RCLCPP_WARN(get_logger(), "Cannot set zero position during motion");
        return;
    }
    publishControlCmdForAllMotors(tinker_msgs::msg::ControlCmd::SET_ZERO_POSITION);
}

void ButtonControl::updateSmoothMotion()
{
    tinker_msgs::msg::LowCmd cmd;
    PoseMotionKind pose_motion = PoseMotionKind::None;
    std::array<float, 10> target_positions{};
    bool publish_final = false;
    bool publish_step = false;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!motion_params_.active) {
            return;
        }

        const float progress = static_cast<float>(motion_params_.elapsed_steps) /
                               static_cast<float>(motion_params_.total_steps);

        if (progress >= 1.0f) {
            motion_params_.active = false;
            target_positions = motion_params_.target_positions;
            pose_motion = motion_params_.pose_motion;
            completed_pose_ = pose_motion;
            publish_final = true;
        } else {
            for (int i = 0; i < kMotorCount; ++i) {
                const float pos = interpolate(
                    motion_params_.start_positions[i],
                    motion_params_.target_positions[i],
                    progress,
                    true);
                fillLowCmdMotorFields(cmd, i, pos, motion_params_.pose_motion);
            }
            motion_params_.elapsed_steps++;
            publish_step = true;
        }
    }

    if (publish_final) {
        RCLCPP_INFO(get_logger(), "Smooth motion completed");
        tinker_msgs::msg::LowCmd final_cmd;
        for (int i = 0; i < kMotorCount; ++i) {
            fillLowCmdMotorFields(final_cmd, i, target_positions[i], pose_motion);
        }
        low_cmd_publisher_->publish(final_cmd);
        return;
    }

    if (publish_step) {
        low_cmd_publisher_->publish(cmd);
    }
}

}  // namespace button_control
