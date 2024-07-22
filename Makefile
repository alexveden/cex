.PHONY: clean run test tests cex all
BUILD_DIR=tests/build

all: cex tests


cex:
	cex process .
	./cli/cex_bundler

clean:
	cex test clean all

test:
	ulimit -c unlimited && cex test run $(t)

debug:
	cex test build $(t)
	ulimit -c unlimited && gdb -q --args $(BUILD_DIR)/$(t).test vvvvv $(c) 

tests: 
	cex test run all

tests32: 
	# cex test clean all
	cex test build all --ccargs="-m32"
	cex test run all
