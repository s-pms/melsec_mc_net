
#.PHONY:all clean 

ifeq ($(DEBUG),true)
#-g是生成调试信息。GNU调试器可以利用该信息
CC = gcc -g
VERSION = debug
else
CC = gcc
VERSION = release
endif

#CC = gcc

# $(wildcard *.c)表示扫描当前目录下所有.c文件
#SRCS = nginx.c ngx_conf.c
SRCS = $(wildcard *.c)

OBJS = $(SRCS:.c=.o)

#把字符串中的.c替换为.d
#DEPS = nginx.d ngx_conf.d
DEPS = $(SRCS:.c=.d)

#可以指定BIN文件的位置,addprefix是增加前缀函数
#BIN = /mnt/hgfs/linux/nginx
BIN := $(addprefix $(BUILD_ROOT)/,$(BIN))

#定义存放ojb文件的目录，目录统一到一个位置才方便后续链接，不然整到各个子目录去，不好链接
#注意下边这个字符串，末尾不要有空格等否则会语法错误 
LINK_OBJ_DIR = $(BUILD_ROOT)/app/link_obj
DEP_DIR = $(BUILD_ROOT)/app/dep

#-p是递归创建目录，没有就创建，有就不需要创建了
$(shell mkdir -p $(LINK_OBJ_DIR))
$(shell mkdir -p $(DEP_DIR))

OBJS := $(addprefix $(LINK_OBJ_DIR)/,$(OBJS))
DEPS := $(addprefix $(DEP_DIR)/,$(DEPS))

#找到目录中的所有.o文件（编译出来的）
LINK_OBJ = $(wildcard $(LINK_OBJ_DIR)/*.o)
#因为构建依赖关系时app目录下这个.o文件还没构建出来，所以LINK_OBJ是缺少这个.o的，我们 要把这个.o文件加进来
LINK_OBJ += $(OBJS)

#-------------------------------------------------------------------------------------------------------
all:$(DEPS) $(OBJS) $(BIN)

ifneq ("$(wildcard $(DEPS))","")   #如果不为空,$(wildcard)是函数【获取匹配模式文件名】，这里 用于比较是否为""
include $(DEPS)  
endif

#----------------------------------------------------------------1begin------------------
#$(BIN):$(OBJS)
$(BIN):$(LINK_OBJ)
	@echo "------------------------build $(VERSION) mode--------------------------------!!!"

ifeq ($(BUILD_SO), true)
# gcc -o 是生成so
	$(CC) -fPIC -shared -o $@.so $^
else
# gcc -o 是生成可执行文件
	$(CC) -o $@ $^
endif

#----------------------------------------------------------------1end-------------------


#----------------------------------------------------------------2begin-----------------
#%.o:%.c
$(LINK_OBJ_DIR)/%.o:%.c
#$(CC) -o $@ -c $^
	$(CC) -I$(INCLUDE_PATH) -o $@ -c $(filter %.c,$^)
#----------------------------------------------------------------2end-------------------



#----------------------------------------------------------------3begin-----------------
$(DEP_DIR)/%.d:%.c
#gcc -MM $^ > $@
	echo -n $(LINK_OBJ_DIR)/ > $@
#	gcc -MM $^ | sed 's/^/$(LINK_OBJ_DIR)&/g' > $@
#  >>表示追加
	gcc -I$(INCLUDE_PATH) -MM $^ >> $@
#----------------------------------------------------------------3begin-----------------



#----------------------------------------------------------------nbegin-----------------
clean:			
	rm -f $(BIN) $(OBJS) $(DEPS) *.gch
#----------------------------------------------------------------nend------------------





