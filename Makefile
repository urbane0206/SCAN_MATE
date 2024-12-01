CXX = g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -m32
LFLAGS := -Llib -lop20pt32 -m32

OUTPUT := output
SRC := src
INCLUDE := include
LIB := lib

# Automatically collect all .cpp files in the SRC directory
SOURCES := $(wildcard $(SRC)/*.cpp)
# Generate corresponding .o files for all .cpp files
OBJECTS := $(SOURCES:$(SRC)/%.cpp=$(SRC)/%.o)
# Define the output executable
MAIN := $(OUTPUT)/main.exe

# Default target builds everything
all: $(OUTPUT) $(MAIN)

# Ensure the output directory exists
$(OUTPUT):
	mkdir -p $(OUTPUT)

# Link object files into the final executable
$(MAIN): $(OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $(OBJECTS)

# Compile each .cpp file to an .o file
$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(SRC)/*.o $(OUTPUT)/main.exe
