/**
 * @file cxsw_directory.c
 * @author Feng
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "cxsw_directory.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef void (*ButtonEventCb)(lv_event_t * e);
enum FileSortWay{
    NAME_SORT,
    TIME_SORT,
    MODE_SORT,
    SIZE_SORT,
    MAX_SORT,
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void DirManagerInit(void);
static void DirManageResetInit(void);
static inline int SortByDatetime(const struct dirent **a, const struct dirent **b);
static inline int SortBySize(const struct dirent **a, const struct dirent **b);
static inline int SortByMode(const struct dirent **a, const struct dirent **b);

static void LvDispFileList(struct LvFileList_t *list_info);
static void LvSetFileList();
static void LvSetFileListItem();
static void LvDispDirInfo(char *path);

static void ItemEventEb(lv_event_t * e);
static void FileEventEb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
static enum FileSortWay TempSortWay = NAME_SORT;
static const char list_item[][10] = {DIR_MANAGER_ITEM_FILE, DIR_MANAGER_ITEM_DATE, DIR_MANAGER_ITEM_TYPE, DIR_MANAGER_ITEM_SIZE};

static lv_style_t style_dir_manager;
static lv_style_t style_dir_manager_child;
static lv_style_t style_dir_file;

static lv_obj_t * DirManager;
static lv_obj_t * DirItem;
static lv_obj_t * FileList;

static char *TempDirRootPath = NULL;
static struct LvDirInfo_t *TempDirInfo = NULL;
static ButtonEventCb TempEventCb = NULL;
static const char *TempSortPath = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
/***********************************************
 *   Name：DirManagerInit
 * Depict：文件管理器初始化
 ***********************************************/
static void DirManagerInit(void)
{
    // 设置文件管理器样式
    lv_style_init(&style_dir_manager);
    lv_style_set_pad_top(&style_dir_manager, 0);
    lv_style_set_pad_bottom(&style_dir_manager, 0);
    lv_style_set_pad_left(&style_dir_manager, 0);
    lv_style_set_pad_right(&style_dir_manager, 0);
    lv_style_set_pad_row(&style_dir_manager, 0);
    lv_style_set_pad_column(&style_dir_manager, 0);

    // 设置文件管理器子类样式
    lv_style_init(&style_dir_manager_child);
    lv_style_set_radius(&style_dir_manager_child, 0);
    lv_style_set_pad_top(&style_dir_manager_child, 0);
    lv_style_set_pad_bottom(&style_dir_manager_child, 0);

    // 设置目录样式
    lv_style_init(&style_dir_file);
    lv_style_set_text_font(&style_dir_file, &lv_font_montserrat_40);
}

/***********************************************
 *   Name：DirManagerInit
 * Depict：文件管理器初始化
 ***********************************************/
static void DirManageResetInit(void)
{
    // 复位样式
    lv_style_reset(&style_dir_manager);
    lv_style_reset(&style_dir_manager_child);
    lv_style_reset(&style_dir_file);

    // 释放路径内存
    if(TempDirRootPath != NULL)
    {
        lv_mem_free(TempDirRootPath);
        TempDirRootPath = NULL;
    }

    // 关闭容器对象
    lv_obj_clean(DirItem);
    lv_obj_del(DirItem);
    lv_obj_clean(FileList);
    lv_obj_del(FileList);
    lv_obj_clean(DirManager);
    lv_obj_del(DirManager);
}

/***********************************************
 *   Name：SortByDatetime
 * Depict：设置通过时间顺序获取文件信息
 ***********************************************/
static inline int SortByDatetime(const struct dirent **a, const struct dirent **b)
{
    struct stat sbuf1, sbuf2;
    char path1[PATH_MAX], path2[PATH_MAX];

    snprintf(path1, PATH_MAX, "%s/%s", TempSortPath, (*a)->d_name);
    snprintf(path2, PATH_MAX, "%s/%s", TempSortPath, (*b)->d_name);

    if (lstat(path1, &sbuf1)) return 0;
    if (lstat(path2, &sbuf2)) return 0;

    return sbuf2.st_mtim.tv_sec - sbuf1.st_mtim.tv_sec;
}

/***********************************************
 *   Name：SortBySize
 * Depict：设置通过大小顺序获取文件信息
 ***********************************************/
static inline int SortBySize(const struct dirent **a, const struct dirent **b)
{
    struct stat sbuf1, sbuf2;
    char path1[PATH_MAX], path2[PATH_MAX];

    snprintf(path1, PATH_MAX, "%s/%s", TempSortPath, (*a)->d_name);
    snprintf(path2, PATH_MAX, "%s/%s", TempSortPath, (*b)->d_name);

    if (lstat(path1, &sbuf1)) return 0;
    if (lstat(path2, &sbuf2)) return 0;

    return sbuf2.st_size - sbuf1.st_size;
}

/***********************************************
 *   Name：SortByType
 * Depict：设置通过类型顺序获取文件信息
 ***********************************************/
static inline int SortByMode(const struct dirent **a, const struct dirent **b)
{
    struct stat sbuf1, sbuf2;
    char path1[PATH_MAX], path2[PATH_MAX];

    snprintf(path1, PATH_MAX, "%s/%s", TempSortPath, (*a)->d_name);
    snprintf(path2, PATH_MAX, "%s/%s", TempSortPath, (*b)->d_name);

    if (lstat(path1, &sbuf1)) return 0;
    if (lstat(path2, &sbuf2)) return 0;

    return sbuf2.st_mode - sbuf1.st_mode;
}

/***********************************************
 *   Name：LvGetDirInfo
 * Depict：获取文件夹信息
 ***********************************************/
void LvGetDirInfo(const char *name, struct LvDirInfo_t *info)
{
    struct dirent **entry_list = NULL;
    TempSortPath = name;

    // 按照制定的规则顺序，获取文件个数和文件名称
    int count = -1;
    switch (TempSortWay)
    {
        case NAME_SORT: count = scandir(name, &entry_list, NULL, alphasort); break;
        case TIME_SORT: count = scandir(name, &entry_list, NULL, SortByDatetime); break;
        case MODE_SORT: count = scandir(name, &entry_list, NULL, SortByMode); break;
        case SIZE_SORT: count = scandir(name, &entry_list, NULL, SortBySize); break;
            default:    count = scandir(name, &entry_list, NULL, alphasort); break;
    }
    if (count < 0) 
    {
        LV_LOG_WARN("Wrong path “%s”", name);
        return;
    }

    // 申请空间
    struct LvFileList_t **FlieList = (struct LvFileList_t **)lv_mem_alloc(sizeof(struct LvFileList_t *) * count);
    for(int i = 0; i < count; i++)
    {
        FlieList[i] = (struct LvFileList_t *)lv_mem_alloc(sizeof(struct LvFileList_t));
        memset(FlieList[i], 0, sizeof(struct LvFileList_t));
    }
    info->PathParam = (char *)lv_mem_alloc(strlen(name)+1);
    memset(info->PathParam, 0, strlen(name)+1);

    // 获取文件信息
    sprintf(info->PathParam, "%s", name);
    info->FileNum = count;
    for (int i = 0; i < count; i++)
    {
        struct dirent *entry = entry_list[i];
        struct stat stat_info; 

        //生成查询文件名路径
        int dir_path_len = strlen(name) + strlen(entry->d_name);
        char dir_path[dir_path_len + 10];
        sprintf(dir_path, "%s/%s", name, entry->d_name);

        int ret = lstat(dir_path, &stat_info);
        if (ret == -1)
        {
            LV_LOG_WARN("%s get info error !", dir_path);
            continue;
        }

        struct LvFileList_t *info_temp = FlieList[i];
        if (entry->d_reclen < DEF_FILE_NAME_LEN)
        {
            sprintf(info_temp->d_name, "%s", entry->d_name);
        }
        else
        {
            sprintf(info_temp->d_name, "%s", DIR_MANAGER_FILE_DEF_NAME);
        }
        info_temp->st_mode = stat_info.st_mode;
        info_temp->st_size = stat_info.st_size;
        info_temp->st_mtim = stat_info.st_mtim;

        free(entry);
    }
    free(entry_list);

    LV_LOG_USER("Get file num = %d", info->FileNum);
    info->FileList = FlieList;
}

/***********************************************
 *   Name：LvFreeDirInfo
 * Depict：释放文件目录信息
 ***********************************************/
void LvFreeDirInfo(struct LvDirInfo_t *free_list)
{
    if (free_list->PathParam != NULL)
    {
        LV_LOG_INFO("Free dir path param!");
        lv_mem_free(free_list->PathParam);
        free_list->PathParam = NULL;
    }
    if (free_list->FileList != NULL)
    {
        LV_LOG_INFO("Free file info list!");
        for (int i = 0; i < free_list->FileNum; i++)
        {
            lv_mem_free(free_list->FileList[i]);
            free_list->FileList[i] = NULL;
        }
        lv_mem_free(free_list->FileList);
        free_list->FileList = NULL;
    }
}

/***********************************************
 *   Name：LvDispFileList
 * Depict：显示目录文件列表
 ***********************************************/
static void LvDispFileList(struct LvFileList_t *list_info)
{
    static lv_coord_t grid_col_dsc[] = {40, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 5, LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    // 获取文件类型
    char file_mode_icon[10] = {0,};
    char file_mode_txt[10] = {0,};
    switch (list_info->st_mode & S_IFMT)
    {
        case S_IFREG: //普通文件
        {
            sprintf(file_mode_icon, "%s", LV_SYMBOL_FILE);
            sprintf(file_mode_txt, "%s", "F");
            break;
        }
        case S_IFBLK: //块设备文件
        {
            sprintf(file_mode_icon, "%s", LV_SYMBOL_DIRECTORY);
            sprintf(file_mode_txt, "%s", "B");
            break;
        }
        case S_IFDIR: //目录文件
        {
            sprintf(file_mode_icon, "%s", LV_SYMBOL_DIRECTORY);
            sprintf(file_mode_txt, "%s", "D");
            break;
        }
        case S_IFCHR: //字符设备文件
        {
            sprintf(file_mode_icon, "%s", LV_SYMBOL_DIRECTORY);
            sprintf(file_mode_txt, "%s", "C");
            break;
        }
        case S_IFLNK: //链接文件
        {
            sprintf(file_mode_icon, "%s", LV_SYMBOL_DIRECTORY);
            sprintf(file_mode_txt, "%s", "L");
            break;
        }
        default:
        {
            sprintf(file_mode_icon, "%s", "");
            sprintf(file_mode_txt, "%s", "");
            break;
        }
    }

    // 创建单个文件显示容器
    lv_obj_t * cont = lv_obj_create(FileList);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);

    // 容器绑定按键
    list_info->btn_hand = lv_btn_create(cont);
    lv_obj_set_size(list_info->btn_hand, LV_PCT(50), LV_PCT(100));
    lv_obj_set_style_bg_color(list_info->btn_hand, lv_color_white(), 0);
    lv_obj_set_style_radius(list_info->btn_hand, 0, 0);
    lv_obj_set_style_opa(list_info->btn_hand, LV_OPA_100, 0);
    lv_obj_add_event_cb(list_info->btn_hand, &FileEventEb, LV_EVENT_SHORT_CLICKED, cont);

    // 显示文件信息
    lv_obj_t * label;
    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, file_mode_icon);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, list_info->d_name);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    struct timespec date = list_info->st_mtim;
    struct tm *p = localtime(&date.tv_sec);
    char time_str[100]; 
    // strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", p);
    strftime(time_str, sizeof(time_str), "%Y/%m/%d", p);
    lv_label_set_text(label, time_str);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, file_mode_txt);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_END, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    __off_t size_kb = (list_info->st_size)/1024;
    char size_str[50];
    sprintf(size_str ,"%ld", size_kb);
    lv_label_set_text(label, size_str);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 6, 1, LV_GRID_ALIGN_START, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, "KB");
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 8, 1, LV_GRID_ALIGN_START, 0, 1);
}

/***********************************************
 *   Name：LvSetFileList
 * Depict：设置目录文件列表
 ***********************************************/
static void LvSetFileList()
{
    // 清空文件列表显示
    lv_obj_clean(FileList);
    for (int i = 0; i < TempDirInfo->FileNum; i++)
    {
        struct LvFileList_t *info = TempDirInfo->FileList[i];
        LvDispFileList(info);
    }
}

/***********************************************
 *   Name：LvSetFileListItem
 * Depict：设置目录文件列表标题栏
 ***********************************************/
static void LvSetFileListItem()
{
    static lv_coord_t grid_col_dsc[] = {40, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 5, LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};

    // 清空标题栏显示
    lv_obj_clean(DirItem);
    // 创建容器
    lv_obj_t *cont = lv_obj_create(DirItem);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);

    lv_obj_t * item_label = lv_label_create(cont);
    lv_obj_add_style(item_label, &style_dir_file, 0);
    char *temppath = TempDirInfo->PathParam + strlen(TempDirRootPath);
    lv_label_set_text(item_label, temppath);
    lv_obj_set_grid_cell(item_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);

    int count = 0;
    char dropdown_str[100];
    int list_size = (sizeof(list_item)/sizeof(list_item[0]));
    for (int i = 0; i < list_size; i++)
    {
        if ((i + 1) == list_size) sprintf(&dropdown_str[count], "%s", list_item[i]);
        else sprintf(&dropdown_str[count], "%s\n", list_item[i]);
        count += (strlen(list_item[i]) + 1);
    }
    lv_obj_t * sort_dropdowm = lv_dropdown_create(cont);
    lv_dropdown_set_options(sort_dropdowm, dropdown_str);
    lv_dropdown_set_selected(sort_dropdowm, TempSortWay);
    lv_obj_add_event_cb(sort_dropdowm, &ItemEventEb, LV_EVENT_VALUE_CHANGED, cont);
    lv_obj_set_grid_cell(sort_dropdowm, LV_GRID_ALIGN_START, 6, 1, LV_GRID_ALIGN_START, 0, 1);

    lv_obj_t *label;
    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, list_item[0]);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 1, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, list_item[1]);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_START, 1, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, list_item[2]);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_END, 1, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_dir_file, 0);
    lv_label_set_text(label, list_item[3]);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 6, 1, LV_GRID_ALIGN_START, 1, 1);
}

/***********************************************
 *   Name：LvDispDirInfo
 * Depict：显示文件信息
 ***********************************************/
static void LvDispDirInfo(char *path)
{
    //获取信息
    struct LvDirInfo_t GetDirInfo = {
        .FileList = NULL,
        .PathParam = NULL,
    };
    LvGetDirInfo(path, &GetDirInfo);

    // 更新信息
    if (GetDirInfo.FileList != NULL)
    {
        LvFreeDirInfo(TempDirInfo);
        TempDirInfo->FileList = GetDirInfo.FileList;
        TempDirInfo->PathParam = GetDirInfo.PathParam;
        TempDirInfo->FileNum = GetDirInfo.FileNum;
    }

    // 更新显示
    if (TempDirInfo->FileList != NULL)
    {
        LvSetFileListItem();
        LvSetFileList();
    }
    LV_LOG_USER("Current display finish");
}

/***********************************************
 *   Name：ItemEventEb
 * Depict：标题按键事件响应
 ***********************************************/
static void ItemEventEb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        // 更新排序方式
        TempSortWay = lv_dropdown_get_selected(btn) < MAX_SORT ? lv_dropdown_get_selected(btn) : NAME_SORT;
        LV_LOG_USER("Current sort way = %d", TempSortWay);
        LvDispDirInfo(TempDirInfo->PathParam);
    }
}

/***********************************************
 *   Name：QuickOperationEventEb
 * Depict：快捷操作事件响应
 ***********************************************/
static void QuickOperationEventEb(void *para)
{
    // 快速操作结束响应，刷新页面
    LvDispDirInfo(TempDirInfo->PathParam);
}

/***********************************************
 *   Name：FileEventEb
 * Depict：文件按键事件响应
 ***********************************************/
static void FileEventEb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_SHORT_CLICKED) 
    {
        // 查找相应按键
        for (int i = 0; i < TempDirInfo->FileNum; i++)
        {
            // 遍历按键列表
            if (btn != TempDirInfo->FileList[i]->btn_hand) continue;
            // 判断是文件夹，继续打开文件夹
            if ((TempDirInfo->FileList[i]->st_mode & S_IFMT) == S_IFDIR)
            {
                // 获取选中文件路径
                int dir_path_len = strlen(TempDirInfo->PathParam) + strlen(TempDirInfo->FileList[i]->d_name);
                char dir_path[dir_path_len + 10];
                // 根据不同的按键功能，修改目录打开路径
                if (!strcmp(TempDirInfo->FileList[i]->d_name, "."))
                {
                    sprintf(dir_path, "%s", TempDirInfo->PathParam);
                }
                else if (!strcmp(TempDirInfo->FileList[i]->d_name, ".."))
                {
                    if (strcmp(TempDirInfo->PathParam, TempDirRootPath))
                    {
                        char *p = strrchr(TempDirInfo->PathParam, '/');
                        *p = '\0';
                    }
                    sprintf(dir_path, "%s", TempDirInfo->PathParam);
                }
                else
                {
                    sprintf(dir_path, "%s/%s", TempDirInfo->PathParam, TempDirInfo->FileList[i]->d_name);
                }
                // 显示文件管理器
                LvDispDirInfo(dir_path); 
            }
            else
            {
                LV_LOG_USER("File not dir, not open!!!");
                DirManageResetInit();
                TempEventCb(e);
            }
            break;
        }
    }
    else if(code == LV_EVENT_LONG_PRESSED)
    {
        LV_LOG_USER("long open file!!!");
        // 查找相应按键
        for (int i = 0; i < TempDirInfo->FileNum; i++)
        {
            // 遍历按键列表
            if (btn != TempDirInfo->FileList[i]->btn_hand) continue;
            // 获取选中文件路径
            int dir_path_len = strlen(TempDirInfo->PathParam) + strlen(TempDirInfo->FileList[i]->d_name);
            char dir_path[dir_path_len + 10];
            sprintf(dir_path, "%s/%s", TempDirInfo->PathParam, TempDirInfo->FileList[i]->d_name);
            LV_LOG_USER("path %s", dir_path);

            LvCreateQuickOperation(dir_path, &QuickOperationEventEb);
            break;
        }
    }
}

/***********************************************
 *   Name：LvCreateDirManager
 * Depict：创建目录管理器
 ***********************************************/
lv_obj_t *LvCreateDirManager(lv_obj_t *parent, char *path, struct LvDirInfo_t *dirinfo, void (*event_cb)(lv_event_t * e))
{
    DirManagerInit();
    // 记录文件列表信息
    TempDirInfo = dirinfo;  //是否需要申请内存
    // 记录按键回调指针
    TempEventCb = event_cb;
    // 记录文件夹路径
    TempDirRootPath = (char *)lv_mem_alloc(strlen(path));
    memset(TempDirRootPath, 0, strlen(path));
    sprintf(TempDirRootPath, "%s", path);

    // 创建文件管理器容器
    DirManager = lv_obj_create(parent);
    lv_obj_set_size(DirManager, LV_PCT(100), LV_PCT(90));
    lv_obj_add_style(DirManager, &style_dir_manager, 0);

    // 创建文件标题容器
    DirItem = lv_obj_create(DirManager);
    lv_obj_set_flex_flow(DirItem, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_size(DirItem, LV_PCT(100), LV_PCT(17));
    lv_obj_add_style(DirItem, &style_dir_manager_child, 0);
    lv_obj_set_flex_grow(DirItem, 1);
    lv_obj_add_flag(DirItem, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    // 创建文件列表容器
    FileList = lv_obj_create(DirManager);
    lv_obj_align_to(FileList, DirItem, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_flex_flow(FileList, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_size(FileList, LV_PCT(100), LV_PCT(83));
    lv_obj_add_style(FileList, &style_dir_manager_child, 0);
    lv_obj_set_flex_grow(FileList, 1);
    lv_obj_add_flag(FileList, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    // 显示文件管理器
    LvDispDirInfo(TempDirRootPath);
    return DirManager; 
}


