SRC_DIR = ./src
OUT_DIR = ./out
OUT_NAME = game

CFLAGS = `sdl2-config --cflags`
LIB_FLAGS = `sdl2-config --libs`

ALL_OBJS = $(OUT_DIR)/main.o $(OUT_DIR)/args.o $(OUT_DIR)/files.o $(OUT_DIR)/GUI.o $(OUT_DIR)/map.o

all: $(OUT_DIR) $(OUT_NAME)

$(OUT_NAME): $(ALL_OBJS)
	gcc $(ALL_OBJS) $(LIB_FLAGS) -o $@

$(OUT_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/args.h $(SRC_DIR)/map.h $(SRC_DIR)/GUI.h $(SRC_DIR)/files.h
	gcc $(CFLAGS) -c $< -o $@

$(OUT_DIR)/args.o: $(SRC_DIR)/args.c $(SRC_DIR)/args.h
	gcc $(CFLAGS) -c $< -o $@

$(OUT_DIR)/files.o: $(SRC_DIR)/files.c $(SRC_DIR)/files.h
	gcc $(CFLAGS) -c $< -o $@

$(OUT_DIR)/GUI.o: $(SRC_DIR)/GUI.c $(SRC_DIR)/GUI.h $(SRC_DIR)/map.h
	gcc $(CFLAGS) -c $< -o $@

$(OUT_DIR)/map.o: $(SRC_DIR)/map.c $(SRC_DIR)/map.h
	gcc $(CFLAGS) -c $< -o $@

run: $(OUT_NAME)
	./$(OUT_NAME)

clean:
	rm -rf $(OUT_DIR) $(OUT_NAME)