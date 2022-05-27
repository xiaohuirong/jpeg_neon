BIN_DIR = bin
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj

CC = gcc
LD = gcc
INC = -I$(INC_DIR)

CFLAGS += $(INC)
OPT = -O1
LDFLAGS = -lm

OBJ_FILES = $(OBJ_DIR)/main.o \
			$(OBJ_DIR)/getimage.o \
			$(OBJ_DIR)/jpeg_encoder_neon.o 
TARGET = $(BIN_DIR)/gcode

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(LD) $(LDFLAGS) $(OBJ_FILES) -o $(TARGET)

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(OPT) -c $(SRC_DIR)/main.c -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/getimage.o: $(SRC_DIR)/getimage.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/getimage.c -o $(OBJ_DIR)/getimage.o

$(OBJ_DIR)/jpeg_encoder_neon.o: $(SRC_DIR)/jpeg_encoder_neon.c
	$(CC) $(CFLAGS) $(OPT) -c $(SRC_DIR)/jpeg_encoder_neon.c -o $(OBJ_DIR)/jpeg_encoder_neon.o

clean:
	rm -f $(TARGET) $(OBJ_FILES)




