/***************************Copyright BestFu 2016-01-28*************************
�ļ�:    MemBlockMalloc.h
˵��:    ??????????????????????
����:    Keil uVision V5.12.0.0
�汾:    v1.0
��д:    jay
����:    2016-01-28
�޸�:     ��
*******************************************************************************/
#ifndef  __MEMBLOCKMALLOC_H
#define  __MEMBLOCKMALLOC_H

/*****************************?????**************************************/
#define configTOTAL_HEAP_NUM        (32)			//�ڴ�������
#define configBlockSize				(sizeof(StructNode_t))			//�ⲿ��Ҫ����Ķ����ڴ��С����λBytes

/*********************************��������*************************************/
void *memBlock_malloc(void);
void memBlock_free(void *pv);

#endif
