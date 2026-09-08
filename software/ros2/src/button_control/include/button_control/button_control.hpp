#ifndef BUTTON_CONTROL_HPP
#define BUTTON_CONTROL_HPP

#include <rclcpp/rclcpp.hpp>
#include <tinker_msgs/msg/control_cmd.hpp>
#include <tinker_msgs/msg/low_cmd.hpp>
#include <tinker_msgs/msg/low_state.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace button_control {

class ButtonControl : public rclcpp::Node {
public:
    explicit ButtonControl();

    enum class PoseMotionKind { None, Standing, Lying };

    enum class MotorIndicatorState { Red, Yellow, Green };

    void publishSetZeroCmdMessage();
    void publishStandingPose();
    void publishLyingPose();
    void publishStartMotorsCmdMessage();
    void publishStopMotorsCmdMessage();

    bool hasFeedback() const;
    bool motorsEnabled() const;
    bool motorsDisabled() const;
    bool anyMotorEnabled() const;
    bool hasMotorFault() const;
    bool isAtCompletedPose(PoseMotionKind pose) const;
    bool motionActive() const;
    MotorIndicatorState motorIndicatorState() const;
    std::string motorIndicatorTooltip() const;

private:
    struct MotionParams {
        bool active = false;
        std::array<float, 10> start_positions{};
        std::array<float, 10> target_positions{};
        float duration_sec = 2.0f;
        int elapsed_steps = 0;
        int total_steps = 0;
        float control_freq_hz = 100.0f;
        PoseMotionKind pose_motion = PoseMotionKind::None;
    };

    struct MotorSnapshot {
        std::array<float, 10> positions{};
        std::array<bool, 10> connected{};
        std::array<bool, 10> enabled{};
        bool has_feedback = false;
        std::chrono::steady_clock::time_point last_update{};
    };

    static constexpr int kMotorCount = 10;
    static constexpr std::chrono::milliseconds kFeedbackTimeout{1000};

    rclcpp::Publisher<tinker_msgs::msg::ControlCmd>::SharedPtr control_cmd_publisher_;
    rclcpp::Publisher<tinker_msgs::msg::LowCmd>::SharedPtr low_cmd_publisher_;
    rclcpp::Subscription<tinker_msgs::msg::LowState>::SharedPtr low_state_subscriber_;
    rclcpp::TimerBase::SharedPtr motion_timer_;

    mutable std::mutex state_mutex_;
    MotorSnapshot snapshot_;
    MotionParams motion_params_;
    PoseMotionKind completed_pose_{PoseMotionKind::None};

    void lowStateCallback(const tinker_msgs::msg::LowState::SharedPtr msg);
    void updateSmoothMotion();
    void beginSmoothMotionToTargets(const std::array<float, 10>& targets, PoseMotionKind pose_motion);
    void publishControlCmdForAllMotors(uint8_t cmd);
    void publishZeroGainsCmd();

    static float interpolate(float start, float target, float progress, bool smooth = true);

    MotorSnapshot getSnapshot() const;
    bool feedbackFresh(const MotorSnapshot& snapshot) const;
    bool allMotorsConnected(const MotorSnapshot& snapshot) const;
    bool allMotorsEnabled(const MotorSnapshot& snapshot) const;
    bool allMotorsDisabled(const MotorSnapshot& snapshot) const;
    bool hasPartialMotorConnection(const MotorSnapshot& snapshot) const;
    bool hasPartialMotorEnable(const MotorSnapshot& snapshot) const;
};

}  // namespace button_control

#endif
