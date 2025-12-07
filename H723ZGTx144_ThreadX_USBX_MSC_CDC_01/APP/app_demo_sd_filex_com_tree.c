/*
 * mytree.c
 *
 *  Created on: 2024/06/29, 17:00:37
 *      Author: phy1335
 *        Note: 打印指定目录的内容
 *        出处: https://forum.anfulai.cn/forum.php?mod=viewthread&tid=124708&highlight=%C4%BF%C2%BC%CA%F7
 */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "fx_api.h"
#include "string.h"
#include "app_filex.h"
#include "app_demo_sd_filex.h"
#ifdef PRINT_TOTAL_TIME
#include "GetRunTime.h"
#endif
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
// uint32_t current_search_index[MAX_TRAVEL_DEPTH] = {0}; // 对应不同深度的条目的索引
// uint64_t current_search_size[MAX_TRAVEL_DEPTH] = {0}; // 对应不同深度的条目的大小
// uint8_t current_is_last[MAX_TRAVEL_DEPTH] = {0};      // 对应不同深度的条目是否是最后一个
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief UTF-16LE转UTF-8
 * @param utf16 输入UTF-16LE字符串（以0x0000结尾）
 * @param utf8 输出UTF-8字符串缓冲区
 * @param utf8_len 输出缓冲区最大长度
 * @return 转换后的UTF-8字符串长度（不含终止符），-1表示溢出
 */
static int utf16le_to_utf8(const uint16_t *utf16, char *utf8, size_t utf8_len) {
    if (utf16 == NULL || utf8 == NULL || utf8_len == 0) return -1;
    size_t i = 0, j = 0;
    while (utf16[i] != 0x0000) {  // 遍历UTF-16字符串（以0结尾）
        uint32_t code_point;
        // 处理BMP（基本多文种平面）字符（U+0000~U+FFFF）
        if ((utf16[i] & 0xFC00) != 0xD800) {  // 非代理对
            code_point = utf16[i];
            i++;
        } else {
            // 处理代理对（U+10000~U+10FFFF），需两个UTF-16编码
            if ((utf16[i] & 0xFC00) != 0xD800 || utf16[i+1] == 0x0000) {
                return -1;  // 无效代理对
            }
            code_point = ((utf16[i] & 0x03FF) << 10) | (utf16[i+1] & 0x03FF);
            code_point += 0x10000;
            i += 2;
        }
        // 转换为UTF-8编码
        if (code_point <= 0x7F) {  // 1字节
            if (j + 1 >= utf8_len) return -1;
            utf8[j++] = (char)code_point;
        } else if (code_point <= 0x7FF) {  // 2字节
            if (j + 2 >= utf8_len) return -1;
            utf8[j++] = (char)(0xC0 | (code_point >> 6));
            utf8[j++] = (char)(0x80 | (code_point & 0x3F));
        } else if (code_point <= 0xFFFF) {  // 3字节
            if (j + 3 >= utf8_len) return -1;
            utf8[j++] = (char)(0xE0 | (code_point >> 12));
            utf8[j++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
            utf8[j++] = (char)(0x80 | (code_point & 0x3F));
        } else if (code_point <= 0x10FFFF) {  // 4字节
            if (j + 4 >= utf8_len) return -1;
            utf8[j++] = (char)(0xF0 | (code_point >> 18));
            utf8[j++] = (char)(0x80 | ((code_point >> 12) & 0x3F));
            utf8[j++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
            utf8[j++] = (char)(0x80 | (code_point & 0x3F));
        } else {
            return -1;  // 无效Unicode码点
        }
    }
    utf8[j] = '\0';  // 终止符
    return j;
}

#ifndef _USE_UTF8_
/**
 * @brief 打印树形结构的缩进和节点
 * @param entry_name char* 要打印的节点名称
 * @param depth uint8_t 当前节点的深度（即层级）
 * @param is_last uint8_t 指示在每一层中当前节点是否是该层的最后一个节点
 * @param option uint8_t 标识当前节点是否是系统或隐藏
 */
static void print_indent(const char *entry_name, uint8_t depth, uint8_t *is_last, uint8_t option)
{
    if (depth == 0) // 是根节点
    {
        if (is_last[depth]) // 当前节点是该层的最后一个节点
        {
            printf("\\-"); // 打印结束引线
        }
        else
        {
            printf("|-"); // 不是最后一个节点，打印中继引线
        }
    }
    else // 不是根节点
    {
        for (size_t i = 0; i < depth; i++) // 循环遍历到当前深度减1的所有层
        {
            if (is_last[i])
            {
                printf("    "); // 该节点前的层级都没东西了。打印空白
            }
            else
            {
                printf("|    "); // 该节点前的某个层级下面还有东西。打印竖线
            }
        }
        if (is_last[depth]) // 当前节点是该层的最后一个节点
        {
            printf("\\"); // 打印结束引线
        }
        else
        {
            printf("+"); // 不是最后一个节点，打印中继引线
        }
    }
    printf("-- %s %s %s\n", entry_name, (option & FX_SYSTEM) ? "^" : "", (option & FX_HIDDEN) ? "*" : ""); // 打印节点名称
}
#else
/**
 * @brief 打印树形结构的缩进和节点
 * @param entry_name char* 要打印的节点名称
 * @param depth uint8_t 当前节点的深度（即层级）
 * @param is_last uint8_t * 指示在每一层中当前节点是否是该层的最后一个节点
 * @param option uint8_t 标识当前节点是否是系统或隐藏
 * @note  注意使用UTF-8 否则自行修改
 */
static void print_indent(const char *entry_name, uint8_t depth, uint8_t *is_last, uint8_t option)
{
    if (depth == 0) // 是根节点
    {
        if (is_last[depth]) // 当前节点是该层的最后一个节点
        {
            printf("└"); // 打印结束引线
        }
        else
        {
            printf("├"); // 不是最后一个节点，打印中继引线
        }
    }
    else // 不是根节点
    {
        for (size_t i = 0; i < depth; i++) // 循环遍历到当前深度减1的所有层
        {
            if (is_last[i])
            {
                printf("    "); // 该节点前的层级都没东西了。打印空白
            }
            else
            {
                printf("│   "); // 该节点前的某个层级下面还有东西。打印竖线
            }
        }
        if (is_last[depth]) // 当前节点是该层的最后一个节点
        {
            printf("└"); // 打印结束引线
        }
        else
        {
            printf("├"); // 不是最后一个节点，打印中继引线
        }
    }
    /* 打印节点名称 */
    printf("── %s %s %s\n", entry_name, (option & FX_SYSTEM) ? "^" : "", (option & FX_HIDDEN) ? "*" : "");


}
#endif
/* Exported functions --------------------------------------------------------*/
/**
 * @brief 以树状图打印指定目录下的内容
 * @param path char * 要打印的指定目录
 * @param depth_max uint8_t 遍历的深度
 * @param tree_opt uint8_t 遍历控制 目前可选 SD_SHOW_HIDE
 * @param workspace_ptr void* 指向工作区的指针
 * @param workspace_size size_t 工作区的大小
 * @return 0成功;其他FileX错误
 */
uint16_t sd_com_tree(char *path, uint8_t depth_max, uint8_t tree_opt, void *workspace_ptr, size_t workspace_size)
{
    uint16_t status = 0;
    /* 条目计数 */
    uint32_t total_cnt = 0; // 总条目计数
    uint32_t file_cnt = 0;  // 文件计数
    uint32_t dir_cnt = 0;   // 目录计数
    uint32_t hide_cnt = 0;  // 隐藏内容计数
    uint32_t sys_cnt = 0;   // 系统内容计数

    /* 遍历参数 */
    uint8_t current_search_depth = 0;                 // 当前搜索深度
    uint8_t *current_is_last = NULL;                  // 对应不同深度的条目是否是最后一个
    uint32_t *current_search_index = NULL;            // 对应不同深度的条目的索引
    uint64_t *current_search_size = NULL;             // 对应不同深度的条目的大小
    char entry_name[FX_MAX_LONG_NAME_LEN] = {0};      // 找到的条目
    char entry_name_next[FX_MAX_LONG_NAME_LEN] = {0}; // 下一个找到的条目，用于判读是否是最后一个
    UINT attributes = 0;                              // 属性，判断文件还是目录 换成uint32_t CubeIDE会报警告
    char default_dir[FX_MAXIMUM_PATH] = {0};          // 默认工作路径。遍历完要恢复

    // 在sd_com_tree函数中定义缓冲区（放在entry_name附近）
    uint16_t unicode_name[FX_MAX_LONG_NAME_LEN * 2] = {0};  // 存储UTF-16文件名（预留足够空间）
    char utf8_name[FX_MAX_LONG_NAME_LEN * 4] = {0};         // 存储转换后的UTF-8文件名（1个UTF-16最多对应4个UTF-8字节）
    ULONG len;  // 用于接收UTF-16文件名长度

    /* 处理工作区 */
    if (workspace_ptr == NULL)
    {
        return (FX_PTR_ERROR);
    }
    /* 计算工作区的起始位置 */
    /* 8 32 位无需对齐 */
    uint8_t *plast = workspace_ptr;           // 由于8不需要特定对齐，直接从workspace_ptr的开始位置计算偏移
    uint8_t *plast_end = plast + depth_max;   // 后面需要预留depth_max个uint8_t的空间
    uint32_t *pindex = (uint32_t *)plast_end; // 32位无需对齐
    // uint32_t *pindex = (uint32_t *)(((uintptr_t)plast_end + 3) & ~3); // 需要4字节对齐，计算plast_end之后的下一个4字节对齐的地址
    uint32_t *pindex_end = pindex + depth_max; // pindex后面需要预留depth_max个uint32_t的空间
    /* 64位要对齐 */
    uint64_t *psize = (uint64_t *)(((uintptr_t)pindex_end + 7) & ~7); // psize需要8字节对齐，计算plast_end之后的下一个8字节对齐的地址
    uint64_t *psize_end = psize + depth_max;                          // psize后面需要预留depth_max个uint64_t的空间

    size_t used_size = (void *)(psize_end) - (void *)plast; // 计算大小
    if (used_size > workspace_size)
    {
        printf("Workspace size Required: %dBytes\nActually provided: %dBytes\n", used_size, workspace_size);
        return (FX_NOT_ENOUGH_MEMORY);
    }
    else
    {
        /* 连接工作区 */
        memset(workspace_ptr, 0, workspace_size);
        current_is_last = plast;
        current_search_index = pindex;
        current_search_size = psize;
    }

    /* 保存全局路径 */
    if (FX_SD_MEDIA->fx_media_id != FX_MEDIA_ID)
    {
        return (FX_MEDIA_NOT_OPEN);
    }
    else
    {
        if (FX_SD_MEDIA->fx_media_default_path.fx_path_string[0] == '\0')
        {
            default_dir[0] = '/';
            default_dir[1] = '\0';
        }
        else
        {
            strncpy(default_dir, FX_SD_MEDIA->fx_media_default_path.fx_path_string, FX_MAXIMUM_PATH);
        }
    }

    /* 设置搜索路径 */
    status = fx_directory_default_set(FX_SD_MEDIA, path);
    if (status != FX_SUCCESS)
    {
        printf("Set default dir failed.%s\n", "0x01");
        printf("status %#X\n", status);
        return status;
    }

    /* 打印表头 */
    printf("\n[%s]\n", path);

#ifdef PRINT_TOTAL_TIME
    uint32_t t1 = 0, t2 = 0;
    t1 = GetRunTime();
#endif

    /* 开始搜索 */
    while (1)
    {
        /* 根据当前深度的索引是否为0设置fx内部的索引值 */
        if (current_search_index[current_search_depth] == 0)
        {
            (FX_SD_MEDIA)->fx_media_default_path.fx_path_current_entry = 0;
        }
        else
        {
            /* 需要同时设置FileX内部的索引值和目录大小 */
            (FX_SD_MEDIA)->fx_media_default_path.fx_path_current_entry = current_search_index[current_search_depth];
            (FX_SD_MEDIA)->fx_media_default_path.fx_path_directory.fx_dir_entry_file_size = current_search_size[current_search_depth];
        }

        /* 根据当前是不是最后一个，继续查找*/
        if (current_is_last[current_search_depth] == 0)
        {
            /* 当前不是最后一个，继续查找 */
            status = fx_directory_next_full_entry_find(FX_SD_MEDIA, entry_name, &attributes,
                                                       FX_NULL, FX_NULL, FX_NULL, FX_NULL, FX_NULL, FX_NULL, FX_NULL);



            if (status != FX_SUCCESS) // 没找到
            {
                if (status == FX_NO_MORE_ENTRIES) // 没有更多了，但不应该在本次find进入这里
                {
                    printf("Why can you enter here?\n");
                    break;
                }
                else // 其他错误
                {
                    printf("Find error\n");
                    printf("status %#X\n", status);
                    break;
                }
            }
            else // 成功找到下一个
            {
            	// 在"成功找到下一个"的分支中（status == FX_SUCCESS后），添加以下代码：
            	// 获取UTF-16文件名
            	status = fx_unicode_name_get(FX_SD_MEDIA, entry_name, (UCHAR *)unicode_name, &len);
            	if (status != FX_SUCCESS) {
            	    // 如果获取失败，降级使用原始entry_name（可能仍乱码）
            	    strncpy(utf8_name, entry_name, sizeof(utf8_name) - 1);
            	} else {
            	    // 转换UTF-16LE到UTF-8
            	    if (utf16le_to_utf8(unicode_name, utf8_name, sizeof(utf8_name)) < 0) {
            	        // 转换失败，降级使用原始名称
            	        strncpy(utf8_name, entry_name, sizeof(utf8_name) - 1);
            	    }
#if DEBUG_UTF_8
            	    int utf8_len = utf16le_to_utf8(unicode_name, utf8_name, sizeof(utf8_name));
            	    printf("UTF-8 转换后字节：");
            	    for (int k = 0; k < utf8_len; k++) {
            	        printf("0x%02X ", (unsigned char)utf8_name[k]);
            	    }
            	    printf("\n");
#endif
            	}


                /* 找到一项，保存当前索引 */
                current_search_index[current_search_depth] = (FX_SD_MEDIA)->fx_media_default_path.fx_path_current_entry;
                current_search_size[current_search_depth] = (FX_SD_MEDIA)->fx_media_default_path.fx_path_directory.fx_dir_entry_file_size;

                /* 再找下一个看看是不是到头了 */
                status = fx_directory_next_entry_find(FX_SD_MEDIA, entry_name_next);
                if (status != FX_SUCCESS)
                {
                    if (status == FX_NO_MORE_ENTRIES) // entry_name是找到的是最后一个
                    {
                        /* 后面没有了，entry_name是最后一个 */
                        current_is_last[current_search_depth] = 1;
                    }
                    else // 其他错误
                    {
                        printf("Find next error\n");
                        printf("status %#X\n", status);
                        break;
                    }
                }
                else
                {
                    /* 后面还有，恢复到entry_name_next之前 */
                    (FX_SD_MEDIA)->fx_media_default_path.fx_path_current_entry = current_search_index[current_search_depth];
                    (FX_SD_MEDIA)->fx_media_default_path.fx_path_directory.fx_dir_entry_file_size = current_search_size[current_search_depth];
                }
            }
        }
        else
        {
            /* 深度为0，遍历完成了 */
            if (current_search_depth == 0)
            {
                break;
            }
            else
            {
                /* 返回上一层 */
                status = fx_directory_default_set(FX_SD_MEDIA, "..");
                if (status != FX_SUCCESS)
                {
                    printf("Set default dir failed.%s\n", "0x02");
                    printf("status %#X\n", status);
                    break;
                }
                else
                {
                    /* 清空本层索引参数 */
                    current_search_index[current_search_depth] = 0;
                    current_is_last[current_search_depth] = 0;
                    current_search_size[current_search_depth] = 0;
                    current_search_depth--; // 退回上层
                    continue;
                }
            }
        }

        /* 处理搜索到的东西 */
        /* 处理"."和".." */
        if ((strcmp(entry_name, ".") == 0) || (strcmp(entry_name, "..") == 0))
        {
            continue;
        }

        if (attributes & FX_VOLUME) // 是卷，跳过
        {
            continue;
        }

        /* 处理隐藏 */
        if (attributes & FX_HIDDEN)
        {
            /* 显示隐藏 */
            if (tree_opt & SD_SHOW_HIDE)
            {
                hide_cnt++;
            }
            else // 跳过
            {
                continue;
            }
        }

        /* 处理系统 */
        if (attributes & FX_HIDDEN)
        {
            sys_cnt++;
        }

        if (attributes & FX_DIRECTORY) // 是目录
        {
            dir_cnt++;
            total_cnt++;
//            print_indent(entry_name, current_search_depth, current_is_last, (attributes & FX_SYSTEM) | (attributes & FX_HIDDEN));
            // 打印目录时（attributes & FX_DIRECTORY分支）：
            print_indent(utf8_name, current_search_depth, current_is_last, (attributes & FX_SYSTEM) | (attributes & FX_HIDDEN));

            /* 达到设定的遍历深度了没? */
            if (current_search_depth >= depth_max - 1)
            {
                continue;
            }

            /* 进入此文件夹 */
            status = fx_directory_default_set(FX_SD_MEDIA, entry_name);
            if (FX_SUCCESS != status)
            {
                printf("Set default dir failed.%s\n", entry_name);
                printf("status %#X\n", status);
                break;
            }
            else
            {
                current_search_depth++; // 搜索深度+1
                /* 清空下一层索引参数 */
                current_search_index[current_search_depth] = 0;
                current_search_size[current_search_depth] = 0;
                current_is_last[current_search_depth] = 0;
                continue;
            }
        }
        else // 是文件
        {
            file_cnt++;
            total_cnt++;
//            print_indent(entry_name, current_search_depth, current_is_last, (attributes & FX_SYSTEM) | (attributes & FX_HIDDEN));
            // 打印文件时（else分支）：
            print_indent(utf8_name, current_search_depth, current_is_last, (attributes & FX_SYSTEM) | (attributes & FX_HIDDEN));
        }
    }

    printf("Total:%ld    Dir:%ld    File:%ld\n", total_cnt, dir_cnt, file_cnt);
    if (tree_opt & SD_SHOW_HIDE)
    {
        printf("Hidden:%ld\n", hide_cnt);
    }
    if (sys_cnt)
    {
        printf("System:%ld\n", sys_cnt);
    }

#ifdef PRINT_TOTAL_TIME
    t2 = GetRunTime();
    printf("===Total Time %ld ms\n", t2 - t1);
#endif

    /* 恢复原来参数 */
    /* 恢复全局路径 */
    status = fx_directory_default_set(FX_SD_MEDIA, path);
    if (status != FX_SUCCESS)
    {
        printf("Set default dir failed.%s\n", default_dir);
        printf("status %#X\n", status);
    }
    /* 恢复索引条目 */
    (FX_SD_MEDIA)->fx_media_default_path.fx_path_current_entry = 0;

    return status;
}


