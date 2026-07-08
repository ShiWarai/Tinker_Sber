/**
 * @file imu_filter.hpp
 * @brief Реализация IMU фильтра на основе алгоритма ошибки ориентации (Error State Kalman Filter)
 * 
 * Фильтр использует шестиосевую структуру для фильтрации гироскопа и акселерометра.
 * Алгоритм отслеживает ошибки ориентации, смещения гироскопа и линейного ускорения.
 */

#ifndef IMU_FILTER_IMU_FILTER_HPP
#define IMU_FILTER_IMU_FILTER_HPP

#include <array>
#include <cmath>
#include <string>
#include <memory>

namespace imu_filter {

/** Структура для хранения данных IMU */
struct IMUData {
    std::array<float, 3> gyroscope;   // угловая скорость [rad/s] по осям X, Y, Z
    std::array<float, 3> accelerometer; // ускорение [g] по осям X, Y, Z
};

/** Структура для хранения отфильтрованных данных IMU */
struct FilteredIMU {
    std::array<float, 3> gyroscope;   // отфильтрованная угловая скорость
    std::array<float, 3> accelerometer; // отфильтрованное ускорение
    
    /** Дополнительно: оценка смещения гироскопа */
    std::array<float, 3> gyro_bias;
    
    /** Дополнительно: ошибка ориентации [rad] */
    std::array<float, 3> orientation_error;
};

/** Параметры фильтра Калмана */
struct ImuFilterParams {
    // Шум процесса (процессный шум) - определяет насколько "быстрыми" мы считаем изменения состояния
    float process_noise_orientation = 0.01f;   // шум ошибки ориентации
    float process_noise_gyro_bias = 0.0001f;   // шум смещения гироскопа
    float process_noise_accel = 0.01f;         // шум линейного ускорения
    
    // Шум измерений - определяет насколько мы доверяем сенсорам
    float measurement_noise_gyro = 0.01f;      // шум гироскопа
    float measurement_noise_accel = 0.1f;      // шум акселерометра
    
    // Частота фильтрации (Гц) - важно для корректной работы дисcretization
    float update_frequency = 100.0f;           // частота вызова filter()
    
    // Количество сэмплов для калибровки гироскопа при старте
    int calibration_samples = 500;            // 5 секунд при 100Hz
};

/**
 * Класс IMU фильтра на основе Error State Kalman Filter (ESKF).
 * 
 * Фильтр моделирует процесс ошибки с состоянием:
 * x = [θ (3×1) - ошибка ориентации, 
 *       b (3×1) - смещение гироскопа, 
 *       a (3×1) - ошибка ускорения]
 */
class ImuFilter {
public:
    using Ptr = std::shared_ptr<ImuFilter>;
    using ConstPtr = std::shared_ptr<const ImuFilter>;
    
    /** Конструктор с настройкой параметров по умолчанию */
    ImuFilter();
    
    /** Конструктор с пользовательскими параметрами */
    explicit ImuFilter(const ImuFilterParams& params);
    
    /** Деструктор */
    virtual ~ImuFilter() = default;
    
    /**
     * Основная функция фильтрации - принимает сырые данные IMU и возвращает отфильтрованные
     * @param raw_imu сырые данные гироскопа и акселерометра
     * @return отфильтрованные данные IMU
     */
    FilteredIMU filter(const IMUData& raw_imu);
    
    /** Сброс фильтра (первоначальная инициализация) */
    void reset();
    
    /** Обновление параметров во время работы */
    void setParams(const ImuFilterParams& params);
    
    // ====== Методы для интеграции с ROS2 ======
    
    /**
     * Инициализация фильтра из параметров ros2 (для declare_parameter)
     * @param prefix префикс параметров (например, "imu_filter")
     */
    void initFromRosParams(const std::string& prefix);
    
    /**
     * Начинает стадию калибровки гироскопа (имеет смысл когда робот неподвижен)
     */
    void startGyroCalibration();
    
    /**
     * Завершает стадию калибровки и применяет смещения
     */
    void finishGyroCalibration();
    
    /**
     * Получение текущей оценки смещения гироскопа
     * @return массив [bias_x, bias_y, bias_z]
     */
    std::array<float, 3> getEstimatedGyroBias() const;
    
    /**
     * Проверяет, находится ли фильтр в стадии калибровки
     */
    bool isCalibrating() const;
    
    /**
     * Получение текущей оценки ковариации состояния (для диагностики)
     */
    std::array<float, 81> getCovarianceMatrix() const;

protected:
    // Внутренние переменные состояния фильтра (9 элементов)
    // State: [orientation_error(3), gyro_bias(3), accel_error(3)]
    std::array<float, 9> state_ = {{0.0f}};
    
    // Матрица ковариации состояния P (9x9) - хранится в виде одномерного массива
    // Индекс: P[i*9 + j] для элемента [i][j]
    std::array<float, 81> covariance_ = {{0.0f}};
    
    // Переменные для калибровки гироскопа
    int calibration_counter_ = 0;
    std::array<float, 3> gyro_calibration_accum_ = {{0.0f}};
    bool is_calibrating_ = false;
    ImuFilterParams params_;

private:
    /** Предсказание состояния на следующий шаг */
    void predict(float dt);
    
    /** Обновление фильтра на основе измерений */
    void update(const IMUData& raw_imu);
    
    /** Вычисление матрицы усиления Калмана K = P⁻·Hᵀ·(H·P⁻·Hᵀ + R)⁻¹ */
    std::array<float, 54> computeKalmanGain();
    
    /** Нормализация вектора ошибок ориентации в диапазон [-π, π] */
    void normalizeOrientationError();
    
    /** Получить индекс состояния для компонента orient/bias/accel */
    int stateIndex(int component, int axis) const;
};

} // namespace imu_filter

#endif /* IMU_FILTER_IMU_FILTER_HPP */