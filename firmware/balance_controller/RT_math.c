#include "include.h"
#include "gait_math.h"

void mat_trans(float src[3][3],float dis[3][3])
{
	char i,j;
	for(i = 0;i < 3;i++){
		for(j = 0; j < 3 ; j++){
			dis[i][j] = src[j][i];
		}
	}
}
