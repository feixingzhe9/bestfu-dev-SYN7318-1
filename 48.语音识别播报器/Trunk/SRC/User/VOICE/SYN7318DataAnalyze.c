/******************************Copyright BestFu 2016-03-10*****************************
文件名: Lark7318DataAnalyze.c
说明  : 对从Lark7318接收到的数据进行分析
编译  : Keil uVision4 V5.0
版本  : v1.0
编写  : xjx
日期  : 2016.03.10
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
函数名	:	SetRecognitionPara
功能说明:	识别参数配置
参数	:	u8 distanceLevel;   //识别距离等级
            u16 silentTime;     //静音时间
            u16 speakingTime;   //语音时间
            u8 rejectionLevel;  //拒识级别  
返回值	:	无
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
函数名	:	SYN7318PowerOff
功能说明:	Lark7318关机
参数	:	无
返回值	:	无
*******************************************************************/ 
void SYN7318PowerOff(void)
{
	GPIOx_Rst(POWER_KEY_PORT,POWER_KEY_PIN);
}


/*******************************************************************
函数名	:	SYN7318PowerOff
功能说明:	Lark7318开机
参数	:	无
返回值	:	无
*******************************************************************/ 
void SYN7318PowerOn(void)
{
	GPIOx_Set(POWER_KEY_PORT,POWER_KEY_PIN);
}

/*******************************************************************
函数名	:	Lark7318Prepare
功能说明:	Lark7318准备工作
参数		:	无
返回值	:	无
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
函数名	:	SYN7318PowerOff
功能说明:	Lark7318关机
参数	:	无
返回值	:	无
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
	
	gVoiceState.InitRdy = FALSE;//此时，模块复位，没有ready
	Thread_Login(FOREVER,0,1000,SYN7318Prepare);	
}


/*******************************************************************
函数名	:	Lark7318Init
功能说明:	Lark7318初始化
参数		:	无
返回值	:	无
*******************************************************************/ 
void SYN7318Init(void)
{
	u8 i = 0;
	
	GPIOx_Cfg(CHIP_RST_PORT, CHIP_RST_PIN, OUT_PP_2M);//SYN7318模块的复位键
	GPIOx_Set(CHIP_RST_PORT,CHIP_RST_PIN);
    
    /*结构体初始化*/
	SYN7318Buf = (StructNode_t *)memBlock_malloc();
	SYN7318Buf->Next = NULL;
	SYN7318Buf->len = 0;
	SYN7318Buf->dataType = NO_DATA;
    memset((u8*)&SYN7318Buf->data[0],0,BUF_DATA_LEN);
       
    /*结构体初始化*/
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
函数名	:	AnalyzeRcvData
功能说明:	分析从Lark7318返回的数据
参数		:	无
返回值	:	无
*******************************************************************/ 
void AnalyzeRcvData(void)
{
	u8 len;
	u8 buf[BUF_DATA_LEN] = {0};
	u8 recognitionResult[4] = {0};
	u8 recognitionResultLen = 0;
	
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////*查找接收到的buf里是否有需要的数据*//////////////////////////////////////   
    
    u8 recognitionHaveCmdIdFeedback[4] = {0xfc,0x00,0x06,0x01};	//语音识别有命令ID
	u8 recognitionHaveNoCmdIdFeedback[4] = {0xfc,0x00,0x04,0x02};//语音识别没有命令ID
    
    u8 pos = 0;
    u8 statusFeedback[3]= {0xfc,0x00,0x01};//四个字节的状态回的前三个字节，包括：模块初始化成功回传(0x4a)，\
                                            收到正确的命令帧回传(0x41)，接收失败，收到错误的命令帧回传(0x45)，\
                                            空闲状态回传(0x4f)，以及识别错误代码回传(0x03,0x04,0x05,0x06,0x07)
    u8 checkRecognitionStatus[7] = {0xfc,0x00,0x01,0x41,0xfc,0x00,0x01};
	
	len = GetDataFromChain(buf,DataRcv);
    
    if(0 != len)
    {
        pos = FindTheData(buf,statusFeedback,len,sizeof(statusFeedback));//识别超时，识别拒识，内部错误，状态回传
        if(pos > 0)
        {
            if(pos + 1 <= len)
            {
                    /*识别错误，清零各个状态和数据*/
                if(buf[pos + 1]>2 && buf[pos + 1]<8)//3,4,5,6,7
                {
                    gRecognitionResult.recognitionState = FALSE;//识别失败
                    gRecognitionResult.currentRecognitionIsRdy = FALSE;
                    gRecognitionResult.haveCmdID = FALSE;
        //						gRecognitionResult.wakeUpState = FALSE;
              
                    gVoiceState.RecognitionIsTimeOut = TRUE;////识别超时                   
                }

                else if(0x4f == buf[pos + 1])//空闲状态
                {
                    gVoiceState.State = FREE;
                }					
                else if(0x4a == buf[pos + 1])//模块初始化成功
                {
                    gVoiceState.State = INIT_RDY;	//模块初始化成功后的回传
                    gVoiceState.InitRdy = TRUE;
                }										
                
                else if(0x31 == buf[pos + 1])//词典更新成功
                {						
                    Thread_Logout(VoiceCmdUpdata);
                    Thread_Logout(WT588D_Report_OPERATING_FAIL);
                    if(SYN7318.currentSetCmdID > 0)//补丁：防止修改前导码时重复播报“设置成功”							
                    {
                        WT588D_Report(ACT_OK);
                    }
                    else
                    {
                        WT588D_Report(VOICE_LEAD_MODIFY);
                    }
                                        
                    Thread_Login(ONCEDELAY,0,20,FreeSYN7318Busy);//根据手册，两个数据帧之间要有17ms间隔，这里间隔20ms释放SYN7318

                    if(!((FALSE == gVoiceState.LeadCmd) && (TOUCH == gVoice.workMode)))
                    {
                        StartRecognition();
                    }
                }
                else if(0x32 == buf[pos + 1])//词典更新失败
                {
                    static u8 cnt = 0;
                    cnt++;
                    Thread_Logout(VoiceCmdUpdata);
                    if(cnt < 2)//
                    {
                        VoiceCmdUpdata();
                    }
                    else//有的失败一次会提示，有的失败两次会提示
                    {
                        Thread_Logout(WT588D_Report_OPERATING_FAIL);
                        WT588D_Report_OPERATING_FAIL();
                        Thread_Login(ONCEDELAY,0,20,FreeSYN7318Busy);//根据手册，两个数据帧之间要有17ms间隔，这里间隔20ms释放SYN7318
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
                if(0x42 != buf[pos + 1])//没有处在语音识别状态
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
                    gRecognitionResult.recognitionState = TRUE;//识别成功
                    gRecognitionResult.haveCmdID = TRUE;//无命令ID
                    gRecognitionResult.currentRecognitionIsRdy = TRUE;
                    
                    
                    gRecognitionResult.ciTiaoID = buf[pos + 2];//词条ID高8位
                    gRecognitionResult.ciTiaoID <<=8;
                    gRecognitionResult.ciTiaoID += buf[pos + 3];//词条ID低8位
                    
                    gRecognitionResult.cmdID = buf[pos + 4];//命令ID高8位
                    gRecognitionResult.cmdID <<= 8;
                    gRecognitionResult.cmdID += buf[pos + 5];//命令ID低8位		

                    memcpy(recognitionResult,&buf[pos + 2],4);
    //                recognitionResult[0] = buf[5];//词条ID高8位
    //                recognitionResult[1] = buf[6];//词条ID低8位
    //                recognitionResult[2] = buf[7];//命令ID高8位
    //                recognitionResult[3] = buf[8];//命令ID低8位
                    recognitionResultLen = 4;
                
                    if(SYN7318BufLen < SYN7318_BUFF_MAX_LEN)
                    {
                        SYN7318BufLen = SaveData_SYN7318(recognitionResult,recognitionResultLen,IsRecognitionResult);//识别结果入队
                    }
                    else
                    {
                        ClrItemFromBuf(IsRecognitionResult);
                    }
                                            
                    if(0 == gRecognitionResult.cmdID)//是引导语
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
                    gRecognitionResult.recognitionState = TRUE;//识别成功
                    gRecognitionResult.haveCmdID = FALSE;//无命令ID
                    gRecognitionResult.currentRecognitionIsRdy = TRUE;
                                            
                    gRecognitionResult.ciTiaoID = buf[pos + 2];//词条ID高8位
                    gRecognitionResult.ciTiaoID <<=8;
                    gRecognitionResult.ciTiaoID += buf[pos + 3];//词条ID低8位
                    
                    recognitionResult[0] = buf[pos + 2];//命令ID高8位
                    recognitionResult[1] = buf[pos + 3];//命令ID低8位
                    recognitionResultLen = 2;
                                            
                    if(SYN7318BufLen < SYN7318_BUFF_MAX_LEN)
                    {
                        SYN7318BufLen = SaveData_SYN7318(recognitionResult,recognitionResultLen,IsRecognitionResult);//识别结果入队
                    }
                    else
                    {
                        ClrItemFromBuf(IsRecognitionResult);
                    }
                                            
                    if(TOUCH != gVoice.workMode)//时时激活模式
                    {
                        StartRecognition();
                        if(0 == gRecognitionResult.ciTiaoID)//是引导语
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
函数名	:	CompareData
功能说明:	比较两个等长数组是否一致
参数		:	u8 *data:需要比较的数据1
	u8 len:	数据长度
返回值:	1:相等 		0:不等
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
函数名	:	FindTheData
功能说明:	从指定地址查找需要的一串数据
参数		:	u8 *src:查找源
                u8 *dst：需要匹配的数据
                u8 srcLen:	查找源的数据长度
                u8 dstLen： 需要匹配数据的长度
返回值:	0:未找到 		非0:找到的数据在src的偏移量
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
函数名	:	QueryStateLark7318
功能说明:	向Lark7318发送一个查询所处的工作状态的命令，并根据串口是否有返回数据判断模块是否还在工作
参数		:	无
返回值		:	无
说明		：必须在上电5秒后或者Lark7318初始化成功后才可以调用
*******************************************************************/ 
void QueryStateSYN7318(void)
{
	u8 buf[4] = {0xfd,0x00,0x01,0x21};
	static u8 cnt = 0;
		
	if(FALSE == SYN7318.isBusy)//忙碌的时候不能查询
	{
		UART_Send(buf,4,2);	
		if(FALSE == SYN7318.isAlive)//并且没有返回数据
		{
			if(cnt++ > 2)//三次接收不到查询返回的数据，说明模块已经挂了
			{
				SYN7318RST();//复位模块
				cnt = 0;
			}			
		}
		SYN7318.isAlive = FALSE;	
	}	
}


/*******************************************************************
函数名	:	QueryDataAndSendToLark7318
功能说明:	查询链表数据并发送给Lark7318
参数		:	无
返回值	:	
说明		：
*******************************************************************/ 
void QueryDataAndSendToSYN7318(void)
{
	u8 len;
	u8 buf[BUF_DATA_LEN] = {0};
		
	if(FALSE == SYN7318.isBusy)
	{
		if(SYN7318BufLen > 0)
        {				
#if 0		//不使用语音播报		
			/*语音播报*/
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
			
			
			/*开始识别指令*/
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
			
			
				/*停止识别指令*/
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
