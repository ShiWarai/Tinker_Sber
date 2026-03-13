#ifndef MIT_PROTOCOL_H
#define MIT_PROTOCOL_H

#include <stdint.h>

typedef struct {
  float p_min;
  float p_max;
  float v_min;
  float v_max;
  float kp_min;
  float kp_max;
  float kd_min;
  float kd_max;
  float t_min;
  float t_max;
} mit_limits_t;

typedef struct {
  float p;
  float v;
  float kp;
  float kd;
  float t;
} mit_command_t;

typedef struct {
  uint8_t motor_id;
  float p;
  float v;
  float t;
} mit_feedback_t;

extern const mit_limits_t MIT_LIMITS_DM_DEFAULT;

uint16_t mit_float_to_uint(float value, float min, float max, uint8_t bits);
float mit_uint_to_float(uint32_t value, float min, float max, uint8_t bits);

void mit_pack_command(const mit_command_t *cmd,
                      const mit_limits_t *limits,
                      uint8_t out[8]);

void mit_unpack_feedback_legacy(const uint8_t in[8],
                                const mit_limits_t *limits,
                                mit_feedback_t *feedback);

#endif /* MIT_PROTOCOL_H */