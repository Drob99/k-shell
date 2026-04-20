CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O0
ASAN_FLAGS := -fsanitize=address -g -O1

SRCS      := $(wildcard src/*.c)
OBJS      := $(SRCS:.c=.o)
LIB_SRCS  := $(filter-out src/main.c, $(SRCS))

TARGET      := kshell
TARGET_ASAN := kshell-asan

TEST_SRC          := tests/test_parser.c
TEST_BIN          := tests/test_parser
TEST_BUILTINS_SRC := tests/test_builtins.c
TEST_BUILTINS_BIN := tests/test_builtins

.PHONY: all test asan clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

asan:
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -Iinclude -o $(TARGET_ASAN) $(SRCS)

test:
	$(CC) $(CFLAGS) -Iinclude -o $(TEST_BIN) $(TEST_SRC) $(LIB_SRCS)
	$(CC) $(CFLAGS) -Iinclude -o $(TEST_BUILTINS_BIN) $(TEST_BUILTINS_SRC) $(LIB_SRCS)
	./$(TEST_BIN) && ./$(TEST_BUILTINS_BIN)

clean:
	rm -f src/*.o $(TARGET) $(TARGET_ASAN) $(TEST_BIN) $(TEST_BUILTINS_BIN)
