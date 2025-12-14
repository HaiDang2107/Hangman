# Compiler settings
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Iinclude -g
LDFLAGS  := -pthread

# Directory structure
SRC_DIR   := src
BUILD_DIR := build
BIN_NAME  := server
TEST_BIN  := test_client
TEST_ROOM_BIN := test_room
TARGET    := $(BUILD_DIR)/$(BIN_NAME)
TEST_TARGET := $(BUILD_DIR)/$(TEST_BIN)
TEST_ROOM_TARGET := $(BUILD_DIR)/$(TEST_ROOM_BIN)

# Auto-detect all .cpp files recursively in src/
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')

# Map source files to object files (src/%.cpp -> build/%.o)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all test test_room

# Default target
all: $(TARGET)

# Test target
test: $(TEST_TARGET)

# Test room target
test_room: $(TEST_ROOM_TARGET)

# Linking: Create server executable from object files
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete! Run: ./$(TARGET)"

# Linking: Create test client executable (needs protocol objects)
$(TEST_TARGET): $(BUILD_DIR)/test_client.o $(BUILD_DIR)/protocol/packets.o
	@mkdir -p $(dir $@)
	@echo "Linking test client: $@"
	$(CXX) $(BUILD_DIR)/test_client.o $(BUILD_DIR)/protocol/packets.o -o $@ $(LDFLAGS)
	@echo "Test client built! Run: ./$(TEST_TARGET) [port]"

# Compilation: Create .o files from .cpp files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)  # Create build sub-directories on the fly
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compilation: Create test client object file
$(BUILD_DIR)/test_client.o: test_client.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Linking: Create test room executable
$(TEST_ROOM_TARGET): $(BUILD_DIR)/test_room.o $(BUILD_DIR)/protocol/packets.o
	@mkdir -p $(dir $@)
	@echo "Linking test room: $@"
	$(CXX) $(BUILD_DIR)/test_room.o $(BUILD_DIR)/protocol/packets.o -o $@ $(LDFLAGS)
	@echo "Test room built! Run: ./$(TEST_ROOM_TARGET) [port]"

# Compilation: Create test room object file
$(BUILD_DIR)/test_room.o: test_room.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleanup
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

.PHONY: all test clean