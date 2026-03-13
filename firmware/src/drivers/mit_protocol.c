#include "drivers/mit_protocol.h"

const mit_limits_t MIT_LIMITS_DM_DEFAULT = {
  .p_min = -12.5f,
  .p_max = 12.5f,
  .v_min = -38.2f,
  .v_max = 38.2f,
  .kp_min = 0.0f,
  .kp_max = 500.0f,
  .kd_min = 0.0f,
  .kd_max = 5.0f,
  .t_min = -12.0f,
  .t_max = 12.0f,
};

static float clampf(float value, float min, float max)
{
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

uint16_t mit_float_to_uint(float value, float min, float max, uint8_t bits)
{
  const float span = max - min;
  const float offset = min;
  const uint32_t max_code = ((uint32_t)1U << bits) - 1U;

  if ((bits == 0U) || (span <= 0.0f)) {
    return 0U;
  }

  value = clampf(value, min, max);
  return (uint16_t)((value - offset) * ((float)max_code) / span);
}

float mit_uint_to_float(uint32_t value, float min, float max, uint8_t bits)
{
  const float span = max - min;
  const float offset = min;
  const uint32_t max_code = ((uint32_t)1U << bits) - 1U;

  if ((bits == 0U) || (span <= 0.0f) || (max_code == 0U)) {
    return min;
  }

  if (value > max_code) {
    value = max_code;
  }

  return ((float)value) * span / ((float)max_code) + offset;
}

void mit_pack_command(const mit_command_t *cmd,
                      const mit_limits_t *limits,
                      uint8_t out[8])
{
  uint16_t p_int;
  uint16_t v_int;
  uint16_t kp_int;
  uint16_t kd_int;
  uint16_t t_int;

  if ((cmd == 0) || (limits == 0) || (out == 0)) {
    return;
  }

  p_int = mit_float_to_uint(cmd->p, limits->p_min, limits->p_max, 16);
  v_int = mit_float_to_uint(cmd->v, limits->v_min, limits->v_max, 12);
  kp_int = mit_float_to_uint(cmd->kp, limits->kp_min, limits->kp_max, 12);
  kd_int = mit_float_to_uint(cmd->kd, limits->kd_min, limits->kd_max, 12);
  t_int = mit_float_to_uint(cmd->t, limits->t_min, limits->t_max, 12);

  out[0] = (uint8_t)(p_int >> 8);
  out[1] = (uint8_t)(p_int & 0xFFU);
  out[2] = (uint8_t)(v_int >> 4);
  out[3] = (uint8_t)(((v_int & 0x0FU) << 4) | (kp_int >> 8));
  out[4] = (uint8_t)(kp_int & 0xFFU);
  out[5] = (uint8_t)(kd_int >> 4);
  out[6] = (uint8_t)(((kd_int & 0x0FU) << 4) | (t_int >> 8));
  out[7] = (uint8_t)(t_int & 0xFFU);
}

void mit_unpack_feedback_legacy(const uint8_t in[8],
                                const mit_limits_t *limits,
                                mit_feedback_t *feedback)
{
  uint16_t p_int;
  uint16_t v_int;
  uint16_t t_int;

  if ((in == 0) || (limits == 0) || (feedback == 0)) {
    return;
  }

  p_int = ((uint16_t)in[1] << 8) | in[2];
  v_int = ((uint16_t)in[3] << 4) | (in[4] >> 4);
  t_int = ((uint16_t)(in[4] & 0x0FU) << 8) | in[5];

  feedback->motor_id = in[0];
  feedback->p = mit_uint_to_float(p_int, limits->p_min, limits->p_max, 16);
  feedback->v = mit_uint_to_float(v_int, limits->v_min, limits->v_max, 12);
  feedback->t = mit_uint_to_float(t_int, limits->t_min, limits->t_max, 12);
}