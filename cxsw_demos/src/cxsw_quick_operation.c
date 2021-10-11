/**
 * @file cxsw_quick_operation.c
 * @author Feng
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "cxsw_quick_operation.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef void (*QuickOperationEventCb)(void *para);

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void QuickOperationInit(void);
static void QuickOperationResetInit(void);
static void QuickOperationclose(void);
static int CustomizeFileCopy(char *SourcePath, char *TargetPath);
static void DeleteMsgEventEb(lv_event_t * e);
static void BackgroundEventEb(lv_event_t * e);
static void ListEventEb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
static const char list_label[][10] = {OPERATION_LIST_COPY, OPERATION_LIST_PASTE, OPERATION_LIST_DELETE};
static const char* del_btn_str[] = {OPERATION_DEL_BTN_YES, OPERATION_DEL_BTN_CANCEL, "" };

static lv_style_t style_quick_operation;
static lv_style_t style_quick_operation_background;
static QuickOperationEventCb TempEventCb = NULL;
static char *QuickOperationSavePath = NULL;
static char *QuickOperationPath = NULL;
static lv_obj_t *Operation = NULL;
static lv_obj_t *Operation_background = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
/***********************************************
 *   Name：QuickOperationInit
 * Depict：文件管理器初始化
 ***********************************************/
static void QuickOperationInit(void)
{
    // 设置快捷操作样式
    lv_style_init(&style_quick_operation);
    // lv_style_set_pad_top(&style_quick_operation, 0);
    // lv_style_set_pad_bottom(&style_quick_operation, 0);
    lv_style_set_pad_left(&style_quick_operation, 0);
    lv_style_set_pad_right(&style_quick_operation, 0);
    lv_style_set_pad_row(&style_quick_operation, 0);
    lv_style_set_pad_column(&style_quick_operation, 0);
    lv_style_set_text_font(&style_quick_operation, &lv_font_montserrat_22);

    lv_style_init(&style_quick_operation_background);
    lv_style_set_radius(&style_quick_operation_background, 0);
    lv_style_set_opa(&style_quick_operation_background, 0);
}

/***********************************************
 *   Name：QuickOperationResetInit
 * Depict：文件管理器复位初始化
 ***********************************************/
static void QuickOperationResetInit(void)
{
    // 复位快捷操作样式
    lv_style_reset(&style_quick_operation);
    lv_style_reset(&style_quick_operation_background);
}

/***********************************************
 *   Name：QuickOperationResetInit
 * Depict：文件管理器复位初始化
 ***********************************************/
static void QuickOperationclose(void)
{
    // 关闭快捷操作
    QuickOperationResetInit();
    lv_obj_clean(Operation);
    lv_obj_del(Operation);
    lv_obj_clean(Operation_background);
    lv_obj_del(Operation_background);
}

/***********************************************
 *   Name：CustomizeFileCopy
 * Depict：自定义文件复制
 ***********************************************/
static int CustomizeFileCopy(char *SourcePath, char *TargetPath)
{
    FILE *fpRead;
    FILE *fpWrite;

    int readCount;
    int bufferLen = 1024*4;
    char *buffer = (char*)lv_mem_alloc(bufferLen);
    memset(buffer, 0, bufferLen);
    
    if( (fpRead = fopen(SourcePath, "rb")) == NULL || (fpWrite = fopen(TargetPath, "wb")) == NULL )
    {
        LV_LOG_WARN("Cannot open file, press any key to exit!\n");
        return -1;
    }

    while( (readCount=fread(buffer, 1, bufferLen, fpRead)) > 0 )
    {
        fwrite(buffer, readCount, 1, fpWrite);
    }

    lv_mem_free(buffer);
    fclose(fpRead);
    fclose(fpWrite);
    return 0;
}

/***********************************************
 *   Name：ConfirmDeleteEventEb
 * Depict：确认删除事件响应
 ***********************************************/
static void DeleteMsgEventEb(lv_event_t * e)
{
    char *RecvPath = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *msgbox = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        LV_LOG_USER("Delete Msg Button %s clicked", lv_msgbox_get_active_btn_text(msgbox));
        // 查找相应按键
        for (int i = 0; i < (sizeof(del_btn_str)/sizeof(del_btn_str[0])); i++)
        {
            if (strcmp(del_btn_str[i], lv_msgbox_get_active_btn_text(msgbox))) continue;
            switch (i)
            {
                case 0:
                {
                    int error = remove(RecvPath);
                    if (error)
                    {
                        LV_LOG_WARN("File delete fail, error code %s", error);
                    }
                    else
                    {
                        LV_LOG_USER("yes delete '%s' success", RecvPath);
                    }
                    break;
                }
                case 1:
                {
                    LV_LOG_USER("Cancel");
                    break;
                }
                default:
                {
                    LV_LOG_USER("Invalid operation");
                    break;
                }
            }
            break;
        }
        if (RecvPath != NULL)
        {
            lv_mem_free(RecvPath);
            RecvPath = NULL;
        }
        lv_obj_clean(msgbox);
        lv_obj_del(msgbox);
        TempEventCb(NULL);
    }
}

/***********************************************
 *   Name：BackgroundEventEb
 * Depict：背景点击事件响应
 ***********************************************/
static void BackgroundEventEb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_SHORT_CLICKED) 
    {
        if (QuickOperationPath != NULL)
        {
            lv_mem_free(QuickOperationPath);
            QuickOperationPath = NULL;
        }
        QuickOperationclose();
    }
}

/***********************************************
 *   Name：FileEventEb
 * Depict：文件按键事件响应
 ***********************************************/
static void ListEventEb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *parent = lv_event_get_user_data(e);

    bool IsCatalogFile = false;
    if (code == LV_EVENT_SHORT_CLICKED) 
    {
        struct stat stat_info; 
        int ret = lstat(QuickOperationPath, &stat_info);
        if (ret == 0)
        {
            if ((stat_info.st_mode & S_IFMT) == S_IFDIR)
            {
                IsCatalogFile = true;
            }

            // 查找相应按键
            for (int i = 0; i < (sizeof(list_label)/sizeof(list_label[0])); i++)
            {
                if (strcmp(list_label[i], lv_list_get_btn_text(Operation, btn))) continue;
                switch (i)
                {
                    case 0:
                    {
                        // 不可以复制文件夹
                        if (IsCatalogFile)
                        {
                            LvCreateWarnPopup("Can not copy folders");
                            break;
                        }
                        LV_LOG_USER("list button copy, path %s", QuickOperationPath);
                        if (QuickOperationSavePath != NULL)
                        {
                            lv_mem_free(QuickOperationSavePath);
                            QuickOperationSavePath = NULL;
                        }
                        int path_len = strlen(QuickOperationPath);
                        QuickOperationSavePath = lv_mem_alloc(path_len + 1);
                        memset(QuickOperationSavePath, 0, path_len + 1);
                        sprintf(QuickOperationSavePath, "%s", QuickOperationPath);
                        break;
                    }
                    case 1:
                    {
                        if (QuickOperationSavePath != NULL)
                        {
                            char *source_file_name = strrchr(QuickOperationSavePath, '/') + 1;
                            LV_LOG_USER("source_file_name '%s'", source_file_name);

                            int source_path_len = strlen(QuickOperationPath);
                            char copy_source_path[source_path_len + 1];
                            sprintf(copy_source_path, "%s", QuickOperationPath);

                            char *p = strrchr(copy_source_path, '/');
                            *p = '\0';
                            LV_LOG_USER("target_file_path '%s'", copy_source_path);

                            int new_path_len = strlen(copy_source_path) + strlen(source_file_name);
                            char new_path[new_path_len + 2];
                            sprintf(new_path, "%s/%s", copy_source_path, source_file_name);
                            LV_LOG_USER("new_file_path %s", new_path);

                            // 判断目标路径是否有同名文件
                            if ((access(new_path, F_OK)) == 0)   
                            {
                                LV_LOG_WARN("file '%s' exist", new_path);
                                LvCreateWarnPopup("file exist");
                            }
                            else
                            {
                                if (CustomizeFileCopy(QuickOperationSavePath, new_path))
                                {
                                    LV_LOG_WARN("File copy fail");
                                }
                                else
                                {
                                    if (QuickOperationSavePath != NULL)
                                    {
                                        lv_mem_free(QuickOperationSavePath);
                                        QuickOperationSavePath = NULL;
                                    }
                                }
                            }
                        }
                        else
                        {
                            LV_LOG_WARN("No paste file");
                            LvCreateWarnPopup("No paste file");
                        }
                        break;
                    }
                    case 2:
                    {
                        // 不可以删除文件夹
                        if (IsCatalogFile)
                        {
                            LvCreateWarnPopup("Can't delete folder");
                            break;
                        }
                        char *file_path = strrchr(QuickOperationPath, '/') + 1;
                        char msg_str[sizeof(OPERATION_DEL_MSG_TXT) + strlen(file_path) + 10];
                        sprintf(msg_str, "%s '%s'", OPERATION_DEL_MSG_TXT, file_path);
                        LV_LOG_USER("list button delete, %s", msg_str);

                        int path_len = strlen(QuickOperationPath);
                        char *SendPath = (char *)lv_mem_alloc(path_len + 1);
                        memset(SendPath, 0, path_len + 1);
                        sprintf(SendPath, "%s", QuickOperationPath);

                        lv_obj_t *msgbox_delete = lv_msgbox_create(parent, OPERATION_DEL_MSG_TITLE, msg_str, del_btn_str, false);
                        lv_obj_align(msgbox_delete, LV_ALIGN_CENTER, 0, 0);
                        lv_obj_add_event_cb(msgbox_delete, &DeleteMsgEventEb, LV_EVENT_VALUE_CHANGED, SendPath);
                        lv_obj_center(msgbox_delete);
                        break;
                    }
                    default:
                    {
                        LV_LOG_USER("Invalid operation, path %s", QuickOperationPath);
                        break;
                    }
                }
                break;
            }
        }
        else
        {
            LV_LOG_WARN("%s get info error !", QuickOperationPath);
        }

        if (QuickOperationPath != NULL)
        {
            lv_mem_free(QuickOperationPath);
            QuickOperationPath = NULL;
        }
        QuickOperationclose();
        TempEventCb(NULL);
    }
}

/***********************************************
 *   Name：LvCreateQuickOperation
 * Depict：创建快捷操作
 ***********************************************/
lv_obj_t * LvCreateQuickOperation(char *path, void (*event_cb)(void *para))
{
    if (path == NULL)
    {
        LV_LOG_WARN("Invalid path");
        return NULL;
    }
    TempEventCb = event_cb;

    // 如果存在快捷标签，先关闭
    if (QuickOperationPath != NULL)
    {
        lv_mem_free(QuickOperationPath);
        QuickOperationPath = NULL;
        QuickOperationclose();
    }
    QuickOperationInit();

    int path_len = strlen(path);
    QuickOperationPath = (char *)lv_mem_alloc(path_len + 1);
    memset(QuickOperationPath, 0, path_len + 1);
    sprintf(QuickOperationPath, "%s", path);

    lv_obj_t *scr = lv_scr_act();
    Operation_background = lv_btn_create(scr);
    lv_obj_align(Operation_background, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(Operation_background, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(Operation_background, &style_quick_operation_background, 0);
    lv_obj_add_event_cb(Operation_background, &BackgroundEventEb, LV_EVENT_SHORT_CLICKED, scr);

    Operation = lv_list_create(scr);
    lv_obj_align(Operation, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(Operation, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    int list_size = (sizeof(list_label)/sizeof(list_label[0]));
    for (int i = 0; i < list_size; i++)
    {
        if (QuickOperationSavePath == NULL)
        {
            if (!strcmp(list_label[i], OPERATION_LIST_PASTE)) continue;
        }
        lv_obj_t *list_btn = lv_list_add_btn(Operation, NULL, list_label[i]);
        lv_obj_set_size(list_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_add_style(list_btn, &style_quick_operation, 0);
        lv_obj_add_event_cb(list_btn, &ListEventEb, LV_EVENT_SHORT_CLICKED, scr);
    }
}

/***********************************************
 *   Name：LvCloseQuickOperation
 * Depict：创建快捷操作
 ***********************************************/
void LvCloseQuickOperation()
{
    // 如果存在快捷标签，先关闭
    if (QuickOperationPath != NULL)
    {
        lv_mem_free(QuickOperationPath);
        QuickOperationPath = NULL;
        QuickOperationclose();
    }
}


