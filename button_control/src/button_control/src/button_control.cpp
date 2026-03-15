#include "button_control/button_control.hpp"

namespace button_control {

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
        "tinker_msgs/controlcmd", 20);
    low_cmd_publisher_ = this->create_publisher<tinker_msgs::msg::LowCmd>(
        "tinker_msgs/lowcmd", 20);
    low_state_subscriber_ = this->create_subscription<tinker_msgs::msg::LowState>(
        "tinker_msgs/lowstate", 20, low_state_callback);
    
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

// Запуск плавного движения к нулю
void ButtonControl::publishMoveZeroCmdMessage() {
    
    if (!positions_initialized_) {
        RCLCPP_WARN(this->get_logger(), 
                   "Positions not initialized yet, waiting for feedback...");
        return;
    }
    
    if (motion_params_.active) {
        RCLCPP_WARN(this->get_logger(), "Motion already in progress!");
        return;
    }
    
    // Инициализация параметров движения
    motion_params_.active = true;
    motion_params_.start_positions = last_known_positions_;
    
    // Целевые позиции = 0.0 для всех моторов
    for (int i = 0; i < 10; i++) {
        motion_params_.target_positions[i] = 0.0f;
    }
    
    motion_params_.duration_sec = 2.0f; 
    motion_params_.control_freq_hz = 200.0f;
    motion_params_.total_steps = static_cast<int>(motion_params_.duration_sec * motion_params_.control_freq_hz);
    motion_params_.elapsed_steps = 1;
    
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
        
        // Финальная команда: зафиксировать позицию
        tinker_msgs::msg::LowCmd final_cmd;
        for (int i = 0; i < 10; i++) {
            final_cmd.motor_cmd[i].position = 0.0f;
            final_cmd.motor_cmd[i].kp = (i == 0 || i == 4 || i == 5 || i == 9) ? 13.0f : 15.0f;
            final_cmd.motor_cmd[i].kd = (i == 0 || i == 4 || i == 5 || i == 9) ? 0.3f : 0.65f;
            final_cmd.motor_cmd[i].torque = 0.0f;  // Сброс крутящего момента
        }
        low_cmd_publisher_->publish(final_cmd);
        return;
    }
    
    // Формируем команду с интерполированными позициями
    tinker_msgs::msg::LowCmd cmd;
    
    for (int i = 0; i < 10; i++) {
        // Интерполяция позиции с плавным easing
        cmd.motor_cmd[i].position = interpolate(
            motion_params_.start_positions[i],
            motion_params_.target_positions[i],
            progress,
            true  // использовать smooth easing
        );
        
        // Параметры контроллера (можно тоже интерполировать при необходимости)
        cmd.motor_cmd[i].kp = (i == 0 || i == 4 || i == 5 || i == 9) ? 13.0f : 15.0f;
        cmd.motor_cmd[i].kd = (i == 0 || i == 4 || i == 5 || i == 9) ? 0.3f : 0.65f;
        cmd.motor_cmd[i].torque = 0.0f;
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


void ButtonControl::publishSetZeroCmdMessage() {
    

    tinker_msgs::msg::ControlCmd control_msg;

    for (int i = 0; i < 10; i++){

        control_msg.motor_id = i;
        control_msg.cmd = 253;
        
        control_cmd_publisher_->publish(control_msg);
        RCLCPP_INFO(this->get_logger(), 
                "Published: motor_id=%d, cmd=%d", 
                control_msg.motor_id, control_msg.cmd);      
    }
    
    //Временная задержка после команды 253

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (int i = 0; i < 10; i++){

        control_msg.motor_id = i;
        control_msg.cmd = 254;
        
        control_cmd_publisher_->publish(control_msg);
        RCLCPP_INFO(this->get_logger(), 
                "Published: motor_id=%d, cmd=%d", 
                control_msg.motor_id, control_msg.cmd);      

    }
    

}

// void ButtonControl::publishMoveZeroCmdMessage(/*const tinker_msgs::msg::LowCmd& low_msg, const tinker_msgs::msg::MotorCmd& motor_msg*/) {
    
//     low_state_subscriber_->
//     tinker_msgs::msg::LowCmd low_cmd_msg;   

//     for (int i = 0; i < 10; i++){

//         low_cmd_msg.motor_cmd[i].position = 0.0; 

//         if (i == 0 || i == 4 || i == 5 || i == 9){
            
//             low_cmd_msg.motor_cmd[i].kp = 13;
//             low_cmd_msg.motor_cmd[i].kd = 0.3;

//         }
//         else {

//             low_cmd_msg.motor_cmd[i].kp = 15;
//             low_cmd_msg.motor_cmd[i].kd = 0.65;

//         }

//     }
//     low_cmd_publisher_->publish(low_cmd_msg);
//     RCLCPP_INFO(this->get_logger(), 
//             "Published: position=%.1f, kp=%.1f, kd=%.1f", 
//             low_cmd_msg.motor_cmd[0].position,low_cmd_msg.motor_cmd[0].kp, low_cmd_msg.motor_cmd[0].kd);      

// }


void ButtonControl::publishActivateMotorsCmdMessage(){

    tinker_msgs::msg::ControlCmd control_msg;

    for (int i = 0; i < 10; i++){

        control_msg.motor_id = i;
        control_msg.cmd = 252;
        
        control_cmd_publisher_->publish(control_msg);
        RCLCPP_INFO(this->get_logger(), 
                "Published: motor_id=%d, cmd=%d", 
                control_msg.motor_id, control_msg.cmd);      
    }
    

}

} // namespace button_control