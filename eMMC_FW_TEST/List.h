#ifndef _LIST_H
#define _LIST_H
#include <TypeDef.h>

typedef struct _DOUBLE_LINK_NODE
{
	U16 blkNO_in_chip;
	struct _DOUBLE_LINK_NODE* prev;
	struct _DOUBLE_LINK_NODE* next;
}DOUBLE_LINK_NODE, *pDOUBLE_LINK_NODE;


#define ASSERT(condition) \
	if(!(condition)) \
{ \
	printf(" file %s line %d, %s\n", __FILE__, __LINE__); \
	while(1); \
}

#if 1
DOUBLE_LINK_NODE* create_double_link_node(int value);
void delete_all_double_link_node(DOUBLE_LINK_NODE** pDLinkNode);
DOUBLE_LINK_NODE* find_data_in_double_link(const DOUBLE_LINK_NODE* pDLinkNode, int block_num);
int insert_data_into_double_link(DOUBLE_LINK_NODE** ppDLinkNode, int block);
int delete_data_from_double_link(DOUBLE_LINK_NODE** ppDLinkNode, int block);
int count_number_in_double_link(const DOUBLE_LINK_NODE* pDLinkNode);
void print_double_link_node(const DOUBLE_LINK_NODE* pDLinkNode);
U16 locate_link_by_pos(const DOUBLE_LINK_NODE* pDLinkNode,U16 pos);
U16 pop_from_list_head(DOUBLE_LINK_NODE** ppDLinkNode);
#endif

#endif