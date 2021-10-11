#
# Makefile
#
CC = /opt/zhangyufeng/Project/R818/lichee/prebuilt/gcc/linux-x86/aarch64/toolchain-sunxi-glibc/toolchain/bin/aarch64-openwrt-linux-gnu-gcc-6.4.1
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
EXT_DIR = -I/opt/zhangyufeng/Project/R818/lichee/out/r818-evb2/staging_dir/target/usr/include/ -I/opt/zhangyufeng/Project/R818/lichee/out/r818-evb2/compile_dir/target/wifimanager-0.0.1/ipkg-install/usr/include/
LDFLAGS ?= -lm $(LIB_DIR) -linput -lmtdev -ludev -levdev -lmycapture -lswscale -lavutil -lavcodec -lx264 -lswresample -lopus -lz -lvencoder -lMemAdapter -lvenc_codec -lvenc_base -lVE -lcdc_base -lpng -lwifid -lwifimg
LIB_DIR = -L/opt/zhangyufeng/Project/R818/lichee/out/r818-evb2/staging_dir/target/usr/lib/ -L/opt/zhangyufeng/Project/R818/lichee/out/r818-evb2/compile_dir/target/wifimanager-0.0.1/ipkg-install/usr/lib/ -L/opt/zhangyufeng/Project/lv_port_linux_frame_buffer
CFLAGS ?= $(EXT_DIR) $(LIB_DIR) $(LDFLAGS) -I$(LVGL_DIR)/ -Os -g -pipe -fno-caller-saves -fno-caller-saves -Wno-unused-result  -Wformat -Werror=format-security -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro
BIN = demo


#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
include $(LVGL_DIR)/lv_demos/lv_demo.mk
include $(LVGL_DIR)/cxsw_demos/cxsw_demo.mk

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

## MAINOBJ -> OBJFILES

all: default

%.o: %.c
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"
    
default: $(AOBJS) $(COBJS) $(MAINOBJ)
	$(CC) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS)

clean: 
	rm -f $(BIN) $(AOBJS) $(COBJS) $(MAINOBJ)

