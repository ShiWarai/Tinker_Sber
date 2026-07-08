/**
 * @file imu_filter.cpp
 * @brief Реализация IMU Error State Kalman Filter (ESKF)
 */

#include "imu_filter/imu_filter.hpp"
#include <iostream>
#include <algorithm>
#include <rclcpp/rclcpp.hpp>

namespace imu_filter {

// Helper: умножение матрицы 3x3 на вектор 3x1
static std::array<float, 3> multiplyMatrixVector(const float* matrix, const std::array<float, 3>& vector) {
    std::array<float, 3> result = {{0.0f, 0.0f, 0.0f}};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result[i] += matrix[i * 3 + j] * vector[j];
        }
    }
    return result;
}

// Helper: сложение двух векторов 3x1
static std::array<float, 3> addVectors(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return {{a[0] + b[0], a[1] + b[1], a[2] + b[2]}};
}

// Helper: вычитание двух векторов 3x1
static std::array<float, 3> subtractVectors(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return {{a[0] - b[0], a[1] - b[1], a[2] - b[2]}};
}

// Helper: умножение вектора на скаляр
static std::array<float, 3> scaleVector(const std::array<float, 3>& vector, float scalar) {
    return {{vector[0] * scalar, vector[1] * scalar, vector[2] * scalar}};
}

/**
 * Helper: инверсия квадратной матрицы N×N (алгоритм Гаусса-Жордана)
 */
static bool invertMatrix(float* input, float* output, int N) {
    // Создаём расширенную матрицу [A|I]
    float aug[12][24];  // Максимум 12×12
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            aug[i][j] = input[i * N + j];
        }
        aug[i][N + i] = 1.0f;
    }

    // Гауссова элиминация с частичным выбором ведущего элемента
    for (int col = 0; col < N; ++col) {
        // Ищем строку с максимальным значением в столбце
        float maxVal = std::abs(aug[col][col]);
        int maxRow = col;
        for (int row = col + 1; row < N; ++row) {
            if (std::abs(aug[row][col]) > maxVal) {
                maxVal = std::abs(aug[row][col]);
                maxRow = row;
            }
        }

        // Обмен строк
        if (maxRow != col) {
            for (int j = 0; j < 2 * N; ++j) {
                std::swap(aug[col][j], aug[maxRow][j]);
            }
        }

        // Проверяем вырожденность матрицы
        if (std::abs(aug[col][col]) < 1e-9f) {
            return false;
        }

        // Нормализация строки
        float pivot = aug[col][col];
        for (int j = 0; j < 2 * N; ++j) {
            aug[col][j] /= pivot;
        }

        // Элиминация столбца
        for (int row = 0; row < N; ++row) {
            if (row != col) {
                float factor = aug[row][col];
                for (int j = 0; j < 2 * N; ++j) {
                    aug[row][j] -= factor * aug[col][j];
                }
            }
        }
    }

    // Извлекаем обратную матрицу
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            output[i * N + j] = aug[i][N + j];
        }
    }

    return true;
}

// ==================== Конструкторы и деструктор ====================

ImuFilter::ImuFilter() : ImuFilter(ImuFilterParams{}) {}

ImuFilter::ImuFilter(const ImuFilterParams& params) 
    : params_(params) {
    // Заполняем всю матрицу нулями
    covariance_.fill(0.0f);
    
    // Устанавливаем диагональ (начальная неопределенность)
    for (int i = 0; i < 9; ++i) {
        covariance_[i * 9 + i] = 1.0f; // Или 10.0f, если хотим, чтобы фильтр быстрее сходился
    }
}

void ImuFilter::reset() {
    state_.fill(0.0f);
    
    // Сбрасываем ковариацию к начальным значениям
    for (int i = 0; i < 81; ++i) {
        covariance_[i] = (i % 9 == i / 9) ? 1.0f : 0.0f;
    }
}

void ImuFilter::setParams(const ImuFilterParams& params) {
    params_ = params;
}

// ==================== Вспомогательные методы ====================

int ImuFilter::stateIndex(int component, int axis) const {
    // component: 0=orientation, 1=gyro_bias, 2=accel_error
    // axis: 0,1,2 для X,Y,Z
    return component * 3 + axis;
}

void ImuFilter::normalizeOrientationError() {
    // Нормализуем ошибку ориентации в диапазон [-PI, PI]
    const float PI = 3.14159265358979f;
    for (int i = 0; i < 3; ++i) {
        while (state_[i] > PI) state_[i] -= 2.0f * PI;
        while (state_[i] < -PI) state_[i] += 2.0f * PI;
    }
}

// ==================== Основные методы фильтра ====================

void ImuFilter::predict(float dt) {
    if (dt <= 0.0f) return;
    
    // Для ESKF с нулевым состоянием по умолчанию:
    // x⁻ = Φ·x⁺ ≈ 0 (так как состояние - это ошибка)
    // P⁻ = Φ·P⁺·Φᵀ + Q
    
    const float dt_real = static_cast<float>(dt);
    
    // Матрица перехода состояния Φ (упрощённая модель)
    // Φ = I + A*dt, где A - матрица динамики системы
    // Для нашей модели: Φ ≈ I (предполагаем медленные изменения ошибки)
    
    // Процессный шум Q (дискретизированный)
    float Q[81] = {0.0f};
    for (int i = 0; i < 9; ++i) {
        float noise;
        if (i < 3) {
            // Ошибка ориентации растет со временем
            noise = params_.process_noise_orientation * dt_real;
        } else if (i < 6) {
            // Смещение гироскопа - случайное блуждание
            noise = params_.process_noise_gyro_bias * std::sqrt(dt_real);
        } else {
            // Ошибка ускорения
            noise = params_.process_noise_accel * dt_real;
        }
        Q[i * 9 + i] = noise * noise;
    }
    
    // P⁻ = P⁺ + Q (упрощение: Φ ≈ I)
    for (int i = 0; i < 81; ++i) {
        covariance_[i] += Q[i];
    }
    
    // Ограничиваем максимальную ковариацию для стабильности
    for (int i = 0; i < 9; ++i) {
        if (covariance_[i * 9 + i] > 100.0f) {
            covariance_[i * 9 + i] = 100.0f;
        }
    }
}

/**
 * Вычисление матрицы усиления Калмана: K = P⁻·Hᵀ·(H·P⁻·Hᵀ + R)⁻¹
 * 
 * H (6×9): матрица наблюдений - связывает состояние с измерениями
 * R (6×6): ковариация шумов измерений
 * P⁻ (9×9): априорная ковариация состояния
 */
std::array<float, 54> ImuFilter::computeKalmanGain() {
    // H матрица (6x9) - что измеряем из состояния:
    // Строки 0-2: гироскоп -> orientation_error (0-2) + gyro_bias (3-5)
    // Строки 3-5: акселерометр -> accel_error (6-8)
    float H[54] = {0.0f};
    for (int i = 0; i < 3; ++i) {
        H[i * 9 + i] = 1.0f;             // gyro влияет на orientation_error
        H[i * 9 + (3 + i)] = 1.0f;       // gyro влияет на gyro_bias
        H[(3 + i) * 9 + (6 + i)] = 1.0f; // acc влияет на accel_error
    }
    
    // R - диагональная матрица шумов измерений (6x6)
    float R[36] = {0.0f};
    for (int i = 0; i < 3; ++i) {
        R[i * 6 + i] = params_.measurement_noise_gyro;
        R[(3 + i) * 6 + (3 + i)] = params_.measurement_noise_accel;
    }
    
    // Вычисляем PHᵀ = P⁻·Hᵀ (9x6)
    float PHt[54] = {0.0f};
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 6; ++j) {
            for (int k = 0; k < 9; ++k) {
                PHt[i * 6 + j] += covariance_[i * 9 + k] * H[j * 9 + k];
            }
        }
    }
    
    // Вычисляем S = H·PHᵀ + R (6x6) - ковариация инноваций
    float S[36] = {0.0f};
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            for (int k = 0; k < 9; ++k) {
                S[i * 6 + j] += H[i * 9 + k] * PHt[k * 6 + j];
            }
            // Добавляем R
            S[i * 6 + j] += R[i * 6 + j];
        }
    }
    
    // Инвертируем S → S⁻¹ (6x6)
    float S_inv[36];
    if (!invertMatrix(S, S_inv, 6)) {
        std::cerr << "[ImuFilter] Warning: Could not invert innovation covariance matrix" << std::endl;
        // Возвращаем единичную матрицу как fallback
        for (int i = 0; i < 36; ++i) {
            S_inv[i] = (i % 7 == 0) ? 1.0f : 0.0f;
        }
    }
    
    // K = PHᵀ·S⁻¹ (9x6)
    float K[54] = {0.0f};
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 6; ++j) {
            for (int k = 0; k < 6; ++k) {
                K[i * 6 + j] += PHt[i * 6 + k] * S_inv[k * 6 + j];
            }
        }
    }
    
    // Копируем результат в std::array
    std::array<float, 54> result;
    for (int i = 0; i < 54; ++i) {
        result[i] = K[i];
    }
    return result;
}

/**
 * Обновление фильтра на основе измерений гироскопа и акселерометра
 * 
 * Формулы ESKF:
 * - Инновация: y = z - H·x⁻ (разница между измерением и предсказанием)
 * - Прибыль Калмана: K = P⁻·Hᵀ·(H·P⁻·Hᵀ + R)⁻¹
 * - Обновление состояния: x⁺ = x⁻ + K·y
 * - Обновление ковариации: P⁺ = (I - K·H)·P⁻
 */
void ImuFilter::update(const IMUData& raw_imu) {
    const float dt_real = 1.0f / params_.update_frequency;
    predict(dt_real);
    
    // Инновация: y = z - H·x⁻ (измерения минус предсказанные)
    // При x⁻ ≈ 0 для ESKF, инновация равна самим измерениям
    float innovation[6];
    for (int i = 0; i < 3; ++i) {
        innovation[i] = raw_imu.gyroscope[i];         // гироскоп: ω_meas - 0
        innovation[3 + i] = raw_imu.accelerometer[i]; // акселерометр: a_meas - 0
    }
    
    // Вычисляем полную матрицу усиления Калмана (9x6)
    std::array<float, 54> K_arr = computeKalmanGain();
    float K[54];
    for (int i = 0; i < 54; ++i) K[i] = K_arr[i];
    
    // Проверка на NaN в матрице усиления
    bool has_nan = false;
    for (int i = 0; i < 54 && !has_nan; ++i) {
        if (std::isnan(K[i]) || std::isinf(K[i])) has_nan = true;
    }
    
    if (!has_nan) {
        // Обновляем состояние: x⁺ = x⁻ + K·y
        for (int i = 0; i < 9; ++i) {
            float delta = 0.0f;
            for (int j = 0; j < 6; ++j) {
                delta += K[i * 6 + j] * innovation[j];
            }
            state_[i] += delta;
        }
        
        // Нормализуем ошибку ориентации
        normalizeOrientationError();
        
        // Правильное обновление ковариации: P = (I - K * H) * P
        // 1. Считаем матрицу (I - K * H) размером 9x9
        float I_KH[81];
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                float KH_ij = 0.0f;
                for (int k = 0; k < 6; ++k) {
                    // H - это матрица 6x9, она определена в computeKalmanGain, 
                    // но мы можем восстановить её логику или просто передать. 
                    // Для простоты восстановим H прямо здесь:
                    float H_kj = 0.0f;
                    if (k < 3) { // строки гироскопа
                        if (j == k || j == 3 + k) H_kj = 1.0f;
                    } else { // строки акселерометра
                        if (j == 6 + (k - 3)) H_kj = 1.0f;
                    }
                    KH_ij += K[i * 6 + k] * H_kj;
                }
                I_KH[i * 9 + j] = -KH_ij;
                if (i == j) I_KH[i * 9 + j] += 1.0f; // Добавляем единичную матрицу I
            }
        }

        // 2. Умножаем (I - K * H) на текущую P
        float P_new[81] = {0.0f};
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                for (int k = 0; k < 9; ++k) {
                    P_new[i * 9 + j] += I_KH[i * 9 + k] * covariance_[k * 9 + j];
                }
            }
        }

        // 3. Копируем обратно и защищаем диагональ от нуля
        for (int i = 0; i < 81; ++i) {
            covariance_[i] = P_new[i];
        }
        for (int i = 0; i < 9; ++i) {
            if (covariance_[i * 9 + i] < 1e-6f) {
                covariance_[i * 9 + i] = 1e-6f;
            }
        }
    } else {
        std::cerr << "[ImuFilter] Warning: NaN/Inf in Kalman gain, skipping update" << std::endl;
    }
}

/**
 * Основная функция фильтрации
 */
FilteredIMU ImuFilter::filter(const IMUData& raw_imu) {
        for (int i = 0; i < 3; ++i) {
        if (std::isnan(raw_imu.gyroscope[i]) || std::isinf(raw_imu.gyroscope[i]) ||
            std::isnan(raw_imu.accelerometer[i]) || std::isinf(raw_imu.accelerometer[i])) {
            // Возвращаем предыдущее состояние, пропуская тик
            FilteredIMU filtered;
            for(int j=0; j<3; ++j) {
                filtered.gyroscope[j] = 0.0f; // или state_[3+j]
                filtered.accelerometer[j] = 0.0f;
            }
            return filtered;
        }
    }

    if (is_calibrating_) {
        for (int i = 0; i < 3; ++i)
            gyro_calibration_accum_[i] += raw_imu.gyroscope[i];
        calibration_counter_++;

        FilteredIMU filtered;
        for (int i = 0; i < 3; ++i) {
            filtered.gyroscope[i] = raw_imu.gyroscope[i];
            filtered.accelerometer[i] = raw_imu.accelerometer[i];
            filtered.gyro_bias[i] = 0.0f;
            filtered.orientation_error[i] = 0.0f;
        }
        return filtered;
    }

    update(raw_imu);

    FilteredIMU filtered;
    for (int i = 0; i < 3; ++i) {
        filtered.gyroscope[i] = raw_imu.gyroscope[i] - state_[3 + i];
        filtered.accelerometer[i] = raw_imu.accelerometer[i] - state_[6 + i];
    }
    for (int i = 0; i < 3; ++i) {
        filtered.gyro_bias[i] = state_[3 + i];
        filtered.orientation_error[i] = state_[i];
    }
    return filtered;
}

void ImuFilter::initFromRosParams(const std::string& prefix) {}

void ImuFilter::startGyroCalibration() {
    is_calibrating_ = true;
    calibration_counter_ = 0;
    gyro_calibration_accum_.fill(0.0f);
}

void ImuFilter::finishGyroCalibration() {
    if (calibration_counter_ > 0)
        for (int i = 0; i < 3; ++i) {
            float bias = gyro_calibration_accum_[i] / static_cast<float>(calibration_counter_);
            state_[3 + i] = bias;
        }
    is_calibrating_ = false;
}

std::array<float, 3> ImuFilter::getEstimatedGyroBias() const {
    return {{state_[3], state_[4], state_[5]}};
}

bool ImuFilter::isCalibrating() const { return is_calibrating_; }
std::array<float, 81> ImuFilter::getCovarianceMatrix() const { return covariance_; }

} // namespace imu_filter