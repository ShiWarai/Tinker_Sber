#include "math/gait_math.h"

void mat_trans(float src[3][3],float dis[3][3])
{
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			dis[i][j] = src[j][i];
		}
	}
}

void update_rotation_matrices(float pitch, float roll, float yaw, float Rn_b[3][3], float Rb_n[3][3]) {
    // Standard ZYX Euler rotation matrix (World to Body: Rn_b)
    // Rn_b = Rz(yaw) * Ry(pitch) * Rx(roll)
    // Using degrees in sind/cosd
    
    float cp = cosd(pitch);
    float sp = sind(pitch);
    float cr = cosd(roll);
    float sr = sind(roll);
    float cy = cosd(yaw);
    float sy = sind(yaw);

    Rn_b[0][0] = cp * cy;
    Rn_b[0][1] = cp * sy;
    Rn_b[0][2] = -sp;

    Rn_b[1][0] = sr * sp * cy - cr * sy;
    Rn_b[1][1] = sr * sp * sy + cr * cy;
    Rn_b[1][2] = sr * cp;

    Rn_b[2][0] = cr * sp * cy + sr * sy;
    Rn_b[2][1] = cr * sp * sy - sr * cy;
    Rn_b[2][2] = cr * cp;

    // Body to World is just the transpose for orthogonal rotation matrices
    mat_trans(Rn_b, Rb_n);
}
