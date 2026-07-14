#include "imu_filter/imu_filter.hpp"

#include <algorithm>
#include <cmath>

namespace imu_filter {

ImuFilter::ImuFilter()
    : ImuFilter(ImuFilterParams{})
{
}

ImuFilter::ImuFilter(const ImuFilterParams& params)
    : params_(params)
{
    reset();
}

void ImuFilter::reset()
{
    q_ = {{1.0f, 0.0f, 0.0f, 0.0f}};
    bias_ = {{0.0f, 0.0f, 0.0f}};
    bias_accum_ = {{0.0f, 0.0f, 0.0f}};
    gravity_ = 1.0f;
    gravity_accum_ = 0.0f;
    startup_count_ = 0;
    static_hold_count_ = 0;
    startup_done_ = false;
    initialized_ = false;
}

void ImuFilter::setParams(const ImuFilterParams& params)
{
    params_ = params;
}

float ImuFilter::norm3(const std::array<float, 3>& v)
{
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

std::array<float, 3> ImuFilter::cross(const std::array<float, 3>& a,
                                      const std::array<float, 3>& b)
{
    return {{
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    }};
}

std::array<float, 4> ImuFilter::quatMul(const std::array<float, 4>& a,
                                        const std::array<float, 4>& b)
{
    return {{
        a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
        a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
        a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
        a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]
    }};
}

std::array<float, 4> ImuFilter::quatNormalize(const std::array<float, 4>& q)
{
    const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (n < 1e-8f) {
        return {{1.0f, 0.0f, 0.0f, 0.0f}};
    }
    return {{q[0] / n, q[1] / n, q[2] / n, q[3] / n}};
}

std::array<float, 4> ImuFilter::quatCanonicalize(const std::array<float, 4>& q)
{
    return (q[0] < 0.0f) ? std::array<float, 4>{{-q[0], -q[1], -q[2], -q[3]}} : q;
}

std::array<float, 3> ImuFilter::quatToEuler(const std::array<float, 4>& q)
{
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    const float roll = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
    const float sinp = 2.0f * (w * y - z * x);
    const float pitch = std::asin(std::clamp(sinp, -1.0f, 1.0f));
    const float yaw = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
    return {{roll, pitch, yaw}};
}

std::array<float, 4> ImuFilter::eulerToQuat(float roll, float pitch, float yaw)
{
    const float cr = std::cos(roll * 0.5f), sr = std::sin(roll * 0.5f);
    const float cp = std::cos(pitch * 0.5f), sp = std::sin(pitch * 0.5f);
    const float cy = std::cos(yaw * 0.5f), sy = std::sin(yaw * 0.5f);
    return quatCanonicalize({{
        cr * cp * cy + sr * sp * sy,
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy
    }});
}

void ImuFilter::initFromAccel(const std::array<float, 3>& accel)
{
    if (norm3(accel) < 1e-6f) {
        return;
    }
    const float roll = std::atan2(accel[1], accel[2]);
    const float pitch =
        std::atan2(-accel[0], std::sqrt(accel[1] * accel[1] + accel[2] * accel[2]));
    q_ = eulerToQuat(roll, pitch, 0.0f);
    initialized_ = true;
}

void ImuFilter::integrateGyro(const std::array<float, 3>& omega, float dt)
{
    const float wx = omega[0], wy = omega[1], wz = omega[2];
    const std::array<float, 4> dq{{
        0.5f * (-q_[1] * wx - q_[2] * wy - q_[3] * wz),
        0.5f * ( q_[0] * wx + q_[2] * wz - q_[3] * wy),
        0.5f * ( q_[0] * wy - q_[1] * wz + q_[3] * wx),
        0.5f * ( q_[0] * wz + q_[1] * wy - q_[2] * wx)
    }};
    q_ = quatCanonicalize(quatNormalize({{
        q_[0] + dq[0] * dt,
        q_[1] + dq[1] * dt,
        q_[2] + dq[2] * dt,
        q_[3] + dq[3] * dt
    }}));
}

void ImuFilter::update(const ImuSample& sample, float dt,
                       std::array<float, 4>& quat_out,
                       std::array<float, 3>& rpy_out)
{
    if (dt <= 0.0f) {
        quat_out = q_;
        rpy_out = quatToEuler(q_);
        return;
    }

    const float an = norm3(sample.accel);

    if (!startup_done_) {
        const float inv = 1.0f / static_cast<float>(params_.startup_bias_samples);
        bias_accum_[0] += sample.gyro[0] * inv;
        bias_accum_[1] += sample.gyro[1] * inv;
        bias_accum_[2] += sample.gyro[2] * inv;
        gravity_accum_ += an * inv;
        ++startup_count_;

        if (startup_count_ >= params_.startup_bias_samples) {
            bias_ = bias_accum_;
            if (gravity_accum_ > 1e-3f) {
                gravity_ = gravity_accum_;
            }
            startup_done_ = true;
            initFromAccel(sample.accel);
        }

        quat_out = q_;
        rpy_out = quatToEuler(q_);
        return;
    }

    if (!initialized_) {
        initFromAccel(sample.accel);
        quat_out = q_;
        rpy_out = quatToEuler(q_);
        return;
    }

    std::array<float, 3> omega{{
        sample.gyro[0] - bias_[0],
        sample.gyro[1] - bias_[1],
        sample.gyro[2] - bias_[2]
    }};

    const float g = (gravity_ > 1e-3f) ? gravity_ : 1.0f;
    const bool accel_ok =
        (an > 1e-6f) && (std::abs(an / g - 1.0f) < params_.accel_reject_rel);

    if (accel_ok) {
        const std::array<float, 3> a_meas{{
            sample.accel[0] / an,
            sample.accel[1] / an,
            sample.accel[2] / an
        }};

        // Gravity direction in body: rotate world [0,0,-1] by q* (same as MuJoCo).
        const std::array<float, 4> q_conj{{q_[0], -q_[1], -q_[2], -q_[3]}};
        const std::array<float, 4> g_quat{{0.0f, 0.0f, 0.0f, -1.0f}};
        const auto tmp = quatMul(quatMul(q_conj, g_quat), q_);
        const std::array<float, 3> v_est{{tmp[1], tmp[2], tmp[3]}};

        const auto e = cross(v_est, a_meas);

        // Mahony bias from gravity — only roll/pitch (X/Y). Z is unobservable.
        bias_[0] -= params_.ki * e[0] * dt;
        bias_[1] -= params_.ki * e[1] * dt;
        const float lim = params_.ki_limit;
        bias_[0] = std::clamp(bias_[0], -lim, lim);
        bias_[1] = std::clamp(bias_[1], -lim, lim);

        omega[0] = sample.gyro[0] - bias_[0] + params_.kp * e[0];
        omega[1] = sample.gyro[1] - bias_[1] + params_.kp * e[1];
        omega[2] = sample.gyro[2] - bias_[2];
    }

    // Learn bias_z only when clearly not rotating (accel ok + small ωx/ωy/ωz).
    // Pure yaw turns have large |ωz| and will not update bias.
    const bool apparently_still =
        accel_ok &&
        (std::abs(omega[0]) < params_.static_gyro_xy_max) &&
        (std::abs(omega[1]) < params_.static_gyro_xy_max) &&
        (std::abs(omega[2]) < params_.static_gyro_z_max);

    if (apparently_still) {
        if (static_hold_count_ < params_.static_hold_samples) {
            ++static_hold_count_;
        } else {
            const float a = params_.yaw_bias_alpha;
            bias_[2] = a * bias_[2] + (1.0f - a) * sample.gyro[2];
            bias_[2] = std::clamp(bias_[2], -params_.ki_limit, params_.ki_limit);
            omega[2] = sample.gyro[2] - bias_[2];
        }
    } else {
        static_hold_count_ = 0;
    }

    if (std::abs(omega[2]) < params_.yaw_deadzone) {
        omega[2] = 0.0f;
    }

    integrateGyro(omega, dt);

    quat_out = q_;
    rpy_out = quatToEuler(q_);
}

}  // namespace imu_filter
