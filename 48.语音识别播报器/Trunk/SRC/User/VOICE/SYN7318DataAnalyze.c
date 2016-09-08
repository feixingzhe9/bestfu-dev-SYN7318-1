/******************************Copyright BestFu 2016-03-10*****************************
�ļ���: Lark7318DataAnalyze.c
˵��  : �Դ�Lark7318���յ������ݽ��з���
����  : Keil uVision4 V5.0
�汾  : v1.0
��д  : xjx
����  : 2016.03.10
*****************************************************************************/

#include "uart.h"	
#include "sys.h"
#include "Thread.h"
#include<stdlib.h>

#include "cmdProcess.h"
#include "thread.h"
#include "VoiceRecognition.h"
#include "KeyUnit.h"

#include "VoicePlayAttr.h"

#include "Lark7318DataAnalyze.h"
#include "LED.h"
#include "WT588D.h"
#include "PowerStatus.h"
#include "MsgPackage.h"
#include "delay.h"

#include "SYN7318Buf.h"

RecognitionResult_t gRecognitionResult;

SYN7318Para_t SYN7318;

u8 CompareData(u8 *data1,u8 *data2,u8 len);
u8 FindTheData(u8 *src,u8 *dst,u8 srcLen,u8 dstLen);
u8 CheckRecognitionTimeOut(void);
void UartRcvProcess(void);
void SYN7318Prepare(void);
void SYN7318Init(void);

StructNode_t *SYN7318Buf = NULL;

/*******************************************************************
������	:	SetRecognitionPara
����˵��:	ʶ���������
����	:	u8 distanceLevel;   //ʶ�����ȼ�
            u16 silentTime;     //����ʱ��
            u16 speakingTime;   //����ʱ��
            u8 rejectionLevel;  //��ʶ����  
����ֵ	:	��
*******************************************************************/ 
void SetRecognitionPara(u8 distance,u16 silentTime,u16 speakingTime,u8 rejectionLevel)
{
    u8 buf[10] = {0};
    buf[0] = 0xFD;
    buf[1] = 0x00;
    buf[2] = 0x06;
    buf[3] = 0x1E;
    
    buf[4] = distance;
    
    buf[5] = (u8)silentTime>>8;
    buf[6] = (u8)silentTime&0xff;
    
    buf[7] = (u8)speakingTime>>8;
    buf[8] = (u8)speakingTime&0xff;
    
    buf[9] = rejectionLevel;
    
    UART_Send(buf,sizeof(buf),2);
    
}


/*******************************************************************
������	:	SYN7318PowerOff
����˵��:	Lark7318�ػ�
����	:	��
����ֵ	:	��
*******************************************************************/ 
void SYN7318PowerOff(void)
{
	GPIOx_Rst(POWER_KEY_PORT,POWER_KEY_PIN);
}


/*******************************************************************
������	:	SYN7318PowerOff
����˵��:	Lark7318����
����	:	��
����ֵ	:	��
*******************************************************************/ 
void SYN7318PowerOn(void)
{
	GPIOx_Set(POWER_KEY_PORT,POWER_KEY_PIN);
}

/*******************************************************************
������	:	Lark7318Prepare
����˵��:	Lark7318׼������
����		:	��
����ֵ	:	��
*******************************************************************/ 
void SYN7318Prepare(void)
{
	static u8 timeCnt = 0;
	static u8 WT588D_ReportFlag = 0;
	timeCnt++;
	if(timeCnt > 3 || TRUE == gVoiceState.InitRdy)
	{		
		timeCnt = 0;
		FreeSYN7318Busy();
		LED_RedOff();
		
        
        
        
        //////////////////////////////////////
        SYN7318PowerOff();
        ///////////////////////////////////////////
        
        
        
        
		if (BATTERY_POWER == PowerStatus_Get())	//??????? 
		{
			LowPowerWarning(gVoiceState.cap);
		}

		if(0 == WT588D_ReportFlag)
		{
			WT588D_Report(WELCOME);
			WT588D_ReportFlag = 1;
		}
		
		Thread_Login(FOREVER,0,17,WorkModeFun);
		
		Thread_Login(FOREVER,0,20,Key_process);
		Voice_StopVoice();

		Thread_Login(FOREVER,0,1000,QueryStateSYN7318);
		Thread_Login(FOREVER,0,13,QueryDataAndSendToSYN7318);	
		
		if(TOUCH != gVoice.workMode)//
		{
			StartRecognition();
		}		
		Thread_Logout(SYN7318Prepare);	
        
        SetRecognitionPara(3,4000,4000,2);
	}
}




/*******************************************************************
������	:	SYN7318PowerOff
����˵��:	Lark7318�ػ�
����	:	��
����ֵ	:	��
*******************************************************************/ 
void SYN7318RST(void)
{

	Thread_Logout(WorkModeFun);
	Thread_Logout(Key_process);
	Thread_Logout(QueryStateSYN7318);
	Thread_Logout(QueryDataAndSendToSYN7318);	
	
	SYN7318.isBusy = TRUE;
	
	LED_RedOn();
	
	GPIOx_Rst(CHIP_RST_PORT,CHIP_RST_PIN);
	delay_ms(100);
	GPIOx_Set(CHIP_RST_PORT,CHIP_RST_PIN);
	
	gVoiceState.InitRdy = FALSE;//��ʱ��ģ�鸴λ��û��ready
	Thread_Login(FOREVER,0,1000,SYN7318Prepare);	
}


/*******************************************************************
������	:	Lark7318Init
����˵��:	Lark7318��ʼ��
����		:	��
����ֵ	:	��
*******************************************************************/ 
void SYN7318Init(void)
{
	u8 i = 0;
	
	GPIOx_Cfg(CHIP_RST_PORT, CHIP_RST_PIN, OUT_PP_2M);//SYN7318ģ��ĸ�λ��
	GPIOx_Set(CHIP_RST_PORT,CHIP_RST_PIN);
    
    /*�ṹ���ʼ��*/
	SYN7318Buf = (StructNode_t *)memBlock_malloc();
	SYN7318Buf->Next = NULL;
	SYN7318Buf->len = 0;
	SYN7318Buf->dataType = NO_DATA;
    memset((u8*)&SYN7318Buf->data[0],0,BUF_DATA_LEN);
       
    /*�ṹ���ʼ��*/
    SYN7318.currentSetCmdID = 0;
    SYN7318.isAlive = FALSE;
    SYN7318.recognitionResultToUpLoad = 0;
    SYN7318.isBusy = TRUE;
       
	for(i=0;i<20;i++)
	{
		SYN7318Buf->data[i] = 0;
	}
	
	Thread_Login(FOREVER,0,5,UartRcvProcess);//
	Thread_Login(FOREVER,0,50,AnalyzeRcvData);//
	
	Thread_Login(FOREVER,0,1000,SYN7318Prepare);		
}

/*******************************************************************
������	:	AnalyzeRcvData
����˵��:	������Lark7318���ص�����
����		:	��
����ֵ	:	��
*******************************************************************/ 
void AnalyzeRcvData(void)
{
	u8 len;
	u8 buf[BUF_DATA_LEN] = {0};
	u8 recognitionResult[4] = {0};
	u8 recognitionResultLen = 0;
	
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////*���ҽ��յ���buf���Ƿ�����Ҫ������*//////////////////////////////////////   
    
    u8 recognitionHaveCmdIdFeedback[4] = {0xfc,0x00,0x06,0x01};	//����ʶ��������ID
	u8 recognitionHaveNoCmdIdFeedback[4] = {0xfc,0x00,0x04,0x02};//����ʶ��û������ID
    
    u8 pos = 0;
    u8 statusFeedback[3]= {0xfc,0x00,0x01};//�ĸ��ֽڵ�״̬�ص�ǰ�����ֽڣ�������ģ���ʼ���ɹ��ش�(0x4a)��\
                                            �յ���ȷ������֡�ش�(0x41)������ʧ�ܣ��յ����������֡�ش�(0x45)��\
                                            ����״̬�ش�(0x4f)���Լ�ʶ��������ش�(0x03,0x04,0x05,0x06,0x07)
    u8 checkRecognitionStatus[7] = {0xfc,0x00,0x01,0x41,0xfc,0x00,0x01};
	
	len = GetDataFromChain(buf,DataRcv);
    
    if(0 != len)
    {
        pos = FindTheData(buf,statusFeedback,len,sizeof(statusFeedback));//ʶ��ʱ��ʶ���ʶ���ڲ�����״̬�ش�
        if(pos > 0)
        {
            if(pos + 1 <= len)
            {
                    /*ʶ������������״̬������*/
                if(buf[pos + 1]>2 && buf[pos + 1]<8)//3,4,5,6,7
                {
                    gRecognitionResult.recognitionState = FALSE;//ʶ��ʧ��
                    gRecognitionResult.currentRecognitionIsRdy = FALSE;
                    gRecognitionResult.haveCmdID = FALSE;
        //						gRecognitionResult.wakeUpState = FALSE;
              
                    gVoiceState.RecognitionIsTimeOut = TRUE;////ʶ��ʱ                   
                }

                else if(0x4f == buf[pos + 1])//����״̬
                {
                    gVoiceState.State = FREE;
                }					
                else if(0x4a == buf[pos + 1])//ģ���ʼ���ɹ�
                {
                    gVoiceState.State = INIT_RDY;	//ģ���ʼ���ɹ���Ļش�
                    gVoiceState.InitRdy = TRUE;
                }										
                
                else if(0x31 == buf[pos + 1])//�ʵ���³ɹ�
                {						
                    Thread_Logout(VoiceCmdUpdata);
                    Thread_Logout(WT588D_Report_OPERATING_FAIL);
                    if(SYN7318.currentSetCmdID > 0)//��������ֹ�޸�ǰ����ʱ�ظ����������óɹ���							
                    {
                        WT588D_Report(ACT_OK);
                    }
                    else
                    {
                        WT588D_Report(VOICE_LEAD_MODIFY);
                    }
                                        
                    Thread_Login(ONCEDELAY,0,20,FreeSYN7318Busy);//�����ֲᣬ��������֮֡��Ҫ��17ms�����������20ms�ͷ�SYN7318

                    if(!((FALSE == gVoiceState.LeadCmd) && (TOUCH == gVoice.workMode)))
                    {
                        StartRecognition();
                    }
                }
                else if(0x32 == buf[pos + 1])//�ʵ����ʧ��
                {
                    static u8 cnt = 0;
                    cnt++;
                    Thread_Logout(VoiceCmdUpdata);
                    if(cnt < 2)//
                    {
                        VoiceCmdUpdata();
                    }
                    else//�е�ʧ��һ�λ���ʾ���е�ʧ�����λ���ʾ
                    {
                        Thread_Logout(WT588D_Report_OPERATING_FAIL);
                        WT588D_Report_OPERATING_FAIL();
                        Thread_Login(ONCEDELAY,0,20,FreeSYN7318Busy);//�����ֲᣬ��������֮֡��Ҫ��17ms�����������20ms�ͷ�SYN7318
                        cnt = 0;
                    }
                    
                    if(!((FALSE == gVoiceState.LeadCmd) && (TOUCH == gVoice.workMode)))
                    {
                        StartRecognition();
                    }
                }
//                else if(0x41 == buf[pos + 1])
//                {
//                    u8 test = 0;
//                    test++;
//                }
              
            }
            
        }
        
        pos = FindTheData(buf,checkRecognitionStatus,len,sizeof(checkRecognitionStatus));
        if(pos > 0)
        {
            if(pos + 1 < len)
            {
                if(0x42 != buf[pos + 1])//û�д�������ʶ��״̬
                {
                    if(!((FALSE == gVoiceState.LeadCmd) && (TOUCH == gVoice.workMode)))
                    {
                        StartRecognition();
                    }
                }	
            }
        }
        
        
      
        pos = FindTheData(buf,recognitionHaveCmdIdFeedback,len,sizeof(recognitionHaveCmdIdFeedback));
        if(pos > 0)
        {
            if(pos + 1 <= len)
            {
                gRecognitionResult.matchDegree = buf[pos + 1];
                        
                if(gRecognitionResult.matchDegree >= gVoice.Senstivity)
                {
                    gRecognitionResult.recognitionState = TRUE;//ʶ��ɹ�
                    gRecognitionResult.haveCmdID = TRUE;//������ID
                    gRecognitionResult.currentRecognitionIsRdy = TRUE;
                    
                    
                    gRecognitionResult.ciTiaoID = buf[pos + 2];//����ID��8λ
                    gRecognitionResult.ciTiaoID <<=8;
                    gRecognitionResult.ciTiaoID += buf[pos + 3];//����ID��8λ
                    
                    gRecognitionResult.cmdID = buf[pos + 4];//����ID��8λ
                    gRecognitionResult.cmdID <<= 8;
                    gRecognitionResult.cmdID += buf[pos + 5];//����ID��8λ		

                    memcpy(recognitionResult,&buf[pos + 2],4);
    //                recognitionResult[0] = buf[5];//����ID��8λ
    //                recognitionResult[1] = buf[6];//����ID��8λ
    //                recognitionResult[2] = buf[7];//����ID��8λ
    //                recognitionResult[3] = buf[8];//����ID��8λ
                    recognitionResultLen = 4;
                
                    if(SYN7318BufLen < SYN7318_BUFF_MAX_LEN)
                    {
                        SYN7318BufLen = SaveData_SYN7318(recognitionResult,recognitionResultLen,IsRecognitionResult);//ʶ�������
                    }
                    else
                    {
                        ClrItemFromBuf(IsRecognitionResult);
                    }
                                            
                    if(0 == gRecognitionResult.cmdID)//��������
                    {
                        gVoiceState.isWakeUp = TRUE;
                    }						
                }
                else
                {
                    if(TOUCH != gVoice.workMode)//
                    {
                        StartRecognition();
                    }
                }					
            }
        }
        
        pos = FindTheData(buf,recognitionHaveNoCmdIdFeedback,len,sizeof(recognitionHaveNoCmdIdFeedback));
        if(pos > 0)
        {
            if(pos + 1 <= len)
            {
                gRecognitionResult.matchDegree = buf[pos + 1];
                if(gRecognitionResult.matchDegree >= gVoice.Senstivity)
                {				
                    gRecognitionResult.recognitionState = TRUE;//ʶ��ɹ�
                    gRecognitionResult.haveCmdID = FALSE;//������ID
                    gRecognitionResult.currentRecognitionIsRdy = TRUE;
                                            
                    gRecognitionResult.ciTiaoID = buf[pos + 2];//����ID��8λ
                    gRecognitionResult.ciTiaoID <<=8;
                    gRecognitionResult.ciTiaoID += buf[pos + 3];//����ID��8λ
                    
                    recognitionResult[0] = buf[pos + 2];//����ID��8λ
                    recognitionResult[1] = buf[pos + 3];//����ID��8λ
                    recognitionResultLen = 2;
                                            
                    if(SYN7318BufLen < SYN7318_BUFF_MAX_LEN)
                    {
                        SYN7318BufLen = SaveData_SYN7318(recognitionResult,recognitionResultLen,IsRecognitionResult);//ʶ�������
                    }
                    else
                    {
                        ClrItemFromBuf(IsRecognitionResult);
                    }
                                            
                    if(TOUCH != gVoice.workMode)//ʱʱ����ģʽ
                    {
                        StartRecognition();
                        if(0 == gRecognitionResult.ciTiaoID)//��������
                        {
                            gVoiceState.isWakeUp = TRUE;
                        }
                    }
                }
            }
        }
    }	
}

/*******************************************************************
������	:	CompareData
����˵��:	�Ƚ������ȳ������Ƿ�һ��
����		:	u8 *data:��Ҫ�Ƚϵ�����1
	u8 len:	���ݳ���
����ֵ:	1:��� 		0:����
*******************************************************************/ 
u8 CompareData(u8 *data1,u8 *data2,u8 len)
{
	u8 i;
	for(i=0;i<len;i++)
	{
		if(data1[i] != data2[i])
		{
			return 0;
		}		
	}
	return 1;
}


/*******************************************************************
������	:	FindTheData
����˵��:	��ָ����ַ������Ҫ��һ������
����		:	u8 *src:����Դ
                u8 *dst����Ҫƥ�������
                u8 srcLen:	����Դ�����ݳ���
                u8 dstLen�� ��Ҫƥ�����ݵĳ���
����ֵ:	0:δ�ҵ� 		��0:�ҵ���������src��ƫ����
*******************************************************************/ 
u8 FindTheData(u8 *src,u8 *dst,u8 srcLen,u8 dstLen)
{
	u8 i;
    u8 n = 0;
    for(i = 0; i < srcLen; i++)
    {
        if(src[i] == dst[n])
        {
            if(++n == dstLen)
            {
                return i;
            }
        }
        else
        {
            n = 0;
        }        
    }	
	return 0;
}
/*******************************************************************
������	:	QueryStateLark7318
����˵��:	��Lark7318����һ����ѯ�����Ĺ���״̬����������ݴ����Ƿ��з��������ж�ģ���Ƿ��ڹ���
����		:	��
����ֵ		:	��
˵��		���������ϵ�5������Lark7318��ʼ���ɹ���ſ��Ե���
*******************************************************************/ 
void QueryStateSYN7318(void)
{
	u8 buf[4] = {0xfd,0x00,0x01,0x21};
	static u8 cnt = 0;
		
	if(FALSE == SYN7318.isBusy)//æµ��ʱ���ܲ�ѯ
	{
		UART_Send(buf,4,2);	
		if(FALSE == SYN7318.isAlive)//����û�з�������
		{
			if(cnt++ > 2)//���ν��ղ�����ѯ���ص����ݣ�˵��ģ���Ѿ�����
			{
				SYN7318RST();//��λģ��
				cnt = 0;
			}			
		}
		SYN7318.isAlive = FALSE;	
	}	
}


/*******************************************************************
������	:	QueryDataAndSendToLark7318
����˵��:	��ѯ�������ݲ����͸�Lark7318
����		:	��
����ֵ	:	
˵��		��
*******************************************************************/ 
void QueryDataAndSendToSYN7318(void)
{
	u8 len;
	u8 buf[BUF_DATA_LEN] = {0};
		
	if(FALSE == SYN7318.isBusy)
	{
		if(SYN7318BufLen > 0)
        {				
#if 0		//��ʹ����������		
			/*��������*/
			if((0x49 != gVoiceState.State)&&(0x4b != gVoiceState.State)\
				&&(0x4e != gVoiceState.State)&&(0x42 != gVoiceState.State))
			{
				if(SYN7318BufLen > 0)
				{
					len = GetDataFromChain(buf,IsVoicePlay);			
					if(len > 0)
					{
						UART_Send(buf,len,2);
					}	
				}
					
			}
#endif
			
			
			/*��ʼʶ��ָ��*/
		//	if((0x49 != gVoiceState.State)&&(0x4b != gVoiceState.State)\
		//		&&(0x4e != gVoiceState.State)/*&&(0x42 != gVoiceState.State)*/)
			{
				if(SYN7318BufLen > 0)
				{
					len = GetDataFromChain(buf,IsStartRecognition);						
					if(len > 0)
					{			
						UART_Send(buf,len,2);				
					}
				}
				
			}
			
			
				/*ֹͣʶ��ָ��*/
		//	if((0x49 != gVoiceState.State)&&(0x4b != gVoiceState.State)\
		//		&&(0x4e != gVoiceState.State)/*&&(0x42 != gVoiceState.State)*/)
			{
				if(SYN7318BufLen > 0)
				{
					len = GetDataFromChain(buf,IsStopRecognition);
					if(len > 0)
					{
						UART_Send(buf,len,2);
					}
				}
			}
		}
	}				
}


/****************File End***********Copyright BestFu 2014-05-14*****************/
