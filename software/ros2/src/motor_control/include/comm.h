 #ifndef __COMM_H_
 #define __COMM_H_
 #include<stdio.h>
 #include<sys/types.h>
 #include<sys/ipc.h>
 #include<sys/shm.h>
 #define PATHNAME "."
 #define PROJ_ID 0X6666
 
 int createshm(int sz);   /* создать сегмент */
 int destroyshm(int shmid); /* уничтожить сегмент */
 int getshm(int sz);      /* получить сегмент */
 #endif /* __COMM_H_ */  