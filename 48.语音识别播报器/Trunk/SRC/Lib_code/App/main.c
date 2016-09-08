/***************************Copyright BestFu 2014-05-14*************************
��	����	main.c
˵	����	ϵͳ������
��	�룺	Keil uVision4 V4.54.0.0
��	����	v1.0
��	д��	Unarty
��	�ڣ�	2014.06.22 
�ޡ��ģ�	����
*******************************************************************************/
#include "BF_type.h"
#include "SysHard.h"
#include "Thread.h"
#include "cmdProcess.h"
#include "Instruct.h"
#include "wdg.h"
#include "DeviceReset.h"
#include "Test.h"

#include "Lark7318DataAnalyze.h"
#include "WT588D.h"
#include "uart.h"
#include "PowerStatus.h"
/*******************************************************************************
  ������:  	main
����˵��:	������
	����:  	��
  ����ֵ:  	��
*******************************************************************************/
int main(void)
{
	PowerStatus_Init();//�������ʼ����Ϊ��������״̬�¿��Կ��ٻ���
    Thread_Init();
	SysHard_Init();		//ϵͳ���Ӳ����ʼ��
	WDG_Init(5,2000);
	FIFOInit();			//��ʼ�����л�����
	Instruct_Init();
	DeviceResetCheck_Init();
	Test_Init();
	Unit_Init();
	            
	while(1)
	{
		Process_FIFOData(&rcv433fifo, &rcv_433); 		//�������FIFO	
		Process_FIFOData(&rcv433_shortfifo, &rcv_433short); 		//�������FIFO		
		Thread_Process();
	}
}

/**************************Copyright BestFu 2014-05-14*************************/
