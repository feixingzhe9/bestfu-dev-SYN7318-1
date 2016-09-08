/***************************Copyright BestFu 2015-09-30*************************
��   ��:	WT588D.c
˵   ��:	WT588DоƬ�����ļ�
��   ��:	Keil uVision5 V4.54.0.0
��   ��:	v1.0
��   ��:	yanhuan
ʱ   ��:	2015.09.30 
��   ��:	��
*******************************************************************************/
#include "WT588D.h"
//#include "voice.h"
#include "delay.h"
#include "Thread.h"
#include "GPIO.h"
#include "stm32f10x.h"
#include "sys.h"
#include "LED.h"

#include "VoiceRecognition.h"

#include "PowerStatus.h"

//Voice_t gVoice;
//u8 gVoiceVolum = 7;


static u8 gWT588D_Cmd = 0;
u8 CS_Busy = 0 ;




/*******************************************************************************
�� �� ��:  	WT588D_Init
����˵��:  	WT588DоƬ�����ų�ʼ��
��    ��:  	��			
�� �� ֵ:  	��
ע    �⣺  ��
*******************************************************************************/
void WT588D_PinInit(void)
{
	RCC->APB2ENR&=0xfffffffe;
	RCC->APB2ENR|=0x00000001;//����AFIOʱ��
	AFIO->MAPR&=0xfdffffff;
	AFIO->MAPR|=0x02000000;  //�ر�JTAG������SW
	
	
	GPIOx_Cfg(CS_PORT, CS_PIN  , OUT_PP_2M);
	GPIOx_Set(CS_PORT, CS_PIN );
	GPIOx_Cfg(CLK_PORT, CLK_PIN , OUT_PP_2M);
	GPIOx_Set(CLK_PORT, CLK_PIN);
	GPIOx_Cfg(DATA_PORT, DATA_PIN , OUT_PP_2M);
	GPIOx_Set(DATA_PORT, DATA_PIN);
	
	GPIOx_Cfg(BUSY_PORT, BUSY_PIN , IN_FLOATING); //busy��������
	Ex_NVIC_Config(BUSY_PORT, 8, RTIR); //��Ҫʹ�ܵ�λ,bit7,�����ش���
	EXTI->PR = BUSY_PIN;          //���ж�
	MY_NVIC_Init(0, 2, ((u8)0x17), 2);  //�ж����ȼ�1+0	  
	EXTI->IMR |= BUSY_PIN;     //�����ⲿ�ж�
	
	CS_SET ;
	CLK_SET ;
	DATA_SET ;
}
/*******************************************************************************
�� �� ��:  	Set_WT588D_Cmd
����˵��:  	��������ָ��
��    ��:  	cmd-ָ���ַ��		
�� �� ֵ:  	��
ע    �⣺  ��
*******************************************************************************/
void Set_WT588D_Cmd(u8 cmd)
{
	gWT588D_Cmd = cmd ;
}
/*******************************************************************************
�� �� ��:  	Check_WT588D_State
����˵��:  	���WT588D�Ƿ�æ
��    ��:  	��	
�� �� ֵ:  	״̬
ע    �⣺  ��
*******************************************************************************/
WT588D_Cmd_Reply_T Check_WT588D_State(void)
{
	if(0 == GPIOx_Get(BUSY_PORT,BUSY_PIN)) return BUSY_STATE ;//æ״̬��������ָ��
	return SPKR_STATE ;
}
/*******************************************************************************
�� �� ��:  	Send_WT588D_Cmd
����˵��:  	WT588D��ָ�����
��    ��:  	cmd-ָ���ַ��		
�� �� ֵ:  	��
ע    �⣺  ��
*******************************************************************************/
void Send_WT588D_Cmd(void)
{
	u8 cmd,i ;
	cmd = gWT588D_Cmd ;
	CS_SET;
	delay_ms(20);
	CS_RST;
	delay_ms(5);
	for( i = 0 ; i < 8 ; i++ )
	{
			CLK_RST ;
			if( cmd & 0x01 )
				DATA_SET ;
			else
				DATA_RST ;
			delay_us(300);
			CLK_SET ;     //����bit
			delay_us(300);
			cmd >>= 1;		
	}
	CS_SET;
	delay_ms(50);
}



/*******************************************************************************
�� �� ��:  	Send_WT588D_Cmd_To_OK
����˵��:  	Ϊ�˷�ֹ�������ǣ�����busy�ŵļ����ӳٷ��͵Ļ���
��    ��:  	��	
�� �� ֵ:  	��
ע    �⣺  ��
*******************************************************************************/
void Send_WT588D_Cmd_To_OK(void)
{
	if(SPKR_STATE == Check_WT588D_State()) //��æ
	{
		Thread_Logout(Send_WT588D_Cmd_To_OK);
		Send_WT588D_Cmd();
		
		if(gWT588D_Cmd < 0xE0 || gWT588D_Cmd > 0xE7) //�����������ر�����ʶ��
		{
////////////////////			Voice_OFF();			
		}

	}	
	else
		Thread_Login(ONCEDELAY , 0 , 100 ,Send_WT588D_Cmd_To_OK);		
}


/*******************************************************************************
�� �� ��:  	Set_WT588D_Volume
����˵��:  	������������WT588D������
��    ��:  	vol-������С		
�� �� ֵ:  	��
ע    �⣺  ָ��E0-E7����������
*******************************************************************************/
void Set_WT588D_Volume(u8 volume)
{
	if(volume < 8) 
	{
//		Thread_Logout(LED_Shine);
		Set_WT588D_Cmd(0XE0 + volume);
		Send_WT588D_Cmd_To_OK();
//		Instruct_StateCheck();
	}
}



/*******************************************************************************
�� �� ��:  	WT588D_Report
����˵��:  	����������ʵ��
��    ��:  	cmd-������ַ	
�� �� ֵ:  	��
ע    �⣺  ֧��ͬʱ������������ָ��
*******************************************************************************/
void WT588D_Report(Report_Voice_T cmd)
{
		if((u8)cmd >= 0xE0 && (u8)cmd <= 0xE7) return ;
		
		Set_WT588D_Cmd(0XE0 + gVoice.Volume);
		Send_WT588D_Cmd_To_OK();
		Send_WT588D_Cmd_To_OK();	
		Set_WT588D_Cmd(cmd);
		Send_WT588D_Cmd_To_OK();
		Voice_StopVoice();
	
}

/*******************************************************************************
�� �� ��:  	EXTI9_5_IRQHandler
����˵��:  	BUSY�����ⲿ�жϴ�����
��    ��:  	��
�� �� ֵ:  	��
ע    �⣺  ������������ϣ����øú��� 
*******************************************************************************/
void EXTI9_5_IRQHandler(void)
{
	EXTI->IMR &= ~(BUSY_PIN);     //�����ⲿ�ж�
	EXTI->PR = BUSY_PIN;          //���ж�
//	Voice_ON();
//	Instruct_StateCheck();  //ָʾ��״̬	
	EXTI->IMR |= BUSY_PIN;       //�����ⲿ�ж�
	
}




/**************************Copyright BestFu 2015-09-30*************************/




