/**
 * @file imu_filter_usage_example.hpp
 * @brief Пример использования IMU фильтра в ROS2 узле
 * 
 * Этот файл показывает как интегрировать ImuFilter в MotorControlNode.
 */

#ifndef IMU_FILTER_USAGE_EXAMPLE_HPP
#define IMU_FILTER_USAGE_EXAMPLE_HPP

#include "motor_control_node.hpp"
#include "imu_filter/imu_filter.hpp"

// Пример класса для демонстрации интеграции (не часть основного узла)
class MotorControlNodeWithImuFilter : public MotorControlNode {
public:
    MotorControlNodeWithImuFilter() : MotorControlNode() {
        // Инициализируем фильтр с настройками по умолчанию
        imu_filter_ = std::make_shared<imu_filter::ImuFilter>();
        
        RCLCPP_INFO(get_logger(), "IMU Filter initialized");
    }

protected:
    void on_timer() override {
        // 1. Сначала вызываем родительский метод (SPI коммуникация)
        MotorControlNode::on_timer();
        
        // 2. Получаем сырые данные IMU
        imu_filter::IMUData raw_imu;
        for (int i = 0; i < 3; ++i) {
            raw_imu.gyroscope[i] = spi_rx_.att_rate[i];
            raw_imu.accelerometer[i] = spi_rx_.acc_b[i];
        }
        
        // 3. Применяем фильтр
        auto filtered = imu_filter_->filter(raw_imu);
        
        // 4. Используем отфильтрованные данные вместо сырых
        // Пример: заменяем Publish сообщения с отфильтрованными данными
        tinker_msgs::msg::IMUState imu_msg;
        imu_msg.timestamp_state = this->now();
        
        // Отфильтрованный гироскоп
        for (int i = 0; i < 3; ++i) {
            imu_msg.gyroscope[i] = filtered.gyroscope[i];
        }
        
        // Отфильтрованный акселерометр с корректировкой
        imu_msg.accelerometer[0] = filtered.accelerometer[0];
        imu_msg.accelerometer[1] = filtered.accelerometer[1] + 1.0f;
        imu_msg.accelerometer[2] = filtered.accelerometer[2] - 1.0f;
        
        // Также можно получить оценку смещения гироскопа
        auto bias = imu_filter_->getEstimatedGyroBias();
        RCLCPP_INFO_THROTTLE(get_logger(), *this->get_clock(), 5000,\n            "Gyro bias estimate: [%.6f, %.6f, %.6f] rad/s",\n            bias[0], bias[1], bias[2]);
        
        // Публикуем отфильтрованные данные
        if (imu_pub_) {
            imu_pub_->publish(imu_msg);
        }
    }

private:
    imu_filter::ImuFilter::Ptr imu_filter_; // Умный указатель на фильтр
};

#endif /* IMU_FILTER_USAGE_EXAMPLE_HPP */