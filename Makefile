TEST_DIR := tests
BUILD_DIR := $(TEST_DIR)/build

SRC = $(shell find ./$(TEST_DIR) -name 'test_*.c' | sed s/\\.\\///g )
TAR = $(SRC:.c=.c.test)
TARGETS := $(addprefix $(BUILD_DIR)/,$(TAR))

WARN_FLAGS := -Wall -Wextra -Werror -Winline -Wuninitialized -pedantic -Wno-format-overflow -Wno-unused-function -Wdouble-promotion
# WARN_FLAGS := -Wall -Wextra -Werror -Winline -Wuninitialized -Wno-format-overflow -Wno-unused-function -Wdouble-promotion
SANITIZE_FLAGS := -fsanitize-address-use-after-scope -fsanitize=address -fsanitize=undefined  -fsanitize=leak -fstack-protector-strong

CFLAGS := -DCEXTEST -O0 -g3 -ggdb3 $(SANITIZE_FLAGS) $(WARN_FLAGS) -Iinclude/

CEX_SRC_INCL = $(shell find ./cex -name '*.c')

.PHONY: clean run test tests cex all

all: cex tests


cex:
	cex process .
	./cli/cex_bundler
	

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -r $(BUILD_DIR) 2> /dev/null || exit 0

test:
	ulimit -c unlimited && cex test run $(t)

debug:
	cex test build $(t)
	ulimit -c unlimited && gdb -q --args $(BUILD_DIR)/$(t).test vvvvv $(c) 

tests: 
	cex test run all

