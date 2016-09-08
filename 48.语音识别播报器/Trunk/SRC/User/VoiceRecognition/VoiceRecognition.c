/***************************Copyright BestFu 2014-05-14*************************
文	件：	VoiceRecognition.c
说	明：	语音识别逻辑层
编	译：	Keil uVision4 V4.54.0.0
版	本：	v1.0
编	写：	Unarty
日	期：	2014.09.01 
修　改：	暂无
*******************************************************************************/

#include "VoiceRecognition.h"
#include "VoiceExeAttr.h"
#include "VoicePlayAttr.h"
#include "VoicePlayUnit.h"
#include "stm32f10x.h"
#include "SysHard.h"
#include "Thread.h"
#include "LED.h"
#include "uart.h"	
#include "PowerStatus.h"

#include "Capacity.h"
#include "KeyUnit.h"
#include "delay.h"

#include "Lark7318.h"

#include "si4432.h"

#include "WT588D.h"
#include "SYN7318Buf.h"
#include "Lark7318DataAnalyze.h"

#include<stdlib.h>

Voice_t gVoice;
VoiceState_t gVoiceState;



/*******************************************************************************
函 数 名:  	VoiceSet
功能说明:  	因语音识别模块没有记忆功能，需要重新设置发音人、音量、语调、语速等
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void VoiceSet(void)
{
	EEPROM_Read(VOICE_SET_ADDR, sizeof(Voice_t), (u8*)&gVoice);	
	VoicePlayState(gVoice.Volume,gVoice.Intonation,gVoice.Speed);
	VoiceSperkers(gVoice.Speakers);
	Voice_Reset();
	KeyUnit_Init();
}

/*******************************************************************************
函 数 名:  	Voice_Init
功能说明:  	初始化语音识别
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void Voice_Init(void)
{
	EEPROM_Read(VOICE_SET_ADDR, sizeof(Voice_t), (u8*)&gVoice);	
	
	
	if((gVoice.Volume > MAX_VOLUME) || (gVoice.workMode > WORK_MODE_MAX_VALUE) || (gVoice.Senstivity > 100))
	{
		Voice_SetInit();
	}
		
	Capacity_Init();	//开启电量检测
	
	if(!gVoice.UpdateCmd)
	{
		gVoice.UpdateCmd = 1;
		Voice_SetSave();
		Thread_Login(ONCEDELAY , 0, 500, Voice_Reset);
		Thread_Login(ONCEDELAY , 0, 1000, KeyUnit_Init);
	}
	else
	{
		Thread_Login(ONCEDELAY , 0, 3000, VoiceSet);
	}
	
	gVoiceState.State = FREE;
	PowerDownCountReset();	//休眠模式
}

/*******************************************************************************
函 数 名:  	VoiceRecognition_Init()
功能说明:  	初始化语音设置值
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void VoiceRecognition_Init(void)
{
	GPIOx_Cfg(PWRKEY_PORT, 	 PWRKEY_PIN, IN_UPDOWN);
	GPIOx_Cfg(CHIPREST_PORT, CHIPREST_PIN, IN_UPDOWN);
	GPIOx_Rst(PWRKEY_PORT,   PWRKEY_PIN);
	GPIOx_Set(CHIPREST_PORT, CHIPREST_PIN);
}
/*******************************************************************************
函 数 名:  	Voice_SetInit()
功能说明:  	初始化语音设置值
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void Voice_SetInit(void)
{
	u8 buf1[20] = {0xFD,0x00,0x11,0x12,0x00,0xb1,0xb4,0xb6,0xe0,0xb8,0xa3,0xbf,0xc6,0xbc,0xbc,0x20,0x31,0x39,0x39,0x7c};//贝多福科技，没有命令ID
	u8 updatebuf[6] = {0xFD,0x00,0x03,0x13,0x00,0x00};
	
	VoiceSave_t voiceAdd = {0};

	gVoice.Senstivity = 60;
	gVoice.Volume = 3;		//音量
	gVoice.Intonation = 5;	//语调
	gVoice.Speed = 5;		//语速
	gVoice.Speakers = 0;	//发音人
	gVoice.workMode = VOICE_TOUCH;	//工作模式默认在触摸引导模式
	gVoice.DormancyTime = 15;
	VoiceSperkers(gVoice.Speakers);
	VoicePlayState(gVoice.Volume,gVoice.Intonation,gVoice.Speed);
	Voice_SetSave();

	EEPROM_Write(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);
	
//	if(FALSE == SYN7318.isBusy)
	{
		SetSYN7318Busy();
		SYN7318.currentSetCmdID = 199;//补丁：
		UART_Send(buf1, 20, 2);
		delay_ms(100);
		UART_Send(updatebuf, 6, 2);	//识别词条缓存更新命令
		Thread_Login(ONCEDELAY,0,4000,FreeSYN7318Busy);
	}	

}

/*******************************************************************************
函 数 名:  	Voice_SetSave()
功能说明:  	语音设置值EEPROM存储
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void Voice_SetSave(void)
{
	EEPROM_Write(VOICE_SET_ADDR, sizeof(Voice_t), (u8*)&gVoice);	
}

/*******************************************************************************
函 数 名:  	Voice_StopVoice
功能说明:  	停止识别
参    数:  	无 			
返 回 值:  	无
注	  意:
*******************************************************************************/
void Voice_StopVoice(void)
{
	u8 buf[4] = {0xFD,0x00,0x01,0x11};
	
	UART_Send(buf, 4, 2);
}
/*******************************************************************************
函 数 名:  	WakeThread
功能说明:  	唤醒线程
参    数:  	无 			
返 回 值:  	无
注	  意：	会更新全
*******************************************************************************/
void WakeThread(void)
{
	u8 updatebuf[5] = {0xFD,0x00,0x02,0x13,0x01};
	
	UART_Send(updatebuf, 5, 2);	//识别词条缓存更新命令


	Thread_Wake(Key_process);
	Si4432_Init();
}

/*******************************************************************************
函 数 名:  	FreeSYN7318Busy
功能说明:  	释放SYN7318的忙碌状态
参    数:  	无 			
返 回 值:  	无
注	  意：	
*******************************************************************************/
void FreeSYN7318Busy(void)
{
	SYN7318.isBusy = FALSE;
}

/*******************************************************************************
函 数 名:  	SetSYN7318Busy
功能说明:  	设置SYN7318为忙碌状态
参    数:  	无 			
返 回 值:  	无
注	  意：	
*******************************************************************************/
void SetSYN7318Busy(void)
{
	SYN7318.isBusy = TRUE;
}
/*******************************************************************************
函 数 名:  	WT588D_Report_OPERATING
功能说明:  	语音播报“语音识别正在配置，请稍后”
参    数:  	无 			
返 回 值:  	无
注	  意：	
*******************************************************************************/
void WT588D_Report_OPERATING(void)
{
	WT588D_Report(OPERATING);	
}


/*******************************************************************************
函 数 名:  	WT588D_Report_OPERATING_FAIL
功能说明:  	语音播报“配置失败，请重新操作”
参    数:  	无 			
返 回 值:  	无
注	  意：	
*******************************************************************************/
void WT588D_Report_OPERATING_FAIL(void)
{
	WT588D_Report(OPERATING_FAIL);//	
}


/*******************************************************************************
函 数 名:  	VoiceCmdUpdata
功能说明:  	更新语音指令
参    数:  	无 			
返 回 值:  	无
注	  意：	会更新全
*******************************************************************************/
void VoiceCmdUpdata(void)
{
	u8 i = 0;
	u8 j = 0;
	u8 m = 0;
	u8 flag = 0;
	u8 buf[21] = {0xFD,0x00,0x00,0x12,0x00,0x01};
	u8 updatebuf[6] = {0xFD,0x00,0x03,0x13,0x00,0x00};
	VoiceSave_t voiceAdd = {0};
	static u8 cnt = 0;
	
	u8 sendCnt = 0;
	u8 sendBuf[250] = {0};

	cnt++;
       
    if(SYN7318.currentSetCmdID > 0)//补丁：设置前导码时不播报
	{
		WT588D_Report_OPERATING();
	}
    
	SetSYN7318Busy();
	
	EEPROM_Read(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);	//获取语音存储表
	
	voiceAdd.Total += 2;
		
	for(i=0;i<voiceAdd.num+1;i++)
	{
		if (0 != (voiceAdd.add[i>>5]&(1<<(i&0x1F))))	//有存储数据
		{
			voiceAdd.Total++;
		}
	}

	buf[1] = (u8)(voiceAdd.Total>>8);
	buf[2] = (u8)voiceAdd.Total;
	
	UART_Send(buf, 5, 2);
		
	sendCnt = voiceAdd.num / 10 + 1;    //一次读10个
	for(i = 0; i < sendCnt; i++)        //一次读10个
	{
		for(m = 0; m < 10; m++)
		{
			if (0 != (voiceAdd.add[(10 * i + m)>>5]&(1<<((10 * i + m)&0x1F))))	//有存储数据
			{
				flag = 1;
				break;
			}
			flag = 0;
		}
		if(1 == flag)
		{
			EEPROM_Read(VOICE_DATA_CMD_ADDR + i *10 *sizeof(VoiceCmd_t), 10 *sizeof(VoiceCmd_t), sendBuf);
			for(j = 0; j < 10; j++)
			{
				if (0 != (voiceAdd.add[(10 * i + j)>>5]&(1<<((10 * i + j)&0x1F))))	//有存储数据
				{
					sendBuf[25 * j + voiceAdd.len[i * 10 + j]] = 0x7c;
					UART_Send(&sendBuf[25 * j], voiceAdd.len[i * 10 + j] + 1, 2);
				}		
			}
		}
		
	}
	
	delay_ms(100);
	
	UART_Send(updatebuf, 6, 2);	//识别词条缓存更新命令

	if(cnt > 0)
	{
		Thread_Wake(Key_process);
		Thread_Wake(ImmediatelyDormancy);
		Thread_Wake(VoiceDormancy);
		
		Si4432_Init();

		Thread_Logout(VoiceCmdUpdata);
	}	
}

/*******************************************************************************
函 数 名:  	Voice_CmdUpdata
功能说明:  	更新语音指令缓冲区内容
参    数:  	无 			
返 回 值:  	无
注	  意：	会更新全
*******************************************************************************/
void Voice_CmdUpdata(void)
{

    EXTI->IMR &= ~(nIRQ_PIN);		//屏蔽433中断

    Thread_Hang(Key_process);
    Thread_Hang(ImmediatelyDormancy);
    Thread_Hang(VoiceDormancy);
    
    Voice_StopVoice();
    
    VoiceCmdUpdata();
}

/*******************************************************************************
函 数 名:  	Voice_CmdAdd
功能说明:  	语音指令添加
参    数:  	pVoiceCmd: 语音指令码: ID/内容  			
返 回 值:  	消息执行结果
*******************************************************************************/
MsgResult_t Voice_CmdAdd(VoiceCmd_t *pVoiceCmd,u8 len,u8 ID)
{
	VoiceSave_t voiceAdd = {0};
    MsgResult_t returnValue;
	
//	Thread_Login(ONCEDELAY, 0, 1500, Voice_CmdUpdata);	//延时更新语音指令Buf

	EEPROM_Read(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);	//获取语音存储表

	voiceAdd.add[ID>>5] |= (1<<(ID&0x1F));		//存储标识置1
	
	voiceAdd.Total = voiceAdd.Total - voiceAdd.len[ID] + (u16)len;
	voiceAdd.len[ID] = len;
	
	pVoiceCmd->cmd[len] = 0x20;//空格
	voiceAdd.len[ID]++;
	
	//把 u8 转换为 ASCII码
	if(ID < 10)
	{
		pVoiceCmd->cmd[len + 1] = ID + 0x30;
		
		voiceAdd.len[ID]++;
		voiceAdd.Total += 2;
	}
	else if(ID < 100)
	{
		pVoiceCmd->cmd[len + 1] = ID / 10 + 0x30;
		pVoiceCmd->cmd[len + 2] = ID % 10 + 0x30;
		
		voiceAdd.len[ID] += 2;
		voiceAdd.Total += 3;
	}
	else if(ID < 200)
	{
		pVoiceCmd->cmd[len + 1] = ID / 100 + 0x30;
		pVoiceCmd->cmd[len + 2] = (ID / 10) % 10 + 0x30;
		pVoiceCmd->cmd[len + 3] = ID  % 10 + 0x30;
		
		voiceAdd.len[ID] += 3;
		voiceAdd.Total += 4;
	}
	
	if(voiceAdd.num < ID)
	{
		voiceAdd.num = ID;
	}
	
	
	EEPROM_Write(VOICE_DATA_CMD_ADDR + ID*sizeof(VoiceCmd_t), 	//存储新指令
								voiceAdd.len[ID], (u8*)pVoiceCmd);
	
	returnValue = EEPROM_Write(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd); //存储修改后的存储标识
    if(returnValue == COMPLETE)
    {
        Thread_Login(ONCEDELAY, 0, 1500, Voice_CmdUpdata);	//延时更新语音指令Buf
    }
    return returnValue;
}
/*******************************************************************************
函 数 名:  	Voice_DataEarse
功能说明:  	语音数据清空
参    数:  	exist：前导是否存在，0：不存在，1：存在
返 回 值:  	消息执行结果
*******************************************************************************/
MsgResult_t Voice_DataEarse(u8 exist)
{
    VoiceSave_t voiceAdd = {0};
	u8 i;
	if(exist)
	{
		Thread_Login(ONCEDELAY, 0, 500, Voice_CmdUpdata);	//延时更新语音指令Buf
		EEPROM_Read(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);	//获取语音存储表
		
		voiceAdd.add[0] = 0x01;
		for(i=1;i<7;i++)
		{
			voiceAdd.add[i] = 0x00;
		}
		for(i=1;i<VOICE_CMD_NUM;i++)
		{
			voiceAdd.len[i] = 0;
		}
		voiceAdd.num = 1;
		voiceAdd.Total = voiceAdd.len[0];
	}
    return EEPROM_Write(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);
}

/*******************************************************************************
函 数 名:  	Voice_CmdDel
功能说明:  	语音指令删除
参    数:  	id:	语音指令码ID 			
返 回 值:  	TRUE/FALSE
*******************************************************************************/
MsgResult_t Voice_CmdDel(u8 id)
{
	u8 i;
    VoiceSave_t voiceAdd = {0};
	
	if (0 == id)
	{
		EEPROM_Read(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);	//获取语音存储表
		
		if (0 != (voiceAdd.add[id>>5]&(1<<(id&0x1F))))	//有存储数据
		{
			return Voice_DataEarse(1);
		}
		else
		{
			return Voice_DataEarse(0);
		}
	}
	Thread_Login(ONCEDELAY, 0, 1500, Voice_CmdUpdata);	//延时更新语音指令Buf
	EEPROM_Read(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd);	//获取语音存储表

	if (0 != (voiceAdd.add[id>>5]&(1<<(id&0x1F))))	//找到存储位置
	{
		voiceAdd.add[id>>5] &= ~(1<<(id&0x1F));
		
		voiceAdd.Total -= (u16)voiceAdd.len[id];
		voiceAdd.len[id] = 0;

		/*获取存在语音识别语句的最大数*/
		for(i=voiceAdd.num;i>0;i--)
		{
			if (0 != (voiceAdd.add[i>>5]&(1<<(i&0x1F))))	//有存储数据
			{
				voiceAdd.num = i;
				break;
			}
		}
		
		return EEPROM_Write(VOICE_DATA_START_ADDR, sizeof(VoiceSave_t), (u8*)&voiceAdd); //存储修改后的存储标识
	}
	else
	{
		return COMPLETE;
	}
}

/*******************************************************************************
函 数 名:  	Voice_Reset
功能说明:  	语音重置
参    数:  	无			
返 回 值:  	无
*******************************************************************************/
void Voice_Reset()
{

}

/*******************************************************************************
函 数 名:  	Voice_Reset_Cmd
功能说明:  	语音重置
参    数:  	无			
返 回 值:  	无
*******************************************************************************/
void Voice_Reset_Cmd(void)
{

}

/*******************************************************************************
函 数 名:  	Voice_Disable
功能说明:  	语音禁用
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void Voice_Disable(void)
{

}

/*******************************************************************************
函 数 名：	EnableLeadCMD
功能说明： 	前导标识，FALSE：需要说前导码，TRUE：开始语音识别
参	  数： 	无
返 回 值：	无
注 		意:		
*******************************************************************************/
void EnableLeadCMD(void)
{
	gVoiceState.LeadCmd = TRUE;
}


/*******************************************************************************
函 数 名：	DisableLeadCMD
功能说明： 	前导标识，FALSE：需要说前导码，TRUE：开始语音识别
参	  数： 	无
返 回 值：	无
注 		意:		
*******************************************************************************/
void DisableLeadCMD(void)
{
	gVoiceState.LeadCmd = FALSE;
}
/*******************************************************************************
函 数 名：	VoiceAndTouchModeFun
功能说明： 	工作在触摸激活模式下的处理
参	  数： 	无
返 回 值：	无
注 		意:		100ms调用一次
*******************************************************************************/
void WorkModeFun(void)
{
	static u8 currentStatus = 0;
	static u8 nextStatus = 0;
	u16 recognitionResult = 0;
	u8 temp[BUF_DATA_LEN] = {0};
	u8 len = 0;
	static u8 flag = 0;
	static u8 powerStatus = 1;
	
	if ((0 == GPIOx_Get(CHRG_PORT, CHRG_PIN)) && (0 == GPIOx_Get(STDBY_PORT, STDBY_PIN)))//电池供电		
	{
		if(0 == flag)
		{
			GPIOx_Set(POWER_LOCK_PORT, POWER_LOCK_PIN);
			Thread_Login(ONCEDELAY , 0 , (gVoice.DormancyTime-3) * 1000 , VoiceDormancy);
			flag = 1;
		}
		powerStatus = 1;
		gVoice.workMode = TOUCH;
	}
	else//外部电源供电
	{
		flag = 0;
		if(1 == powerStatus)
		{
			WT588D_Report(CHARGING);
			powerStatus = 0;
		}
	}
	
	if(ALWAYS_ACTIVATION == gVoice.workMode)
	{
		nextStatus = 2;
		EnableLeadCMD();
	}
	
	if(TRUE == gVoiceState.isWakeUp)
	{
		nextStatus = 0;
	}
	
	if(FALSE == gVoiceState.LeadCmd)//
	{
		nextStatus = 0;
		LED_GreenOff();
	}
	else
	{
		LED_GreenOn();
	}
	
	switch(currentStatus)
	{
		case 0: 		
						if(TRUE == gVoiceState.isWakeUp)
						{
							nextStatus = 1;
							EnableLeadCMD();
							gVoiceState.isWakeUp = FALSE;
						}
						break;
			
		case 1: 	//开始识别
						ClrItemFromBuf(IsRecognitionResult);//清除队列中 之前的识别结果
						
						WT588D_Report(PLEASE_CMD);
						Thread_Login(ONCEDELAY,0,50,StartRecognition);
						LED_RedOff();
						
						PowerDownCountReset();
						
						nextStatus = 2;
						break;
		
		case 2:		
						len = GetDataFromChain(temp,IsRecognitionResult);//从队列里查找语音识别结果，查到后从队列里删除
						if(len > 0)
						{
							if(2 == len)
							{
								recognitionResult = temp[0];
								recognitionResult <<= 8;
								recognitionResult += temp[1];
							}
							else if(4 == len)
							{
								recognitionResult = temp[2];
								recognitionResult <<= 8;
								recognitionResult += temp[3];
							}
							if(0 != recognitionResult)
							{
								SYN7318.recognitionResultToUpLoad = (u8)recognitionResult;
								PropEventFifo(1, 3, SRCEVENT , SYN7318.recognitionResultToUpLoad);
								Upload(1);
				
								WT588D_Report(IDENTIFY_COMPLETE);
                                if(TRUE == SYN7318.lowPowerProtect)
                                {
                                    LowPowerWarning(gVoiceState.cap);
                                    gVoiceState.isWakeUp = FALSE;
//                                    return ;
                                }
								PowerDownCountReset();
								recognitionResult = 0;
								
								if(TRUE == gVoiceState.LeadCmd)
								{
									StartRecognition();
								}
								else
								{
									nextStatus = 0;
								}
							}
						}
						
						break;
	
		default :break;			
	}
	
	if(TRUE == gVoiceState.RecognitionIsTimeOut)
	{
		gVoiceState.RecognitionIsTimeOut = FALSE;
		if(TRUE == gVoiceState.LeadCmd)
		{
			StartRecognition();
		}
	}
	
	currentStatus = nextStatus;
}

/*******************************************************************************
函 数 名:  	StartRecognition
功能说明:  	开始语音识别入队
参    数:  	无 			
返 回 值:  	无
*******************************************************************************/
void StartRecognition(void)
{
	u8 buf[5] = {0xfd,0x00,0x02,0x10,0x00};//
	u8 len = 5;
	buf[4] = 0x00;//词典编号为0
	if(SYN7318BufLen < SYN7318_BUFF_MAX_LEN)
	{
		SYN7318BufLen = SaveData_SYN7318(buf,len,IsStartRecognition);//开始识别指令入队	
	}
	
	gVoiceState.BeingRecognized = TRUE; //
	gVoiceState.RecognitionIsTimeOut = FALSE;//
}
/**************************Copyright BestFu 2014-05-14*************************/
