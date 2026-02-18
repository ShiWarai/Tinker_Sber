
#include "imu.h"
#include "include.h"
#include "parameter.h"
#include "gait_math.h"
u8 fly_ready;
static xyz_f_t reference_v;
static ref_t 	ref;
float reference_vr[3];
float Roll,Pitch,Yaw;    				//?????
static float q0=1,q1,q2,q3;
static float ref_q[4] = {1,0,0,0};
static float norm_acc,norm_q;
static float norm_acc_lpf;
extern u8 fly_ready;

float invSqrt(float x) {
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}
volatile float beta_start=6;
volatile float beta_end=0.03;
volatile float beta_step=0.01;
volatile float beta;
float accConfidenceDecay = 0.52f;
float accConfidence      = 1.0f;
#define HardFilter(O,N)  ((O)*0.9f+(N)*0.1f)
#define accelOneG 9.8
void calculateAccConfidence(float accMag_in)
{
	// G.K. Egan (C) computes confidence in accelerometers when
	// aircraft is being accelerated over and above that due to gravity

	static float accMagP = 1.0f;
  float accMag=accMag_in;
	accMag /= accelOneG;  // HJI Added to convert MPS^2 to G's

	accMagP  = HardFilter(accMagP, accMag );

	accConfidence=1-((accConfidenceDecay * sqrtf(fabs(accMagP - 1.0f))));
  if(accConfidence>1)
		accConfidence=1;
	if(accConfidence<0)
		accConfidence=0;
	accConfidence=1;
}


int madgwick_update_new(float T,float wx, float wy, float wz, float ax, float ay, float az,float *rol,float *pit,float *yaw) 							
{
    static u8 init;
    float recip_norm,norm;
    float s0, s1, s2, s3;
    float dq1, dq2, dq3, dq4;
    float _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q1q1, q1q3, q2q2;
    float beta_use;
		if(!init){init=1;
				beta = beta_start;
		}
		
		mems.hmlOneACC= norm = sqrtf(ax*(ax) + ay*(ay) + az*az)/4096.*9.8;	
		
    if (beta > beta_end)
    {
        if ((beta - beta_step) > beta_end)
            beta -= beta_step;
        else
            beta = beta_end;
    }else beta=beta_end;
					
    dq1 = 0.5f * (-q1 * wx - q2 * wy - q3 * wz);
    dq2 = 0.5f * (q0 * wx + q2 * wz - q3 * wy);
    dq3 = 0.5f * (q0 * wy - q1 * wz + q3 * wx);
    dq4 = 0.5f * (q0 * wz + q1 * wy - q2 * wx);

    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))
    {   		  
				calculateAccConfidence(norm);
				beta_use = beta*accConfidence;

        recip_norm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recip_norm;
        ay *= recip_norm;
        az *= recip_norm;

        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        _2q0q2 = 2.0f * q0 * q2;
        _2q2q3 = 2.0f * q2 * q3;
        q1q1 = q1 * q1;
        q1q3 = q1 * q3;
        q2q2 = q2 * q2;

        s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) + _2q1 * (2.0f * (q0 * q1) + _2q2q3 - ay);
        s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) + _2q0 * (2.0f * (q0 * q1) + _2q2q3 - ay) - 4.0f * q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az);
        s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) + _2q3 * (2.0f * (q0 * q1) + _2q2q3 - ay) - 4.0f * q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az);
        s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) + _2q2 * (2.0f * (q0 * q1) + _2q2q3 - ay);

        recip_norm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        s0 *= recip_norm;
        s1 *= recip_norm;
        s2 *= recip_norm;
        s3 *= recip_norm;

        dq1 -= beta_use * s0;
        dq2 -= beta_use * s1;
        dq3 -= beta_use * s2;
        dq4 -= beta_use * s3;
    }
		
    q0 += dq1 * T;
    q1 += dq2 * T;
    q2 += dq3 * T;
    q3 += dq4 * T;

    recip_norm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recip_norm;
    q1 *= recip_norm;
    q2 *= recip_norm;
    q3 *= recip_norm;

	ref_q[0]=q0;
	ref_q[1]=q1;
	ref_q[2]=q2;
	ref_q[3]=q3;
	reference_vr[0] = 2*(ref_q[1]*ref_q[3] - ref_q[0]*ref_q[2]);
	reference_vr[1] = 2*(ref_q[0]*ref_q[1] + ref_q[2]*ref_q[3]);
	reference_vr[2] = 1 - 2*(ref_q[1]*ref_q[1] + ref_q[2]*ref_q[2]);
	*rol = fast_atan2(2*(ref_q[0]*ref_q[1] + ref_q[2]*ref_q[3]),1 - 2*(ref_q[1]*ref_q[1] + ref_q[2]*ref_q[2])) *57.3f;
	*pit = asin(2*(ref_q[1]*ref_q[3] - ref_q[0]*ref_q[2])) *57.3f;
	*yaw = fast_atan2(2*(-ref_q[1]*ref_q[2] - ref_q[0]*ref_q[3]), 2*(ref_q[0]*ref_q[0] + ref_q[1]*ref_q[1]) - 1) *57.3f;
    return 1;
}

float Kp =0.36;
float Ki =0.005f;
#define IMU_INTEGRAL_LIM  ( 2.0f *ANGLE_TO_RADIAN )
#define NORM_ACC_LPF_HZ 10  		//(Hz)
#define REF_ERR_LPF_HZ  1				//(Hz)
static xyz_f_t acc_3d_hg,acc_ng,acc_ng_offset;
void IMUupdate(float half_T,float gx, float gy, float gz, float ax, float ay, float az,float *rol,float *pit,float *yaw) 
{		
	float ref_err_lpf_hz;
	static u8 init;
	if(init==0)
	{
	init=1;
	ref.err.x=ref.err.y=ref.err.z=0;
	ref.err_Int.x=ref.err_Int.y=ref.err_Int.z=0;
	ref.err_lpf.x=ref.err_lpf.y=ref.err_lpf.z=0;
	ref.err_tmp.x=ref.err_tmp.y=ref.err_tmp.z=0;
	ref.g.x=ref.g.y=ref.g.z=0;
	}

	reference_vr[0]=reference_v.x = 2*(ref_q[1]*ref_q[3] - ref_q[0]*ref_q[2]);
	reference_vr[1]=reference_v.y = 2*(ref_q[0]*ref_q[1] + ref_q[2]*ref_q[3]);
	reference_vr[2]=reference_v.z = 1 - 2*(ref_q[1]*ref_q[1] + ref_q[2]*ref_q[2]);

	acc_ng.x = 10 *TO_M_S2 *(ax - 4096*reference_v.x) - acc_ng_offset.x;
	acc_ng.y = 10 *TO_M_S2 *(ay - 4096*reference_v.y) - acc_ng_offset.y;
	acc_ng.z = 10 *TO_M_S2 *(az - 4096*reference_v.z) - acc_ng_offset.z;
	
	acc_3d_hg.z = acc_ng.x *reference_v.x + acc_ng.y *reference_v.y + acc_ng.z *reference_v.z;

	norm_acc = sqrtf(ax*ax + ay*ay + az*az);   
	norm_acc_lpf +=  NORM_ACC_LPF_HZ *(6.28f *half_T) *(norm_acc - norm_acc_lpf);
	mems.hmlOneACC= norm_acc_lpf/4096.*9.8;	
	calculateAccConfidence(mems.hmlOneACC);
  if(norm_acc==0)norm_acc=0.0001;
	if(ABS(ax)<4400 && ABS(ay)<4400 && ABS(az)<4400 )
	{	
		ax = ax / norm_acc;
		ay = ay / norm_acc;
		az = az / norm_acc; 
		
		if( 3800 < norm_acc && norm_acc < 4400 )
		{
			ref.err_tmp.x = ay*reference_v.z - az*reference_v.y;
			ref.err_tmp.y = az*reference_v.x - ax*reference_v.z;
			ref_err_lpf_hz = REF_ERR_LPF_HZ *(6.28f *half_T);
			ref.err_lpf.x += ref_err_lpf_hz *( ref.err_tmp.x  - ref.err_lpf.x );
			ref.err_lpf.y += ref_err_lpf_hz *( ref.err_tmp.y  - ref.err_lpf.y );
			ref.err.x = ref.err_lpf.x;
			ref.err.y = ref.err_lpf.y;
		}
	}
	else
	{
		ref.err.x = 0; 
		ref.err.y = 0;
	}
	ref.err_Int.x += ref.err.x *Ki *2 *half_T ;
	ref.err_Int.y += ref.err.y *Ki *2 *half_T ;
	ref.err_Int.z += ref.err.z *Ki *2 *half_T ;
	
	ref.err_Int.x = LIMIT(ref.err_Int.x, - IMU_INTEGRAL_LIM ,IMU_INTEGRAL_LIM );
	ref.err_Int.y = LIMIT(ref.err_Int.y, - IMU_INTEGRAL_LIM ,IMU_INTEGRAL_LIM );
	ref.err_Int.z = LIMIT(ref.err_Int.z, - IMU_INTEGRAL_LIM ,IMU_INTEGRAL_LIM );

	ref.g.x = gx *ANGLE_TO_RADIAN + ( accConfidence*Kp*(ref.err.x + ref.err_Int.x) );
	ref.g.y = gy *ANGLE_TO_RADIAN + ( accConfidence*Kp*(ref.err.y + ref.err_Int.y) );
	ref.g.z = gz *ANGLE_TO_RADIAN;
	
	ref_q[0] = ref_q[0] +(-ref_q[1]*ref.g.x - ref_q[2]*ref.g.y - ref_q[3]*ref.g.z)*half_T;
	ref_q[1] = ref_q[1] + (ref_q[0]*ref.g.x + ref_q[2]*ref.g.z - ref_q[3]*ref.g.y)*half_T;
	ref_q[2] = ref_q[2] + (ref_q[0]*ref.g.y - ref_q[1]*ref.g.z + ref_q[3]*ref.g.x)*half_T;
	ref_q[3] = ref_q[3] + (ref_q[0]*ref.g.z + ref_q[1]*ref.g.y - ref_q[2]*ref.g.x)*half_T;  

	norm_q = sqrtf(ref_q[0]*ref_q[0] + ref_q[1]*ref_q[1] + ref_q[2]*ref_q[2] + ref_q[3]*ref_q[3]);
	if(norm_q==0)norm_q=1;
	ref_q[0] = ref_q[0] / norm_q;
	ref_q[1] = ref_q[1] / norm_q;
	ref_q[2] = ref_q[2] / norm_q;
	ref_q[3] = ref_q[3] / norm_q;
	
	*rol = fast_atan2(2*(ref_q[0]*ref_q[1] + ref_q[2]*ref_q[3]),1 - 2*(ref_q[1]*ref_q[1] + ref_q[2]*ref_q[2])) *57.3f;
	*pit = asinf(2*(ref_q[1]*ref_q[3] - ref_q[0]*ref_q[2])) *57.3f;
	*yaw = fast_atan2(2*(-ref_q[1]*ref_q[2] - ref_q[0]*ref_q[3]), 2*(ref_q[0]*ref_q[0] + ref_q[1]*ref_q[1]) - 1) *57.3f;
}