TEST_DIR := tests
BUILD_DIR := $(TEST_DIR)/build

SRC = $(shell find ./$(TEST_DIR) -name 'test_*.c' | sed s/\\.\\///g )
TAR = $(SRC:.c=.c.test)
TARGETS := $(addprefix $(BUILD_DIR)/,$(TAR))

WARN_FLAGS := -Wall -Wextra -Werror -Winline -Wuninitialized -pedantic -Wno-format-overflow -Wno-unused-function -Wdouble-promotion
# WARN_FLAGS := -Wall -Wextra -Werror -Winline -Wuninitialized -Wno-format-overflow -Wno-unused-function -Wdouble-promotion
SANITIZE_FLAGS := -fsanitize-address-use-after-scope -fsanitize=address -fsanitize=undefined  -fsanitize=leak -fstack-protector-strong

CFLAGS := -DCEXTEST -O0 -g3 -ggdb3 $(SANITIZE_FLAGS) $(WARN_FLAGS) -I.
LDFLAGS := -lasan -lubsan

CEX_SRC_INCL = $(shell find ./cex -name '*.c')

.PHONY: clean run test tests cex all

all: cex tests


cex:
	@for cfile in $(CEX_SRC_INCL); do  (cat $$cfile | grep -E 'const struct __(class|module)__' > /dev/null) && cli/cex $$cfile --fakedir=tests/fake/; done 
	./cli/cex_bundler
	

$(BUILD_DIR):
	mkdir -p $@

# DEFAULT test building (no dependencies!)
$(BUILD_DIR)/%.c.test: %.c
	@echo "\nBuilding generic tests for: $*"
	@mkdir -p $(dir $@) > /dev/null 2>&1
	@./cli/atest --data=$(BUILD_DIR)/.atestdb $^ 
	# @rm $(BUILD_DIR)/$(t)*.gcda > /dev/null 2>&1|| exit 0
	# @rm $(BUILD_DIR)/$(t)*.gcno > /dev/null 2>&1|| exit 0
	$(CC) $(CFLAGS) $^ $(shell cat $*.c | grep -e "//\s*\#gcc_args " | sed 's@//\s*#gcc_args @@' | xargs) -o $@ $(LDFLAGS)

clean:
	rm -r $(BUILD_DIR) 2> /dev/null || exit 0


test:
	@echo "\nMaking CEX TEST: $(t)"
	@./cli/atest --data=$(BUILD_DIR)/.atestdb $(t) 
	@mkdir -p $(BUILD_DIR)/$(dir $(t)) # > /dev/null 2>&1
	$(CC) $(CFLAGS) $(t) $(shell cat $(t) | grep -e "//\s*\#gcc_args " | sed 's@//\s*#gcc_args @@' | xargs) -o $(BUILD_DIR)/$(t).test $(LDFLAGS)
	ulimit -c unlimited && $(BUILD_DIR)/$(t).test vvvvv $(c) 

debug:
	@echo "\nMaking CEX TEST: $(t)"
	@./cli/atest --data=$(BUILD_DIR)/.atestdb $(t) 
	@mkdir -p $(BUILD_DIR)/$(dir $(t)) # > /dev/null 2>&1
	$(CC) $(CFLAGS) $(t) $(shell cat $(t) | grep -e "//\s*\#gcc_args " | sed 's@//\s*#gcc_args @@' | xargs) -o $(BUILD_DIR)/$(t).test $(LDFLAGS)
	ulimit -c unlimited && gdb -q --args $(BUILD_DIR)/$(t).test vvvvv $(c) 

tests: $(TARGETS)
	@for test in $(SRC); do ulimit -c unlimited; $(BUILD_DIR)/$$test.test q $(c); done 

