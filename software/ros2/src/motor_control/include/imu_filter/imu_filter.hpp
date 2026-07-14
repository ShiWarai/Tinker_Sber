#ifndef IMU_FILTER_IMU_FILTER_HPP
#define IMU_FILTER_IMU_FILTER_HPP

#include <array>

namespace imu_filter {

struct ImuSample {
    std::array<float, 3> gyro;   // rad/s
    std::array<float, 3> accel;  // any linear units (tilt uses unit vector)
};

struct ImuFilterParams {
    float kp = 2.0f;                 // Mahony proportional gain (roll/pitch)
    float ki = 0.01f;                // Mahony integral gain (roll/pitch bias)
    float ki_limit = 0.5f;           // rad/s, clamp on bias
    float accel_reject_rel = 0.25f;  // |||a||/g - 1| → skip accel correction
    int startup_bias_samples = 2000; // initial gyro bias average @ 1 kHz

    // Yaw (Z) has no gravity observability — learn bias_z only when still.
    float static_gyro_xy_max = 0.08f;   // rad/s, |ωx|,|ωy| after bias
    float static_gyro_z_max = 0.08f;    // rad/s, |ωz| after bias (not turning)
    float yaw_bias_alpha = 0.999f;      // LPF toward gyro_z while static
    float yaw_deadzone = 0.01f;         // rad/s ≈ 0.6°/s
    int static_hold_samples = 200;      // 200 ms @ 1 kHz before adapting Z
};

/**
 * 6-DOF Mahony AHRS (gyro + accel).
 * Roll/pitch observable from gravity; yaw free-integrates (no magnetometer).
 */
class ImuFilter {
public:
    ImuFilter();
    explicit ImuFilter(const ImuFilterParams& params);

    void reset();
    void setParams(const ImuFilterParams& params);
    const ImuFilterParams& params() const { return params_; }

    void update(const ImuSample& sample, float dt,
                std::array<float, 4>& quat_out,
                std::array<float, 3>& rpy_out);

    bool isCalibrationDone() const { return startup_done_; }
    bool isInitialized() const { return initialized_; }
    const std::array<float, 4>& quaternion() const { return q_; }
    const std::array<float, 3>& gyroBias() const { return bias_; }
    float gravityNorm() const { return gravity_; }

    static std::array<float, 4> eulerToQuat(float roll, float pitch, float yaw);
    static std::array<float, 3> quatToEuler(const std::array<float, 4>& q);

private:
    static float norm3(const std::array<float, 3>& v);
    static std::array<float, 3> cross(const std::array<float, 3>& a,
                                      const std::array<float, 3>& b);
    static std::array<float, 4> quatMul(const std::array<float, 4>& a,
                                        const std::array<float, 4>& b);
    static std::array<float, 4> quatNormalize(const std::array<float, 4>& q);
    static std::array<float, 4> quatCanonicalize(const std::array<float, 4>& q);

    void initFromAccel(const std::array<float, 3>& accel);
    void integrateGyro(const std::array<float, 3>& omega, float dt);

    ImuFilterParams params_;
    std::array<float, 4> q_{{1.0f, 0.0f, 0.0f, 0.0f}};
    std::array<float, 3> bias_{{0.0f, 0.0f, 0.0f}};
    std::array<float, 3> bias_accum_{{0.0f, 0.0f, 0.0f}};
    float gravity_ = 1.0f;
    float gravity_accum_ = 0.0f;
    int startup_count_ = 0;
    int static_hold_count_ = 0;
    bool startup_done_ = false;
    bool initialized_ = false;
};

}  // namespace imu_filter

#endif /* IMU_FILTER_IMU_FILTER_HPP */
