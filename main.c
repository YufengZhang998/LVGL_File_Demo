#include "lvgl/lvgl.h"
#include "lv_conf.h"
#include "lv_drivers/indev/evdev.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_demos/lv_demo.h"
#include "cxsw_demos/cxsw_lvgl.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define DISP_BUF_SIZE (128 * 1024)

struct LvDirInfo_t DirInfo = {
    .PathParam = NULL,
    .FileList = NULL,
};

static void file_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if(code == LV_EVENT_SHORT_CLICKED) 
    {
        // 获取文件信息
        for (int i = 0; i < DirInfo.FileNum; i++)
        {
            if (btn != DirInfo.FileList[i]->btn_hand) continue;

            int dir_path_len = strlen(DirInfo.PathParam) + strlen(DirInfo.FileList[i]->d_name);
            char dir_path[dir_path_len + 10];
            sprintf(dir_path, "%s/%s", DirInfo.PathParam, DirInfo.FileList[i]->d_name);

            LV_LOG_USER("The file you opened is = %s", dir_path);
            break;
        }
        LvFreeDirInfo(&DirInfo);
        
        lv_obj_t *scr = lv_scr_act();
        LvCreateDirManager(scr, "/", &DirInfo, &file_event_cb);
    }
}

int main(void)
{
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 1024;
    disp_drv.ver_res    = 600;
    disp_drv.rotated    = LV_DISP_ROT_270;
    disp_drv.sw_rotate  = 1;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    evdev_init();
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    /* 为触摸屏绑定显示设备 */
    indev_drv.disp = disp;
    /* 注册触摸屏设备 */
    lv_indev_drv_register(&indev_drv);

    /*Create a Demo*/
    // lv_demo_widgets();
    lv_obj_t *scr = lv_scr_act();
    // LvCreateDirManager(scr, "/", &DirInfo, &file_event_cb);
    LvCreateWifiManager(scr);

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_task_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
