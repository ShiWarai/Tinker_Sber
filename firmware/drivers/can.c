#include "can.h"
#include "ecos_link.h" 
#include "delay.h"
#include "led_fc.h"
#include "include.h"
#include "gait_math.h"
#include "locomotion_header.h"
#include "Custom_SPI_DEVICE.h"

/* ???????? ?????? USB (can_write_flash, can_cmd_usb_disable, ocu_connect, ocu_loss_cnt) */
int can_write_flash = 0;
int can_cmd_usb_disable = 0;
int ocu_connect = 0;
float ocu_loss_cnt = 0;

_LEG_MOTOR leg_motor;
motor_measure_t  motor_chassis[10];
uint32_t can1_rx_id;
int can_rx_over[5],can_rx_cnt[5];
u8 canbuft1[8],canbufr1[8];

#define USE_ID_CHECK1 0
#if CAN_NART_SEL== DISABLE || CAN_FB_SYNC //unuse
int  CAN_SAFE_DELAY=166;//us  ????????????????11
#else
int  CAN_SAFE_DELAY=130;//us  ????????????????11
#endif

u32  slave_id1 = 99 ; 
float cnt_rst1=0;
int can1_rx_cnt;
int can_rx_cnt_all[6]={0};
void CAN_motor_init(void)
{
	char i;
	
	for(i=0;i<10;i++)
	{
		leg_motor.connect=0;
		leg_motor.motor_en=0;
		leg_motor.motor_mode=MOTOR_MODE_T;	//  ????????????
		
		reset_current_cmd(i);
		
		motor_chassis[i].max_t=leg_motor.max_t[i]=120;//Nm ???????
		
		motor_chassis[i].stiff=1.0;
		motor_chassis[i].kp=0.5;
		motor_chassis[i].kd=0.1;

		motor_chassis[i].param.q_reset_angle = 0.0f; //to right set_zero_pos
	}
}

static float fmaxf(float x, float y){
    /// Returns maximum of x, y ///
    return (((x)>(y))?(x):(y));
    }

static float fminf(float x, float y){
    /// Returns minimum of x, y ///
    return (((x)<(y))?(x):(y));
    }

static float Bytes2Float(unsigned char *bytes,int num)
{
    unsigned char cByte[24];
    int i;
    for (i=0;i<num;i++)
    {
		 cByte[num-1-i] = bytes[i];
    }   
    float pfValue=*(float*)&cByte;
    return  pfValue;
}

static void Float2Bytes(float pfValue,unsigned char* bytes)
{
  char* pchar=(char*)&pfValue;
  for(int i=0;i<sizeof(float);i++)
  {
    *bytes=*pchar;
     pchar++;
     bytes++;  
  }
}

static int float_to_uint(float x, float x_min, float x_max, int bits){
    /// Converts a float to an unsigned int, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    return (int) ((x-offset)*((float)((1<<bits)-1))/span);
    }

static float uint_to_float(int x_int, float x_min, float x_max, int bits){
    /// converts unsigned int to float, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int)*span/((float)((1<<bits)-1)) + offset;
}
		
u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,float brp,u8 mode)
{

  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN1_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //?????????
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//???PORTA???	                   											 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//???CAN1???	
	  
    //?????GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8| GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//???�???
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//???????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//????
    GPIO_Init(GPIOB, &GPIO_InitStructure);//?????PA11,PA12
	
	  //??????????????
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_CAN1); //GPIOA11?????CAN1
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_CAN1); //GPIOA12?????CAN1
	  
  	//CAN???????
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//????????????   
		#if !CAN_ABOM_E
		CAN_InitStructure.CAN_ABOM=DISABLE;	//??????????????	  
		#else
  	CAN_InitStructure.CAN_ABOM=ENABLE;	//??????????????	  
		#endif
  	CAN_InitStructure.CAN_AWUM=ENABLE;//????????????????(???CAN->MCR??SLEEP?)
  	CAN_InitStructure.CAN_NART=CAN_NART_SEL;//DISABLE;	//?????????????? 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//?????????,?�??????  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//?????????????????? 
  	CAN_InitStructure.CAN_Mode= mode;	 //?????? 
  	CAN_InitStructure.CAN_SJW=tsjw;	//??????????????(Tsjw)?tsjw+1?????? CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1???CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2???CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //??????(Fdiv)?brp+1	
  	CAN_Init(CAN1, &CAN_InitStructure);   // ?????CAN1 
    
	//???�?????
 	  CAN_FilterInitStructure.CAN_FilterNumber=0;	  //??????0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32? 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32?ID
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32?MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//??????0??????FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //?????????0
  	CAN_FilterInit(&CAN_FilterInitStructure);//??????????
		
#if CAN1_RX0_INT_ENABLE
	  CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0?????????????.		    
  	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // ????????1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            // ????????0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
 	#if CAN_ABOM_E
		CAN_ITConfig(CAN1,CAN_IT_ERR,DISABLE);
	#endif
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);//??????????
	CAN_ClearITPendingBit(CAN1, CAN_IT_TME);//?????????  
	return 0;
}   

char can_rx_t1[10];
void CAN1_RX0_IRQHandler(void)
{
		int i;
  	CanRxMsg RxMessage;
    CAN_Receive(CAN1, 0, &RxMessage);
	
		can_rx_t1[0]=RxMessage.StdId;
	
		can_rx_t1[1]=RxMessage.Data[0];
		can_rx_t1[2]=RxMessage.Data[1];
		can_rx_t1[3]=RxMessage.Data[2];
		can_rx_t1[4]=RxMessage.Data[3];
		can_rx_t1[5]=RxMessage.Data[4];
		can_rx_t1[6]=RxMessage.Data[5];
		can_rx_t1[7]=RxMessage.Data[6];
		can_rx_t1[8]=RxMessage.Data[7];
	 
		// Frame DMG
		// CMD    | nodeID
		// 7 bits | 4 bits
		uint32_t frameID = RxMessage.StdId;
		uint32_t cmd = (frameID >> 4);
		uint32_t nodeID = (frameID & 0xF)-1;
		
		//----------------------------MIT  ???CAN1 ID??1???  ???CAN1???????  ??????????? ???
		if ((motor_chassis[0].motor.type<EC_1)&&
		(RxMessage.IDE == CAN_Id_Standard) //??????
		&& (RxMessage.IDE == CAN_RTR_Data) //???????
		&& ((RxMessage.DLC == 6||RxMessage.DLC == 8))) /* ????????8 */
		{
			if(RxMessage.Data[0]==0+1||RxMessage.Data[0]==0+1+0x10){
				data_can_mit_anal(&motor_chassis[0],RxMessage.Data);
				leg_motor.q_now[0]=motor_chassis[0].q_now_flt;
				leg_motor.qd_now[0]=motor_chassis[0].qd_now_flt;
				leg_motor.t_now[0]=motor_chassis[0].t_now_flt;
			}
			else if(RxMessage.Data[0]==1+1||RxMessage.Data[0]==1+1+0x10){
				data_can_mit_anal(&motor_chassis[1],RxMessage.Data);
				leg_motor.q_now[1]=motor_chassis[1].q_now_flt;
				leg_motor.qd_now[1]=motor_chassis[1].qd_now_flt;
				leg_motor.t_now[1]=motor_chassis[1].t_now_flt;
			}
			else if(RxMessage.Data[0]==2+1||RxMessage.Data[0]==2+1+0x10){
				data_can_mit_anal(&motor_chassis[2],RxMessage.Data);
				leg_motor.q_now[2]=motor_chassis[2].q_now_flt;
				leg_motor.qd_now[2]=motor_chassis[2].qd_now_flt;
				leg_motor.t_now[2]=motor_chassis[2].t_now_flt;
			}
			else if(RxMessage.Data[0]==3+1||RxMessage.Data[0]==3+1+0x10){
				data_can_mit_anal(&motor_chassis[3],RxMessage.Data);
				leg_motor.q_now[3]=motor_chassis[3].q_now_flt;
				leg_motor.qd_now[3]=motor_chassis[3].qd_now_flt;
				leg_motor.t_now[3]=motor_chassis[3].t_now_flt;
			}
			else if(RxMessage.Data[0]==4+1||RxMessage.Data[0]==4+1+0x10){
				data_can_mit_anal(&motor_chassis[4],RxMessage.Data);
				leg_motor.q_now[4]=motor_chassis[4].q_now_flt;
				leg_motor.qd_now[4]=motor_chassis[4].qd_now_flt;
				leg_motor.t_now[4]=motor_chassis[4].t_now_flt;
			}
		  else if(RxMessage.Data[0]==5+1||RxMessage.Data[0]==5+1+0x10){
				data_can_mit_anal(&motor_chassis[5],RxMessage.Data);
				leg_motor.q_now[5]=motor_chassis[5].q_now_flt;
				leg_motor.qd_now[5]=motor_chassis[5].qd_now_flt;
				leg_motor.t_now[5]=motor_chassis[5].t_now_flt;
			}
		 else if(RxMessage.Data[0]==6+1||RxMessage.Data[0]==6+1+0x10){
				data_can_mit_anal(&motor_chassis[6],RxMessage.Data);
				leg_motor.q_now[6]=motor_chassis[6].q_now_flt;
				leg_motor.qd_now[6]=motor_chassis[6].qd_now_flt;
				leg_motor.t_now[6]=motor_chassis[6].t_now_flt;
			}
		}	
		
				//----------------------------RV??? VESC????
		if (
		(RxMessage.IDE == CAN_Id_Standard) //??????
		&& (RxMessage.IDE == CAN_RTR_Data) //???????
		&& ((RxMessage.DLC >=6)) &&
			motor_chassis[0].motor.type>=EC_1) /* ????????8 */
		{
			RV_can_data_repack(&RxMessage,0,0);
			for(i=0;i<10;i++)
			{
				leg_motor.q_now[i]=motor_chassis[i].q_now_flt;
				leg_motor.qd_now[i]=motor_chassis[i].qd_now_flt;
				leg_motor.t_now[i]=motor_chassis[i].t_now_flt;
			}
		}			
		
	  can1_rx_cnt++;
}
 

u8 CAN1_Send_Msg(u8* msg,u8 len,uint32_t id)
{	
	static char cnt_tx;
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;//0x12;	 // ?????????0
  TxMessage.ExtId=0x00;//0x12;	 // ??????????????29???
  TxMessage.IDE=0;		  // ???????????
  TxMessage.RTR=0;		  // ?????????????????8?
  TxMessage.DLC=len;							 // ??????????
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ???????          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//??????????
  if(i>=0XFFF)
			return 1;
  return 0;		//good

}

u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
	static int cnt_rx,led_flag;
	char can_node_id=0;
	char temp;
	int id=0;
	CAN_Receive(CAN1, 0, &RxMessage);
	cnt_rst1=0;
	
	if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//�??????????,?????? 
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//???????	
	for(i=0;i<RxMessage.DLC;i++)
	buf[i]=RxMessage.Data[i]; 
	can1_rx_id=RxMessage.StdId;	
	can1_rx_cnt++;
	return RxMessage.DLC;	
}


uint32_t can2_rx_id;
u8 canbuft2[8];
u8 canbufr2[8];
#define USE_ID_CHECK2 0
u32  slave_id2 = 99 ; 
float cnt_rst2;
int can2_rx_cnt;
u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,float brp,u8 mode)
{
  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN2_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //?????????
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//???PORTA???	                   											 
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);//???CAN1???	
	
    //?????GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//???�???
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//???????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//????
    GPIO_Init(GPIOB, &GPIO_InitStructure);//?????PA11,PA12
	
	  //??????????????
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_CAN2); //GPIOA11?????CAN1
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_CAN2); //GPIOA12?????CAN1
	  
  	//CAN???????
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//????????????   
		#if !CAN_ABOM_E
		CAN_InitStructure.CAN_ABOM=DISABLE;	//??????????????	  
		#else
  	CAN_InitStructure.CAN_ABOM=ENABLE;	//??????????????	  
		#endif
  	CAN_InitStructure.CAN_AWUM=ENABLE;//????????????????(???CAN->MCR??SLEEP?)
  	CAN_InitStructure.CAN_NART=CAN_NART_SEL;//DISABLE	//?????????????? 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//?????????,?�??????  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//?????????????????? 
  	CAN_InitStructure.CAN_Mode= mode;	 //?????? 
  	CAN_InitStructure.CAN_SJW=tsjw;	//??????????????(Tsjw)?tsjw+1?????? CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1???CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2???CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //??????(Fdiv)?brp+1	
  	CAN_Init(CAN2, &CAN_InitStructure);   // ?????CAN1 
    
	//???�?????
 	  CAN_FilterInitStructure.CAN_FilterNumber=14;	  //??????0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32? 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32?ID
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32?MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//??????0??????FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //?????????0
  	CAN_FilterInit(&CAN_FilterInitStructure);//??????????
		
#if CAN2_RX0_INT_ENABLE
	  CAN_ITConfig(CAN2,CAN_IT_FMP0,ENABLE);//FIFO0?????????????.		    
  	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // ????????1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            // ????????0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif

	#if CAN_ABOM_E
		CAN_ITConfig(CAN2,CAN_IT_ERR,DISABLE);
	#endif
	return 0;
}   
 
#if CAN2_RX0_INT_ENABLE	//???RX0???
//????????			    
void CAN2_RX0_IRQHandler(void)
{
	int ge,shi,bai;
	static int cnt_rx,led_flag,id=0;
	CanRxMsg RxMessage;
	int i=0;
	u8 temp;
	CAN_Receive(CAN2, 0, &RxMessage);
	cnt_rst2=0;
//	if(cnt_rx++>500&&1){cnt_rx=0;
//	led_flag=!led_flag;
//	//LEDRGB_BLUE(led_flag);
//	}
//	for(i=0;i<8;i++)
//		canbufr2[i]=0x00;
//	for(i=0;i<RxMessage.DLC;i++)
//		canbufr2[i]=RxMessage.Data[i];
//	can2_rx_id=RxMessage.StdId;	
// 
	//----------------------------MIT???CAN2 ID??1???  ???CAN2???????  ??????????? ???
	if ((motor_chassis[0].motor.type<EC_1)&& 
	(RxMessage.IDE == CAN_Id_Standard) //??????
	&& (RxMessage.IDE == CAN_RTR_Data) //???????
	&& ((RxMessage.DLC == 6||RxMessage.DLC == 8))) /* ????????8 */
	{
			if(RxMessage.Data[0]==0+1||RxMessage.Data[0]==0+1+0x10){
				data_can_mit_anal(&motor_chassis[5+0],RxMessage.Data);
				leg_motor.q_now[5+0]=motor_chassis[5+0].q_now_flt;
				leg_motor.qd_now[5+0]=motor_chassis[5+0].qd_now_flt;
				leg_motor.t_now[5+0]=motor_chassis[5+0].t_now_flt;
			}
			else if(RxMessage.Data[0]==1+1||RxMessage.Data[0]==1+1+0x10){
				data_can_mit_anal(&motor_chassis[5+1],RxMessage.Data);
				leg_motor.q_now[5+1]=motor_chassis[5+1].q_now_flt;
				leg_motor.qd_now[5+1]=motor_chassis[5+1].qd_now_flt;
				leg_motor.t_now[5+1]=motor_chassis[5+1].t_now_flt;
			}
			else if(RxMessage.Data[0]==2+1||RxMessage.Data[0]==2+1+0x10){
				data_can_mit_anal(&motor_chassis[5+2],RxMessage.Data);
				leg_motor.q_now[5+2]=motor_chassis[5+2].q_now_flt;
				leg_motor.qd_now[5+2]=motor_chassis[5+2].qd_now_flt;
				leg_motor.t_now[5+2]=motor_chassis[5+2].t_now_flt;
			}
			else if(RxMessage.Data[0]==3+1||RxMessage.Data[0]==3+1+0x10){
				data_can_mit_anal(&motor_chassis[5+3],RxMessage.Data);
				leg_motor.q_now[5+3]=motor_chassis[5+3].q_now_flt;
				leg_motor.qd_now[5+3]=motor_chassis[5+3].qd_now_flt;
				leg_motor.t_now[5+3]=motor_chassis[5+3].t_now_flt;
			}
			else if(RxMessage.Data[0]==4+1||RxMessage.Data[0]==4+1+0x10){
				data_can_mit_anal(&motor_chassis[5+4],RxMessage.Data);
				leg_motor.q_now[5+4]=motor_chassis[5+4].q_now_flt;
				leg_motor.qd_now[5+4]=motor_chassis[5+4].qd_now_flt;
				leg_motor.t_now[5+4]=motor_chassis[5+4].t_now_flt;
			}
		  else if(RxMessage.Data[0]==5+1||RxMessage.Data[0]==5+1+0x10){
				data_can_mit_anal(&motor_chassis[5+5],RxMessage.Data);
				leg_motor.q_now[5+5]=motor_chassis[5+5].q_now_flt;
				leg_motor.qd_now[5+5]=motor_chassis[5+5].qd_now_flt;
				leg_motor.t_now[5+5]=motor_chassis[5+5].t_now_flt;
			}
		 else if(RxMessage.Data[0]==6+1||RxMessage.Data[0]==6+1+0x10){
				data_can_mit_anal(&motor_chassis[5+6],RxMessage.Data);
				leg_motor.q_now[5+6]=motor_chassis[5+6].q_now_flt;
				leg_motor.qd_now[5+6]=motor_chassis[5+6].qd_now_flt;
				leg_motor.t_now[5+6]=motor_chassis[5+6].t_now_flt;
			}
	}	
	
		//----------------------------RV??? VESC????
		if (
		(RxMessage.IDE == CAN_Id_Standard) //??????
		&& (RxMessage.IDE == CAN_RTR_Data) //???????
		&& ((RxMessage.DLC >=6)) &&
			motor_chassis[0].motor.type>=EC_1) /* ????????8 */
		{
			RV_can_data_repack(&RxMessage,0,1);//can2
			for(i=0;i<10;i++){
				leg_motor.q_now[i]=motor_chassis[i].q_now_flt;
				leg_motor.qd_now[i]=motor_chassis[i].qd_now_flt; 
				leg_motor.t_now[i]=motor_chassis[i].t_now_flt;
			}
		}			
		
		can2_rx_cnt++;
}
#endif


u8 CAN2_Send_Msg(u8* msg,u8 len,uint32_t id)
{	
	static char cnt_tx;
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;//0x12;	 // ?????????0
  TxMessage.ExtId=0x00;	 // ??????????????29???
  TxMessage.IDE=0;		  // ???????????
  TxMessage.RTR=0;		  // ?????????????????8?
  TxMessage.DLC=len;							 // ??????????
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ???????          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//??????????
  if(i>=0XFFF)
			return 1;
  return 0;		//good
}

u8 CAN2_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN2,CAN_FIFO0)==0)return 0;		//�??????????,?????? 
    CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);//???????	
    for(i=0;i<RxMessage.DLC;i++)
    buf[i]=RxMessage.Data[i];  
	  can2_rx_id=RxMessage.StdId;	
	return RxMessage.DLC;	
}
//----------------------------------------------------------------------------------------------------------------------
void reset_current_cmd(char id)//???????
{
	leg_motor.set_t[id]=0;
	leg_motor.set_i[id]=0;
}

u8 CAN1_Send_Msg_Board(u8* msg,u8 len,uint32_t id)
{	
	static char cnt_tx;
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;//0x12;	 // ?????????0
  TxMessage.ExtId=0x00;//0x12;	 // ??????????????29???
  TxMessage.IDE=0;		  // ???????????
  TxMessage.RTR=0;		  // ?????????????????8?
  TxMessage.DLC=0;							 // ??????????
  for(i=0;i<len;i++)
  TxMessage.Data[i]=0;				 // ???????          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//??????????
  if(i>=0XFFF)
			return 1;
  return 0;		//good
}

u8 CAN2_Send_Msg_Board(u8* msg,u8 len,uint32_t id)
{	
	static char cnt_tx;
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;//0x12;	 // ?????????0
  TxMessage.ExtId=0x00;//0x12;	 // ??????????????29???
  TxMessage.IDE=0;		  // ???????????
  TxMessage.RTR=0;		  // ?????????????????8?
  TxMessage.DLC=0;							 // ??????????
  for(i=0;i<len;i++)
  TxMessage.Data[i]=0;				 // ???????          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//??????????
  if(i>=0XFFF)
			return 1;
  return 0;		//good
}

void CAN_motor_sm(float dt)
{
	char i=0;
	char id_cnt=0;
	
	if(motor_chassis[0].motor.type>=EC_1)//----------ECOS??? motor.type ???????????????
	{
		if(!can_cmd_usb_disable||ocu_connect==0)//usb�????
		{
			for(i=0;i<10;i++)
				motor_chassis[i].en_cmd=leg_motor.motor_en;
				mit_bldc_thread_rv(leg_motor.motor_en,dt);
		}
		else
		{
			for(i=0;i<10;i++)
			{
				motor_chassis[i].en_cmd=motor_chassis[i].en_cmd_ocu;
				mit_bldc_thread_rv(motor_chassis[0].en_cmd_ocu,dt);
			}
		}
	}
	else	//---------------------------------------------DM???
	{
		if(!can_cmd_usb_disable||ocu_connect==0)//usb�????
		{
			for(i=0;i<10;i++)
				motor_chassis[i].en_cmd=leg_motor.motor_en;
			
			mit_bldc_thread(leg_motor.motor_en,dt);
		}
		else
		{
			for(i=0;i<10;i++)
				motor_chassis[i].en_cmd=motor_chassis[i].en_cmd_ocu;
			
			mit_bldc_thread(motor_chassis[0].en_cmd_ocu,dt);
		}
	}
}

