/***************************Copyright BestFu 2015-09-30*************************
��   ��:	WT588D.h
˵   ��:	WT588DоƬ����ͷ�ļ�
��   ��:	Keil uVision5 V4.54.0.0
��   ��:	v1.0
��   ��:	yanhuan
ʱ   ��:	2015.09.30 
��   ��:	��
*******************************************************************************/
#ifndef _WT588D_H
#define _WT588D_H


#include "BF_type.h"
#include "gpio.h"

#define VOLUME_VALID         (0x5a)
//�����ŷֱ�ΪCS��CLK��DATA��BUSY
#define CS_PORT      GPIO_C
#define CS_PIN       pin_15

#define CLK_PORT     GPIO_A
#define CLK_PIN      pin_15

#define DATA_PORT    GPIO_B
#define DATA_PIN     pin_2

#define BUSY_PORT    GPIO_A
#define BUSY_PIN     pin_8

#define CS_RST       GPIOx_Rst(CS_PORT , CS_PIN)
#define CS_SET		   GPIOx_Set(CS_PORT , CS_PIN)
#define CLK_RST      GPIOx_Rst(CLK_PORT , CLK_PIN)
#define CLK_SET		   GPIOx_Set(CLK_PORT , CLK_PIN)
#define DATA_RST     GPIOx_Rst(DATA_PORT , DATA_PIN)
#define DATA_SET	   GPIOx_Set(DATA_PORT , DATA_PIN)

typedef enum
{
	SPKR_STATE,
	BUSY_STATE,
} WT588D_Cmd_Reply_T ;

typedef enum
{
	WELCOME = 0 , // 0 "��ӭʹ��"
	IDENTIFY_COMPLETE ,//1 "ʶ��ɹ�"
	RECOVERY_SET , //2 "��ӭʹ�ñ��ร����ϵͳ"
	DEV_INSTRUCT ,//3 "�豸��֤"
	SLEEP_AFTER ,//4 "������������"
	MODE_VOICEandTOUCH_LEAD ,//5 "�����ʹ���ǰ��"
	MODE_TOUCH_LEAD , //6 "����ǰ��"
	NO_LEAD , //7 "��ǰ��"
	ACTIVATE ,//8 �豸�Ѽ����ָʾ
	IN_ACTIVATE ,//9 �뼤���豸
	LOW_BATTERY , //10 "������"
	CHARGING , //11 "���ڳ��"	
	PLEASE_CMD ,//12 ����ָʾ��
	VOICE_LEAD_MODIFY ,//13������ǰ�������޸�
	UPDATA_CHECK , //14,�豸���������������Ӻ��ⲿ��Դ
	UPDATA_OK , //15,�����ɹ����豸��������
	UPDATA_ING , //16,����������
	ACT_OK ,//17,�����ɹ�
	DEV_DEAL , //18��"��ӭʹ�ñ��ร����ϵͳ,�ѳɹ��ָ��������ã���ͬ���豸���ݻ�������Ӹ��豸"
	OPERATING,//19,����ʶ���������ã����Ժ�
	OPERATING_FAIL,//20,����ʧ�ܣ������²���
} Report_Voice_T;


//typedef enum
//{
//	TOUCH,
//	TOUCH_AND_Voice,
//	ALWAYS_ON
//}RecognitionMode_t;
extern u8 CS_Busy ;
void WT588D_PinInit(void);
void Set_WT588D_Volume(u8 volume);
void WT588D_Report(Report_Voice_T cmd);

//void TouchModeFun(void);
//void VoiceAndTouchModeFun(void);
void WorkModeFun(void);
#endif
/**************************Copyright BestFu 2015-09-30*************************/

