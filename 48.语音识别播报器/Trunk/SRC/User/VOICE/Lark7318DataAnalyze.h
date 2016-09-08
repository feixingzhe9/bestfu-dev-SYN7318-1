/***************************Copyright BestFu 2014-05-14*************************
�ļ���:	Lark7318DataAnalyze.h
����	:	
����	:	Keil uVision5
�汾	:	v1.0
��д	:	xjx
����	:	2016.03.11
�޸�	:	����
*******************************************************************************/
#ifndef __LARK7318DATAANALYZE_H
#define __LARK7318DATAANALYZE_H
#include "sys.h"

typedef struct
{
	u8 recognitionState;//����ʶ��״̬��ʧ�ܣ��ɹ�
//	u8 wakeUpState;	//��������״̬
	u8 haveCmdID;	//�Ƿ�������ID
	u8 currentRecognitionIsRdy;//		��ǰʶ�����Ƿ�ready
	u8 matchDegree;
	u16 cmdID;			//����ID
	u16 ciTiaoID;		//����ID	
}RecognitionResult_t;//����ʶ�����ṹ��

typedef struct
{
	u8 len;			//���ݳ���
	u8 data[200];//����ʶ��ָ������
}RecognitionCmd_t;//����ʶ�����ݽṹ��

typedef struct 
{
    u8 isBusy;
    u8 currentSetCmdID;
    u8 isAlive;
    u8 recognitionResultToUpLoad;
    u8 lowPowerProtect;//�͵���������־λ
}SYN7318Para_t;

extern SYN7318Para_t SYN7318;

extern void QueryStateSYN7318(void);
extern void QueryDataAndSendToSYN7318(void);
extern void SYN7318Init(void);
//extern void KeyProcess(void);



extern void TestVoiceSpeakerFun(void);////���Ժ���/////////



#endif






/**************************Copyright BestFu 2014-05-14*************************/


