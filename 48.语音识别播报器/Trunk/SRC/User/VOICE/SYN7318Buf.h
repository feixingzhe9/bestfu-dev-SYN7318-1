/***************************Copyright BestFu 2016-01-28*************************
文件:    MemBlockMalloc.h
说明:    ??????????????????????
编译:    Keil uVision V5.12.0.0
版本:    v1.0
编写:    jay
日期:    2016-01-28
修改:     无
*******************************************************************************/
#ifndef  __MEMBLOCKMALLOC_H
#define  __MEMBLOCKMALLOC_H

/*****************************?????**************************************/
#define configTOTAL_HEAP_NUM        (32)			//内存块的数量
#define configBlockSize				(sizeof(StructNode_t))			//外部需要申请的定长内存大小，单位Bytes

/*********************************函数声明*************************************/
void *memBlock_malloc(void);
void memBlock_free(void *pv);

#endif
