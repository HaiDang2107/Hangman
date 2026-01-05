CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -g
LDFLAGS = -lncurses -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Source files
UI_SOURCES = $(wildcard $(SRC_DIR)/ui/*.cpp)
NETWORK_SOURCES = $(wildcard $(SRC_DIR)/network/*.cpp)
PROTOCOL_SOURCES = $(wildcard $(SRC_DIR)/protocol/*.cpp)
MAIN_SOURCE = $(SRC_DIR)/test.cpp

ALL_SOURCES = $(UI_SOURCES) $(NETWORK_SOURCES) $(PROTOCOL_SOURCES) $(MAIN_SOURCE)

# Object files
UI_OBJECTS = $(patsubst $(SRC_DIR)/ui/%.cpp,$(BUILD_DIR)/ui/%.o,$(UI_SOURCES))
NETWORK_OBJECTS = $(patsubst $(SRC_DIR)/network/%.cpp,$(BUILD_DIR)/network/%.o,$(NETWORK_SOURCES))
PROTOCOL_OBJECTS = $(patsubst $(SRC_DIR)/protocol/%.cpp,$(BUILD_DIR)/protocol/%.o,$(PROTOCOL_SOURCES))
MAIN_OBJECT = $(BUILD_DIR)/test.o

ALL_OBJECTS = $(UI_OBJECTS) $(NETWORK_OBJECTS) $(PROTOCOL_OBJECTS) $(MAIN_OBJECT)

# Target executable
TARGET = client

# Default target
all: $(TARGET)

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/ui
	mkdir -p $(BUILD_DIR)/network
	mkdir -p $(BUILD_DIR)/protocol

# Link
$(TARGET): $(BUILD_DIR) $(ALL_OBJECTS)
	@echo "Linking: $(TARGET)"
	$(CXX) $(ALL_OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! Run: ./$(TARGET)"

# Compile UI sources
$(BUILD_DIR)/ui/%.o: $(SRC_DIR)/ui/%.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Network sources
$(BUILD_DIR)/network/%.o: $(SRC_DIR)/network/%.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Protocol sources
$(BUILD_DIR)/protocol/%.o: $(SRC_DIR)/protocol/%.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile main source
$(BUILD_DIR)/test.o: $(SRC_DIR)/test.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Rebuild
rebuild: clean all

.PHONY: all clean rebuild
