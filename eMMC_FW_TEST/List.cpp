/**********************************************************
Description	:		List.cpp
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
#include "stdafx.h"
#include "List.h"
#include "PhyManage.h"
#include<malloc.h>
#include<stdlib.h>
#include"stdio.h"
#include <TypeDef.h>

/**********************************************************************
Function: 
Description: 
Input: 
Output: 
Return: 
Others: 
Modify Date:
Author :
************************************************************************/
#if 1
DOUBLE_LINK_NODE* create_double_link_node(int value)
{
    DOUBLE_LINK_NODE* pDLinkNode = NULL;
    pDLinkNode = (DOUBLE_LINK_NODE*)malloc(sizeof(DOUBLE_LINK_NODE));
    ASSERT(NULL != pDLinkNode);

    memset(pDLinkNode, 0, sizeof(DOUBLE_LINK_NODE));
    pDLinkNode->blkNO_in_chip = value;
	pDLinkNode->next = NULL;
	pDLinkNode->prev = NULL;
    return pDLinkNode;
}

void delete_all_double_link_node(DOUBLE_LINK_NODE** pDLinkNode)
{
    DOUBLE_LINK_NODE* pNode;
    if(NULL == *pDLinkNode)
        return ;

    pNode = *pDLinkNode;
    *pDLinkNode = pNode->next;
    free(pNode);
    pNode = NULL;
    delete_all_double_link_node(pDLinkNode);
}

DOUBLE_LINK_NODE* find_data_in_double_link(const DOUBLE_LINK_NODE* pDLinkNode, int block_num)
{
    DOUBLE_LINK_NODE* pNode = NULL;
    if(NULL == pDLinkNode)
        return NULL;

    pNode = (DOUBLE_LINK_NODE*)pDLinkNode;
    while(NULL != pNode){
        if(block_num == pNode->blkNO_in_chip)
            return pNode;
        pNode = pNode ->next;
    }
    return NULL;
}

int insert_data_into_double_link(DOUBLE_LINK_NODE** ppDLinkNode, int block)
{
    DOUBLE_LINK_NODE* pNode;
    DOUBLE_LINK_NODE* pIndex;

    if(NULL == ppDLinkNode)
        return FALSE;

    if(NULL == *ppDLinkNode){
        pNode = create_double_link_node(block);
        ASSERT(NULL != pNode);
        *ppDLinkNode = pNode;
        (*ppDLinkNode)->prev = (*ppDLinkNode)->next = NULL;
        return TRUE;
    }

    if(NULL != find_data_in_double_link(*ppDLinkNode, block))
        return FALSE;

    pNode = create_double_link_node(block);
    ASSERT(NULL != pNode);

    pIndex = *ppDLinkNode;
    while(NULL != pIndex->next)
        pIndex = pIndex->next;

    pNode->prev = pIndex;
    pNode->next = pIndex->next;
    pIndex->next = pNode;
    return TRUE;
}

int delete_data_from_double_link(DOUBLE_LINK_NODE** ppDLinkNode, int block)
{
    DOUBLE_LINK_NODE* pNode;
    if(NULL == ppDLinkNode || NULL == *ppDLinkNode)
        return FALSE;

    pNode = find_data_in_double_link(*ppDLinkNode, block);
    if(NULL == pNode)
        return FALSE;

    if(pNode == *ppDLinkNode){
        if(NULL == (*ppDLinkNode)->next){
            *ppDLinkNode = NULL;
        }else{
            *ppDLinkNode = pNode->next;
            (*ppDLinkNode)->prev = NULL;
        }

    }else{
        if(pNode->next)
            pNode->next->prev = pNode->prev;
        pNode->prev->next = pNode->next;
    }

    free(pNode);
    pNode = NULL;
    return TRUE;
}

U16 pop_from_list_head(DOUBLE_LINK_NODE** ppDLinkNode)
{
	U16 block;
	DOUBLE_LINK_NODE* pNode;
    if(NULL == *ppDLinkNode)
        return INVALID_U16;

    pNode = *ppDLinkNode;
    *ppDLinkNode = pNode->next;
	block = pNode->blkNO_in_chip;
	free(pNode);
	pNode = NULL;
	
    return block;
}


/*node num*/
int count_number_in_double_link(const DOUBLE_LINK_NODE* pDLinkNode)
{
    int count = 0;
    DOUBLE_LINK_NODE* pNode = (DOUBLE_LINK_NODE*)pDLinkNode;

    while(NULL != pNode){
        count ++;
        pNode = pNode->next;
    }
    return count;
}

/*Browse link*/
//DOUBLE_LINK_NODE * locate_link_by_pos(const DOUBLE_LINK_NODE* pDLinkNode,int pos)
#if 0 
U16 locate_link_by_pos(const DOUBLE_LINK_NODE* pDLinkNode,int pos)
{
    U16 num = 0;
    DOUBLE_LINK_NODE* pNode;
    pNode = pDLinkNode->next;
    
    ASSERT(pDLinkNode != NULL);
    ASSERT(pos <= count_number_in_double_link(pDLinkNode));
    
    while(((num ++) != pos) && (pNode != NULL)){
        pNode = pNode->next;
    }
    //return pNode;
    return pNode->blkNO_in_chip;
}
#endif

U16 locate_link_by_pos(const DOUBLE_LINK_NODE* pDLinkNode,U16 pos)
{
    U16 num = 0;
    DOUBLE_LINK_NODE* pNode = (DOUBLE_LINK_NODE*)pDLinkNode;
    while(NULL != pNode)
    {
        num++;
        if(num == pos)
        {
			return pNode->blkNO_in_chip;
        }
        pNode = pNode->next;
    }

    return INVALID_U16;
}

/*print list*/
void print_double_link_node(const DOUBLE_LINK_NODE* pDLinkNode)
{
    DOUBLE_LINK_NODE* pNode = (DOUBLE_LINK_NODE*)pDLinkNode;

    while(NULL != pNode){
        printf("%d\n", pNode->blkNO_in_chip);
        pNode = pNode ->next;
    }
}

#endif  



