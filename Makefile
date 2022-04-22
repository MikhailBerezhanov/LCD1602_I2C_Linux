# CCPP=g++
# CC=gcc
CXXFLAGS = -Wall -O2 
#-Wno-format-security -Wno-unused-variable -Wno-unused-function

# PATHS
SRC_DIR = ./
INCLUDES = -I$(SRC_DIR)

LIBS = -lpthread

BIN_DIR = ./bin
BIN_NAME = lcd_util
OBJ_DIR = ./obj
TESTS_DIR=./tests

OBJS = $(addprefix $(OBJ_DIR)/, i2c.o lcd1602.o main.o)

.PHONY : clean info

all: info prep bin

# Prepare directories for output
prep:
	@if test ! -d $(BIN_DIR); then mkdir $(BIN_DIR); fi
	@if test ! -d $(OBJ_DIR); then mkdir $(OBJ_DIR); fi
	@if test ! -d $(TESTS_DIR); then mkdir $(TESTS_DIR); fi

info:
	@echo "Using compiler: $(notdir $(CXX))"

# cpp sources
$(OBJ_DIR)/%.o : %.cpp
	@echo "\033[32m>\033[0m CXX compile: \t" $<" >>> "$@
	@$(CXX) -c $(CXXFLAGS) $(INCLUDES) $(DEFINES) -o $@ $<

# Start linker
bin:$(OBJS)
	@echo Linking bin file: $(BIN_NAME)
	@$(CXX) -o $(BIN_DIR)/$(BIN_NAME) $^ $(LIBS) 
	@echo "\033[32mBuilding finished [$(shell date +"%T")]."


clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR) $(TESTS_DIR)

mem_check:	OUTPUT_FILE = $(TESTS_DIR)/valgrind-out.txt
mem_check:  CXXFLAGS = -g -O0

mem_check: bin
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=$(OUTPUT_FILE) $(BIN_NAME)
	@cat $(OUTPUT_FILE)