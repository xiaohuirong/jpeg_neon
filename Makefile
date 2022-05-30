BIN_DIR = bin
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj

CC = gcc
C++ = gcc
LD = gcc
INC = -I$(INC_DIR)

CFLAGS += $(INC)
OPT = -O1
LDFLAGS = -lm

# 需要编译不带neon版本的将jpeg_encoder_noen.o替换为jpeg_encoder.o
# 并将主函数中的//#include "jpeg_encoder_neon.h" 替换为jpeg_encoder.o
# C文件
OBJ_FILES_C = $(OBJ_DIR)/main.o \
			  $(OBJ_DIR)/jpeg_decoder.o \
			  $(OBJ_DIR)/show.o \
			$(OBJ_DIR)/getimage.o \
			$(OBJ_DIR)/jpeg_encoder.o \
			$(OBJ_DIR)/image_enhanced.o \
		    $(OBJ_DIR)/huff.o  
# 所有文件
OBJ_FILES = $(OBJ_FILES_C)
#总链接生成文件
TARGET = $(BIN_DIR)/gcode

all: $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

# cpp文件编译
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(C++) $(CFLAGS) -c $< -o $@ 

# 必须要在最后所有.o文件链接为一个文件的时候链接opencv库 
# 如果在编译cpp的时候链接会导致最终找不到cv库
$(TARGET) : $(OBJ_FILES_C)
# $(C++) $(LDFLAGS) $(OBJ_FILES) -o $@ `pkg-config opencv --libs`  #若包含opencv库需要链接
	$(C++) $(LDFLAGS) $(OBJ_FILES_C)-o $@ -lpthread
	
	@echo "> build success <"	

clean:
	rm -f $(TARGET) $(OBJ_FILES)




