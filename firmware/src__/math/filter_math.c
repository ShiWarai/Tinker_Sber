#include "math/gait_math.h"

void DigitalLPF(float in, float* out, float cutoff_freq, float dt) {
    if (cutoff_freq <= 0.0f || dt <= 0.0f) {
        *out = in;
        return;
    }
    
    float lpf_cof;
    if (cutoff_freq < 1.0f)
        lpf_cof = LPF_COF_05Hz;
    else if (cutoff_freq < 5.0f)
        lpf_cof = LPF_COF_1t5Hz;
    else if (cutoff_freq < 10.0f)
        lpf_cof = LPF_COF_5t10Hz;
    else if (cutoff_freq < 15.0f)
        lpf_cof = LPF_COF_10t15Hz;
    else if (cutoff_freq < 20.0f)
        lpf_cof = LPF_COF_15t20Hz;
    else if (cutoff_freq < 25.0f)
        lpf_cof = LPF_COF_20t25Hz;
    else if (cutoff_freq < 30.0f)
        lpf_cof = LPF_COF_25t30Hz;
    else if (cutoff_freq < 50.0f)
        lpf_cof = LPF_COF_30t50Hz;
    else if (cutoff_freq < 70.0f)
        lpf_cof = LPF_COF_50t70Hz;
    else if (cutoff_freq < 100.0f)
        lpf_cof = LPF_COF_70t100Hz;
    else
        lpf_cof = LPF_COF_100tHz;

    float rc = lpf_cof;
    float alpha = LIMIT(dt / (dt + rc), 0.0f, 1.0f);
    *out += (in - *out) * alpha;
}

void DigitalLPF_Double(float in, double* out, float cutoff_freq, float dt) {
    if (cutoff_freq <= 0.0f || dt <= 0.0f) {
        *out = (double)in;
        return;
    }
    
    float lpf_cof;
    if (cutoff_freq < 1.0f)
        lpf_cof = LPF_COF_05Hz;
    else if (cutoff_freq < 5.0f)
        lpf_cof = LPF_COF_1t5Hz;
    else if (cutoff_freq < 10.0f)
        lpf_cof = LPF_COF_5t10Hz;
    else if (cutoff_freq < 15.0f)
        lpf_cof = LPF_COF_10t15Hz;
    else if (cutoff_freq < 20.0f)
        lpf_cof = LPF_COF_15t20Hz;
    else if (cutoff_freq < 25.0f)
        lpf_cof = LPF_COF_20t25Hz;
    else if (cutoff_freq < 30.0f)
        lpf_cof = LPF_COF_25t30Hz;
    else if (cutoff_freq < 50.0f)
        lpf_cof = LPF_COF_30t50Hz;
    else if (cutoff_freq < 70.0f)
        lpf_cof = LPF_COF_50t70Hz;
    else if (cutoff_freq < 100.0f)
        lpf_cof = LPF_COF_70t100Hz;
    else
        lpf_cof = LPF_COF_100tHz;

    float rc = lpf_cof;
    float alpha = LIMIT(dt / (dt + rc), 0.0f, 1.0f);
    *out += (double)((in - (float)(*out)) * alpha);
}
