/***************************Copyright BestFu 2014-05-14*************************
文 件:	VoicePlayAttr.c
说 明:	语音播报设备属性实现文件
编 辑:	Keil uVision4 V4.54.0.0
版 本:	v1.0
编 写:	liushengcai
日 期:	2015.03.11
修 改:	暂无
*******************************************************************************/
#include "VoicePlayAttr.h"
#include "sys.h"
#include "VoicePlayUnit.h"
#include "Lark7318.h"
#include "Thread.h"

#include "uart.h"	
#include "PWM.h"
#include "delay.h"

#include "WT588D.h"


extern RcvFrom7318_t RcvFrom7318tmp;

/*******************************************************************************
函数名:  void voice_play_Init()
说  明:  语音参数的初始化
参  数:  unitID-单元号
返回值:  无
*******************************************************************************/
void voice_play_Init(void)
{

}

/*******************************************************************************
函数名:  MsgResult_t Set_VoicePlay_Attr(UnitPara_t *pData)
说  明:  设置语音播报属性
参  数:   pData->unit: 	单元号,
			pData->cmd:	  	属性号
            pData->len:     为参数长度
            pData->data:	为参数值（0为关闭，非0为开启）
返回值:  	操作结果
*******************************************************************************/
MsgResult_t Set_VoicePlay_Attr(UnitPara_t *pData)
{	
	return NO_CMD;
}
/*******************************************************************************
函数名:  MsgResult_t Set_StopPlay_Attr(UnitPara_t *pData)
说  明:  停止语音播报属性
参  数:   pData->unit: 	单元号,
			pData->cmd:	  	属性号
            pData->len:     为参数长度
            pData->data:	为参数值（0为关闭，非0为开启）
返回值:  	操作结果
*******************************************************************************/
MsgResult_t Set_StopPlay_Attr(UnitPara_t *pData)
{	
	return NO_CMD;
}
/*******************************************************************************
函数名:  void VoiceSperkers(u8 speakers)
说  明:  设置发音人
参  数:  speakers：发音人号
返回值:  无
*******************************************************************************/
void VoiceSperkers(u8 speakers)
{
	gVoice.Speakers = speakers;
}

/*******************************************************************************
函数名:  MsgResult_t Set_Speakers_Attr(UnitPara_t *pData)
说  明:  设置发音人
参  数:   pData->unit: 	单元号,
			pData->cmd:	  	属性号
            pData->len:     为参数长度
            pData->data:	为参数值（0为关闭，非0为开启）
返回值:  	操作结果
*******************************************************************************/
MsgResult_t Set_Speakers_Attr(UnitPara_t *pData)
{

	if(pData->data[0] > 5)
	{
		return PARA_MEANING_ERR;
	}

	VoiceSperkers(pData->data[0]);
	Voice_SetSave();
	return NO_CMD;
}

/*******************************************************************************
函 数 名：  Get_VoiceManage_Attr  
功能说明：  获取当前语音指令
参	  数:    pData:   单元号/属性号/参数长度/参数内容
            *rlen-返回参数长度
            *rpara-返回参数存放地址         
返 回 值：  TRUE/FALSE
*******************************************************************************/
MsgResult_t Get_Speakers_Attr(UnitPara_t *pData, u8 *rlen, u8 *rpara)
{    
	*rlen = 1;
    *rpara = gVoice.Speakers;
	return NO_CMD;
}
/*******************************************************************************
函数名:  void VoicePlayState(u8 Volume,u8 Intonation,u8 Speed)
说  明:  设置音量、语调、语速
参  数:  
返回值:  无
*******************************************************************************/
void VoicePlayState(u8 Volume,u8 Intonation,u8 Speed)
{

	if(Volume < 8)
	{
		gVoice.Volume = Volume;		//音量
	}
	else
	{
		gVoice.Volume = MAX_VOLUME;
	}
	
	gVoice.Intonation 	= Intonation;	//语调
	gVoice.Speed 		= Speed;		//语速

}
/*******************************************************************************
函数名:  MsgResult_t Set_VoicePlayState_Attr(UnitPara_t *pData)
说  明:  设置音量、语调、语速属性
参  数:   pData->unit: 	单元号,
			pData->cmd:	  	属性号
            pData->len:     为参数长度
            pData->data:	为参数值（0为关闭，非0为开启）
返回值:  	操作结果
*******************************************************************************/
MsgResult_t Set_VoicePlayState_Attr(UnitPara_t *pData)
{
	if(pData->data[0] > 7)
	{
		return PARA_MEANING_ERR;
	}
	
	VoicePlayState(pData->data[0],pData->data[1],pData->data[2]);
	Voice_SetSave();
	
	WT588D_Report(ACT_OK);
	return COMPLETE;
}
/*******************************************************************************
函 数 名：  Get_VoiceManage_Attr  
功能说明：  获取当前语音指令
参	  数:    pData:   单元号/属性号/参数长度/参数内容
            *rlen-返回参数长度
            *rpara-返回参数存放地址         
返 回 值：  TRUE/FALSE
*******************************************************************************/
MsgResult_t Get_VoicePlayState_Attr(UnitPara_t *pData, u8 *rlen, u8 *rpara)
{    	
	*rlen = 1;
    rpara[0] = gVoice.Volume;			//音量

	return COMPLETE;  
}
/**************************Copyright BestFu 2014-05-14*************************/

