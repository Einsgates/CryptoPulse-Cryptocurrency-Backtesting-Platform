# Variables
CC = g++
#CFLAGS = -std=c++20 --coverage -ftest-coverage -fprofile-arcs -fprofile-dir=build/coverage -I./boost_1_84_0
CFLAGS = -std=c++20 -I./boost_1_84_0
APP = crypto_back_testing
APP_TESTER = crypto_tests
APP_INCLUDE = $(wildcard include/backtesting/*.h)

##############################################
# only need to change this part
# add more source files here to be compiled
APP_SOURCES = src/main.cpp

# add more test files here to be compiled
APP_TESTS = tests/unit_tests/order_unit_test.cpp
##############################################

GTEST_DIR = googletest
GTEST_REPO = https://github.com/google/googletest.git
GTEST_TAG = v1.14.0
ARCH := $(shell uname -m)

ifeq ($(ARCH),x86_64)
  CMAKE_ARCH = -DCMAKE_OSX_ARCHITECTURES=x86_64
else ifeq ($(ARCH),arm64)
  CMAKE_ARCH = -DCMAKE_OSX_ARCHITECTURES=arm64
else
  CMAKE_ARCH = ""
endif

# Targets
all: $(GTEST_DIR) $(APP)

$(APP): $(APP_SOURCES)
	rm -rf build
	mkdir -p build
	rm -rf *.gcno
	rm -rf *.gcda
	$(CC) $(CFLAGS) -o build/$@ $^ $(LDFLAGS)

$(GTEST_DIR):
	git clone --branch $(GTEST_TAG) $(GTEST_REPO) $@
	cd $@ && mkdir build && cd build && cmake $(CMAKE_ARCH) .. && make

test: $(APP_TESTS) $(APP_INCLUDE)
	rm -rf *.gcno
	rm -rf *.gcda
	$(CC) $(CFLAGS) -o build/$@ $^ -I$(GTEST_DIR)/googletest/include -Iinclude -L$(GTEST_DIR)/build/lib -lgtest -lgtest_main -pthread
	#mkdir -p ./build/coverage
#	mv *.gcno build/coverage


.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP_TESTER)
