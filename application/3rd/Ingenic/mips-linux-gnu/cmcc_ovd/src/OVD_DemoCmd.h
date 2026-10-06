#ifndef _OVD_DEMO_CMD_H_
#define _OVD_DEMO_CMD_H_

#define CMD_MAX_LEN     256
#define CMD_MAX_NUM     20


/**
 * 双向链表结构体定义
 */
typedef struct cmd_list {
    struct cmd_list *prev; /**< 后继节点指针 */
    struct cmd_list *next; /**< 前向节点指针 */
} cmd_list;

#define CMD_LIST_HEAD(list) cmd_list list = { &(list), &(list) }

#define LIST_OFFSET_OF(type, member) ((char *)&((type *)0)->member)


#define cmd_list_entry(list, type, member) \
    ((type *)(void *)((char *)(list) - LIST_OFFSET_OF(type, member)))

#define cmd_list_for_each_entry(item, list, type, member)             \
    for (item = cmd_list_entry((list)->next, type, member);        \
         &(item)->member != (list);                                      \
         item = cmd_list_entry((item)->member.next, type, member))

static inline void cmd_list_add(cmd_list *list, cmd_list *node)
{
    node->next = list->next;
    node->prev = list;
    list->next->prev = node;
    list->next = node;
}

/**
 * @brief Insert a node to the tail of a doubly linked list.
 * @param list     [IN] Doubly linked list where the new node is inserted.
 * @param node     [IN] New node to be inserted.
 * @return
 */
static inline void cmd_list_add_tail(cmd_list *list, cmd_list *node)
{
    cmd_list_add(list->prev, node);
}

/**
 * @brief This API is used to delete a specified node from a doubly linked list
 * @param node    [IN] Node to be deleted.
 * @return
 */
static inline void cmd_list_delete(cmd_list *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
}

typedef enum cmd_err
{
    CMD_NO_ERR = 0,
    CMD_LEN_OUT,
    CMD_NUM_OUT,
    CMD_NO_CMD,
    CMD_PARAM_EXCEED,
    CMD_PARAM_LITTLE,
    CMD_EXEC_ERR
} CmdErrType;

typedef struct cmd_object
{
    const char* name;
    /* id auto increase from 1, if return id is 0 means that there is not target command */
    unsigned int id;
    /* parameter number, max 255 */
    unsigned char param_num;
    const char *param;
    unsigned char (*cmd_hdl)(int, char* []);
    const char* usage;
    const char* detail; // detail param info
    cmd_list list;
} CmdObj;

CmdErrType cmd_exec(char* cmd_msg);
void cmd_init(CmdObj *cmd_array, int size);
void cmd_add(CmdObj* obj);
void cmd_del(CmdObj* obj);
unsigned char cmd_isexist(CmdObj* obj);
unsigned int cmd_get_id(CmdObj* obj);
unsigned int cmd_num(void);

#endif // _OVD_DEMO_CMD_H_