/******************************Copyright BestFu 2016-03-10*****************************
�ļ���: Lark7318Buf.c
˵��  : Lark7318���ݻ��崦��
����  : Keil uVision4 V5.0
�汾  : v1.0
��д  : xjx
����  : 2016.03.10
*****************************************************************************/
#include "VoiceRecognition.h"
#include<stdlib.h>

#include "BF_type.h"
#include "SYN7318Buf.h"

extern StructNode_t *SYN7318Buf;

/*******************************************************************
������:	SaveData_Lark7318
����	:�Ѵ�Lark7318���յ������ݺ���Ҫ���͸�Lark7318�����ݴ浽������
����	:	u8 *data:��Ҫ�洢���ݵ���ʼ��ַ
						u8 len	:�洢����
 DataType_e dataType:�������������
����ֵ:	�Ѿ���ŵ�����֡��
*******************************************************************/ 
u8 SaveData_SYN7318(u8 *data,u8 len,DataType_e dataType)
{
	StructNode_t *h=NULL,*tmp = NULL;
	u8 cnt = 0;
	u8 i = 0;
     
	h = SYN7318Buf;
	
	while(NULL != h->Next)//�ҵ�����β��
	{
		cnt++;
		h = h->Next;
	}
	//��ʱ,h->Next == NULL 
    
    if(len > BUF_DATA_LEN)//�洢����Խ�磬ֱ�ӷ���
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
				tmp->data[i] = data[i];//��������
			}
			
			cnt++;//�ɹ��洢һ֡���ݣ�������1
		}		
	}
	return cnt;
	
}

/*******************************************************************
������:	GetDataFromChain
����	:��˳��ӻ�����ȡ��һ֡����(FIFO:�ȴ����ȡ)
����	:	u8 *buf:ȡ�����ݵĴ�ŵ�ַ
			DataType_e dataType:��Ҫȡ������������
����ֵ:	ȡ�������ݵĳ���
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
				flag = 1;//���ҳɹ�
				break;
			}			
		}
        while(h->Next != NULL);//����datatype			
			
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
������:	ClrItemFromBuf
����	:���buf��Ӧ������
����	:	DataType_e dataType:��Ҫ�������������
����ֵ:	��
*******************************************************************/ 
void ClrItemFromBuf(DataType_e dataType)
{
	u8 data[BUF_DATA_LEN];
	while(GetDataFromChain(data,dataType) != 0);	
}


/* �����ڴ��������ṹ�� */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;			//����ָ����
	u8 xBlockData[configBlockSize];					//����������
} xBlockLink;

/* ˽�����ݶ��� */
static xBlockLink ucHeap[ configTOTAL_HEAP_NUM ];	//Ϊ��̬�ѷ���ռ�
static xBlockLink* xStart;							//����ͷָ�룬��¼�׽ڵ��ַ

/* ˽�к������� */
static void prvHeapInit( void );					//�ڴ�����ʼ������

/*******************************************************************************
������	:	void *memBlock_malloc(void)
����˵��: 	�ڴ����亯��
����	: 	void
����ֵ	:	���ط����ڴ����׵�ַ��û�з��䵽�ڴ淵��NULL
˵��	:  
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
	if(NULL != xStart)						//ÿ�η��������ͷ��ȡ�ڴ�
	{
		xplink = xStart;
		pvReturn = (void*)(&(xStart->xBlockData));
		xStart = xStart->pxNextFreeBlock;
		xplink->pxNextFreeBlock = NULL;
	}
	
	return ((void*)pvReturn);
}


/*******************************************************************************
������	:	void memBlock_free(void *pv)
����˵��: 	�ڴ���ͷź���
����	: 	��Ҫ�ͷ��ڴ����׵�ַ
����ֵ	:	��
˵��	:  
*******************************************************************************/
void memBlock_free(void *pv)
{
	u8 *puc = (u8*) pv;
	xBlockLink *pxLink = NULL;
	
	if(NULL != pv)							//�ж��ͷ��ڴ���Ƿ���Ч
	{
		puc -= offsetof(xBlockLink,xBlockData[0]);
		pxLink = (xBlockLink*)puc;
		pxLink->pxNextFreeBlock = xStart;
		xStart = pxLink;
	}
}

/*******************************************************************************
������	:	static void prvHeapInit( void )
����˵��: 	�ڴ���ʼ��
����	: 	void
����ֵ	:	��
˵��	:  
*******************************************************************************/
static void prvHeapInit( void )
{
	u32 count;
	xStart = &ucHeap[0];
	for(count = 0;count < configTOTAL_HEAP_NUM - 1;count ++)	//���ڴ���ʼ��Ϊδʹ�õ�����ṹ
	{
		ucHeap[count].pxNextFreeBlock = &ucHeap[count+1];
	}
	ucHeap[count].pxNextFreeBlock = NULL;						//
}
/******************************End of File************************************/

