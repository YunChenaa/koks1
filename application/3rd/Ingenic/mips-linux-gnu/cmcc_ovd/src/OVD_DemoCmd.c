#include <stdio.h>
#include "OVD_DemoCmd.h"
#include "OVD_OpenAPI.h"
#include "OVD_define.h"

#define ASCII_SPACE 0x20
#define TABLE_WIDTH 36
static unsigned char do_cmd_help(int argc, char *argv[]);

static CMD_LIST_HEAD(cmd_head);

static unsigned int cmd_id = 1;

static CmdObj cmd_help = {
    "help",          0, 0, NULL, do_cmd_help,          "Print help infomation",
};

static void simple_help(CmdObj *cmd)
{
    char buffer[1024];
    int nch;
    int size;
    char *start;
    size = 1024;
    start = buffer;
    if (cmd->param)
    {
        nch = snprintf(start, size, "%s %s", cmd->name, cmd->param);
    }
    else
    {
        nch = snprintf(start, size, "%s",cmd->name);
    }
    size -= nch;
    start += nch;
    while(nch < TABLE_WIDTH)
    {
        *start++ = ASCII_SPACE;
        nch++;
        size--;
    }
    nch = snprintf(start, size, "%s", cmd->usage);
    printf("%s\n", buffer);
}
static unsigned char do_cmd_help(int argc, char *argv[])
{
    CmdObj* cmd = NULL;

    printf("==================== 帮助信息 ====================\n");
    cmd_list_for_each_entry(cmd, &cmd_head, CmdObj, list)
    {
        if (cmd != NULL)
        {
            simple_help(cmd);
        }
    }
    return CMD_NO_ERR;
}

static void help_info(CmdObj *cmd)
{
    printf("usage:\n", cmd->name, cmd->usage);
    if (cmd->detail != NULL)
    {
        simple_help(cmd);
        printf("%s\n", cmd->detail);
    }
}

static unsigned char cmd_strcmp(const char* s1, const char* s2);


CmdErrType cmd_exec(char* cmd_msg)
{
    CmdObj* cmd;

    /* This section is used to dismantling input command string */
    char* argv[CMD_MAX_NUM + 1];
    unsigned int argn = 0;
    unsigned int argc = 0;
    unsigned char i = 0;
    int new_words = 1;

    while (cmd_msg[i] != 0)
    {
        if (i > CMD_MAX_LEN - 1)
        {
            return CMD_LEN_OUT;
        }
        else if (argn > CMD_MAX_NUM - 1)
        {
            return CMD_NUM_OUT;
        }

        if (cmd_msg[i] == ASCII_SPACE)
        {
            new_words = 1;
            cmd_msg[i] = 0;
        }
        else if (new_words == 1)
        {
            new_words = 0;
            argv[argn++] = &cmd_msg[i];
        }
        i++;
    }
    if (argn == 0)
    {
        return CMD_NO_CMD;
    }
    argc = argn; // add 1 because there is no space key value front of the first argv
    /* End of dismantling input command string */
    cmd_list_for_each_entry(cmd, &cmd_head, CmdObj, list)
    {
        if (cmd_strcmp(cmd->name, argv[0]) == 0)
        {
            if (cmd->param_num > argc - 1)
            {
                printf("参数错误\n");
                help_info(cmd);
                return CMD_PARAM_LITTLE;
            }
            else if (cmd->param_num < argc - 1)
            {
                printf("参数错误\n");
                help_info(cmd);
                return CMD_PARAM_EXCEED;
            }

            if (cmd->cmd_hdl(argc, argv) != 0)
            {
                printf("运行错误\n");
                return CMD_EXEC_ERR;
            }
            else
            {
                return CMD_NO_ERR;
            }
        }
    }
    do_cmd_help(0, NULL);

    return CMD_NO_CMD;
}

void cmd_init(CmdObj *cmd_array, int size)
{
    int i;
    for (i = 0; i<size; i++)
    {
        cmd_add(&cmd_array[i]);
    }
    cmd_add(&cmd_help);
}

void cmd_add(CmdObj* obj)
{
    obj->id = cmd_id ++;
    cmd_list_add_tail(&cmd_head, &obj->list);
}

void cmd_del(CmdObj* obj)
{
    cmd_list_delete(&obj->list);
}

unsigned char cmd_isexist(CmdObj* obj)
{
    unsigned char isexist = 0;
    CmdObj* cmd;

    cmd_list_for_each_entry(cmd, &cmd_head, CmdObj, list)
    {
        if (obj->id == cmd->id)
        {
            isexist = 1;
        }
    }

    return isexist;
}

unsigned int cmd_get_id(CmdObj* obj)
{
    return obj->id;
}

unsigned int cmd_num()
{
    cmd_list *head = &cmd_head;
    cmd_list *next = cmd_head.next;
    unsigned int len = 0;
    while(head != next)
    {
        len++;
        next = next->next;
    }
    return len;
}

unsigned char cmd_strcmp(const char* s1, const char* s2)
{
    unsigned int i = 0;

    while (s1[i] != 0 || s2[i] != 0)
    {
        if (((s1[i] == 0) && (s2[i] != 0)) || ((s1[i] != 0) && (s2[i] == 0)) || (s1[i] != s2[i]))
        {
            return 1;
        }
        i++;
    }

    return 0;
}