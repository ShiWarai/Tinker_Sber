#ifndef BUTTON_CONTROL_HPP
#define BUTTON_CONTROL_HPP

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <tinker_msgs/msg/control_cmd.hpp>
#include <tinker_msgs/msg/imu_state.hpp>
#include <tinker_msgs/msg/low_cmd.hpp>
#include <tinker_msgs/msg/low_state.hpp>
#include <tinker_msgs/msg/motor_cmd.hpp>
#include <tinker_msgs/msg/motor_state.hpp>
#include <tinker_msgs/msg/one_motor_cmd.hpp>

#include <memory>

#include <thread>
#include <chrono>
#include <iostream>

#include <array>
#include <cstdint>
#include <optional>

namespace button_control {

class ButtonControl : public rclcpp::Node {
public:
    explicit ButtonControl();

    enum class PoseMotionKind { None, Standing, Lying };
    
    //Публикации из GUI
    void publishSetZeroCmdMessage();
    /** Smooth motion to standing pose (motors 0–4 and mirrored 5–9). */
    void publishStandingPose();
    /** Smooth motion to lying pose (all joint angles zero). */
    void publishLyingPose();
    /** ControlCmd 252 (enable) for motors 0–9. */
    void publishStartMotorsCmdMessage();
    /** ControlCmd 253 (disable) for motors 0–9. */
    void publishStopMotorsCmdMessage();

    void updateSmoothMotion();
    
private:
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<tinker_msgs::msg::ControlCmd>::SharedPtr control_cmd_publisher_;
    rclcpp::Publisher<tinker_msgs::msg::LowCmd>::SharedPtr low_cmd_publisher_;
    rclcpp::Publisher<tinker_msgs::msg::MotorCmd>::SharedPtr motor_cmd_publisher_;
    rclcpp::Publisher<tinker_msgs::msg::OneMotorCmd>::SharedPtr one_motor_cmd_publisher_;   
    int count_;

private:
    rclcpp::Subscription<tinker_msgs::msg::IMUState>::SharedPtr imu_state_subscriber_;
    rclcpp::Subscription<tinker_msgs::msg::LowState>::SharedPtr low_state_subscriber_;
    rclcpp::Subscription<tinker_msgs::msg::MotorState>::SharedPtr motor_state_subscriber_;  
  
        // Таймер для контроля движения
    rclcpp::TimerBase::SharedPtr motion_timer_;
    
    // Параметры плавного движения
    struct MotionParams {
        bool active = false;
        std::array<float, 10> start_positions;  // Начальные позиции (из feedback)
        std::array<float, 10> target_positions; // Целевые позиции (обычно 0.0)
        float duration_sec = 2.0f;              // Длительность движения в секундах
        int elapsed_steps = 0;                  // Счётчик шагов
        int total_steps = 0;                    // Общее количество шагов
        float control_freq_hz = 100.0f;         // Частота обновления (Гц)
        PoseMotionKind pose_motion = PoseMotionKind::None;
    };
    
    MotionParams motion_params_;

        // Хранение последних полученных позиций для инициализации траектории
    std::array<float, 10> last_known_positions_;
    bool positions_initialized_ = false;
    std::atomic<bool> positions_ready_{false}; 
    
    // Callback для получения обратной связи
    void lowStateCallback(const tinker_msgs::msg::LowState::SharedPtr msg);

    void beginSmoothMotionToTargets(const std::array<float, 10>& targets, PoseMotionKind pose_motion);

    void publishControlCmdForAllMotors(uint8_t cmd);

    // Вспомогательная функция интерполяции
    static float interpolate(float start, float target, float progress, bool smooth = true);
};

} // namespace button_control

#endif