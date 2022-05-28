BIN_DIR = bin
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj

CC = g++
LD = g++
INC = -I$(INC_DIR)

CFLAGS += $(INC)
OPT = -O1
LDFLAGS = -lm

# 需要编译不带neon版本的将jpeg_encoder_noen.o替换为jpeg_encoder.o
OBJ_FILES = $(OBJ_DIR)/main.o \
			$(OBJ_DIR)/getimage.o \
			$(OBJ_DIR)/jpeg_encoder_neon.o 

TARGET = $(BIN_DIR)/gcode

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(LD) $(LDFLAGS) $(OBJ_FILES) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@


clean:
	rm -f $(TARGET) $(OBJ_FILES)




