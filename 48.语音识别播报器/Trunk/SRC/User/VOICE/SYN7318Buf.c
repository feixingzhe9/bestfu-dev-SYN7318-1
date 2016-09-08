/******************************Copyright BestFu 2016-03-10*****************************
文件名: Lark7318Buf.c
说明  : Lark7318数据缓冲处理
编译  : Keil uVision4 V5.0
版本  : v1.0
编写  : xjx
日期  : 2016.03.10
*****************************************************************************/
#include "VoiceRecognition.h"
#include<stdlib.h>

#include "BF_type.h"
#include "SYN7318Buf.h"

extern StructNode_t *SYN7318Buf;

/*******************************************************************
函数名:	SaveData_Lark7318
描述	:把从Lark7318接收到的数据和需要发送给Lark7318的数据存到缓冲区
参数	:	u8 *data:需要存储数据的起始地址
						u8 len	:存储长度
 DataType_e dataType:存入的数据类型
返回值:	已经存放的数据帧数
*******************************************************************/ 
u8 SaveData_SYN7318(u8 *data,u8 len,DataType_e dataType)
{
	StructNode_t *h=NULL,*tmp = NULL;
	u8 cnt = 0;
	u8 i = 0;
     
	h = SYN7318Buf;
	
	while(NULL != h->Next)//找到链表尾部
	{
		cnt++;
		h = h->Next;
	}
	//此时,h->Next == NULL 
    
    if(len > BUF_DATA_LEN)//存储长度越界，直接返回
    {
        return cnt;
    }
    
	if(len < BUF_DATA_LEN)
	{
		tmp = (StructNode_t *)memBlock_malloc();
		if(NULL != tmp)
		{
			h->Next = tmp;
			tmp->Next = NULL;
			tmp->len = len;
			tmp->dataType = dataType;
			
			for(i=0; i<len; i++)
			{
				tmp->data[i] = data[i];//保存数据
			}
			
			cnt++;//成功存储一帧数据，计数加1
		}		
	}
	return cnt;
	
}

/*******************************************************************
函数名:	GetDataFromChain
描述	:按顺序从缓存中取出一帧数据(FIFO:先存的先取)
参数	:	u8 *buf:取出数据的存放地址
			DataType_e dataType:需要取出的数据类型
返回值:	取出的数据的长度
*******************************************************************/ 
u8 GetDataFromChain(u8 *buf,DataType_e dataType)
{
	StructNode_t *h=NULL,*p = NULL;
	u8 num = 0;
	u8 i = 0;
	u8 flag = 0;//?????
	
	if(SYN7318BufLen > 0)
	{	 
        h = SYN7318Buf;
        p = SYN7318Buf;
        
        do
		{		
			p = h;	
			h = h->Next;   
			if(dataType == h->dataType)
			{
				flag = 1;//查找成功
				break;
			}			
		}
        while(h->Next != NULL);//查找datatype			
			
		if(1 == flag)
		{          
			num = h->len;
			if(0 != num)
			{
				for(i=0; (i<num)&&(num<BUF_DATA_LEN); i++)
				{
					buf[i] =  h->data[i];//* = ;= i
				}
                    
				p->Next = h->Next;
                
				memBlock_free(h);
           
				SYN7318BufLen--;
               
				return num;
			}		
		}	
	}
	return 0;	
}

/*******************************************************************
函数名:	ClrItemFromBuf
描述	:清除buf对应的数据
参数	:	DataType_e dataType:需要清除的数据类型
返回值:	无
*******************************************************************/ 
void ClrItemFromBuf(DataType_e dataType)
{
	u8 data[BUF_DATA_LEN];
	while(GetDataFromChain(data,dataType) != 0);	
}


/* 定义内存分配链表结构体 */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;			//链表指针域
	u8 xBlockData[configBlockSize];					//链表数据域
} xBlockLink;

/* 私有数据定义 */
static xBlockLink ucHeap[ configTOTAL_HEAP_NUM ];	//为静态堆分配空间
static xBlockLink* xStart;							//定义头指针，记录首节点地址

/* 私有函数声明 */
static void prvHeapInit( void );					//内存分配初始化函数

/*******************************************************************************
函数名	:	void *memBlock_malloc(void)
函数说明: 	内存块分配函数
参数	: 	void
返回值	:	返回分配内存块的首地址，没有分配到内存返回NULL
说明	:  
*******************************************************************************/
void *memBlock_malloc(void)
{
	void *pvReturn = NULL;
	xBlockLink* xplink = NULL;
	static u8 xHeapHasBeenInitialised = 0u;
	
	if( xHeapHasBeenInitialised == 0u )
	{
		prvHeapInit();
		xHeapHasBeenInitialised = 1u;
	}
	if(NULL != xStart)						//每次分配从链表头部取内存
	{
		xplink = xStart;
		pvReturn = (void*)(&(xStart->xBlockData));
		xStart = xStart->pxNextFreeBlock;
		xplink->pxNextFreeBlock = NULL;
	}
	
	return ((void*)pvReturn);
}


/*******************************************************************************
函数名	:	void memBlock_free(void *pv)
函数说明: 	内存块释放函数
参数	: 	需要释放内存块的首地址
返回值	:	无
说明	:  
*******************************************************************************/
void memBlock_free(void *pv)
{
	u8 *puc = (u8*) pv;
	xBlockLink *pxLink = NULL;
	
	if(NULL != pv)							//判断释放内存块是否有效
	{
		puc -= offsetof(xBlockLink,xBlockData[0]);
		pxLink = (xBlockLink*)puc;
		pxLink->pxNextFreeBlock = xStart;
		xStart = pxLink;
	}
}

/*******************************************************************************
函数名	:	static void prvHeapInit( void )
函数说明: 	内存块初始化
参数	: 	void
返回值	:	无
说明	:  
*******************************************************************************/
static void prvHeapInit( void )
{
	u32 count;
	xStart = &ucHeap[0];
	for(count = 0;count < configTOTAL_HEAP_NUM - 1;count ++)	//将内存块初始化为未使用的链表结构
	{
		ucHeap[count].pxNextFreeBlock = &ucHeap[count+1];
	}
	ucHeap[count].pxNextFreeBlock = NULL;						//
}
/******************************End of File************************************/

