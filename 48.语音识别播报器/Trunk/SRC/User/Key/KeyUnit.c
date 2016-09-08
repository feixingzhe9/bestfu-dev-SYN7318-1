/***************************Copyright BestFu 2014-05-14*************************
文	件：	KeyUnit.c
说	明：	按键单元功能函数
编	译：	Keil uVision4 V4.54.0.0
版	本：	v2.0
编	写：	Unarty
日	期：	2014.08.28
修　改：	暂无
*******************************************************************************/
#include "KeyUnit.h"
#include "Key.h"
#include "Thread.h"
#include "VoiceRecognition.h"
#include "PWM.h"
#include "delay.h"
#include "uart.h"	

#include "PowerStatus.h"

#include "LED.h"
#include "GPIO.h"
#include "Lark7318DataAnalyze.h"
#include "WT588D.h"

/*******************************************************************************
函 数 名：	KeyUnit_Init
功能说明： 	按键单元初始化
参	  数： 	无
返 回 值：	无
*******************************************************************************/
void KeyUnit_Init(void)
{
	Key_Init();
	Thread_Login(FOREVER, 0, 20, &Key_process);
}
/*******************************************************************************
函 数 名：	key_Process
功能说明： 	按键处理
参	  数： 	无
返 回 值：	无
*******************************************************************************/
void Key_process(void)
{
	u8 val;
	Power_T powerStatus;
	if ((val = Key_Scan()) != 0)	//有按键事件
	{
		if((LEAD_KEY|KEY_SHORT_EVENT/*KEY_LONG_EVENT*/) == (val & (LEAD_KEY|KEY_SHORT_EVENT/*KEY_LONG_EVENT*/)))
		{
			//短按事件
            if(TRUE == SYN7318.lowPowerProtect)
            {
                LowPowerWarning(gVoiceState.cap);
                gVoiceState.isWakeUp = FALSE;
                return ;
            }
			gVoiceState.isWakeUp = TRUE;
            			
		}
		
		if((LEAD_KEY|KEY_LONG_EVENT) == (val & (LEAD_KEY|KEY_LONG_EVENT/*KEY_SHORT_EVENT*/)))
		{
            
            if(TRUE == SYN7318.lowPowerProtect)
            {
                LowPowerWarning(gVoiceState.cap);
                gVoiceState.isWakeUp = FALSE;
                return ;
            }
			//长按事件
			powerStatus = PowerStatus_Get();
			
			if(EXTERN_POWER == powerStatus)
			{				
				switch(gVoice.workMode)//模式切换
				{
					case ALWAYS_ACTIVATION:
									DisableLeadCMD();
									gVoice.workMode = VOICE_TOUCH;
									WT588D_Report(MODE_VOICEandTOUCH_LEAD);
									Thread_Login(ONCEDELAY,0,550,StartRecognition);
									break;
					
					case VOICE_TOUCH:
									DisableLeadCMD();
									gVoice.workMode = TOUCH;
									WT588D_Report(MODE_TOUCH_LEAD);
									break;
					
					case TOUCH:
									EnableLeadCMD();
									gVoice.workMode = ALWAYS_ACTIVATION;
									gVoiceState.isWakeUp = TRUE;
									WT588D_Report(NO_LEAD);
									Thread_Login(ONCEDELAY,0,550,StartRecognition);
									break;
					
					default :		break;
				}
				
				Voice_SetSave();
						
			}
			else
			{
				gVoice.workMode = TOUCH;
			}
		}				
	}	
}





/**************************Copyright BestFu 2014-05-14*************************/	
