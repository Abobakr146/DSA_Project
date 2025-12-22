# Compiler & Flags
CXX = g++
CXXFLAGS = -Wall -g
INCLUDES = -Iexternal/tinyxml2

# Source Files
SRCS = xml_editor.cpp external/tinyxml2/tinyxml2.cpp graph.cpp functions.cpp utils.cpp

# ---------------------------------------------------------
# OS Configuration
# Usage: make OS=windows OR make OS=linux
# ---------------------------------------------------------
OS = windows

# Define Output Directory
OUT_DIR = output

# Define Output Files (All inside the output directory)
INPUT_XML       = full_test.xml

VERIFY_OUTPUT   = $(OUT_DIR)/verify_file.xml
FORMAT_OUTPUT   = $(OUT_DIR)/format_file.xml
MINIFY_OUTPUT   = $(OUT_DIR)/minify_file.xml
DECOMP_OUTPUT   = $(OUT_DIR)/decompress_file.xml
JSON_FILE       = $(OUT_DIR)/output_file.json
COMP_FILE       = $(OUT_DIR)/output_file.comp
DRAW_OUTPUT     = $(OUT_DIR)/output_file.jpg
TEMP_DOT        = temp_graph.dot
SEARCH_W_OUTPUT = $(OUT_DIR)/search_word_file.xml
SEARCH_T_OUTPUT = $(OUT_DIR)/search_topic_file.xml

WORD = Hello
TOPIC = education

RM = rm -f
RMDIR = rm -rf
MKDIR = mkdir -p $(OUT_DIR)

ifeq ($(OS), windows)
    TARGET = xml_editor.exe
    EXEC = .\$(TARGET)
    # Path separator fix for Windows cleanup
    CLEAN_FILES = $(OUT_DIR)\*
else
    TARGET = xml_editor
    EXEC = ./$(TARGET)
    CLEAN_FILES = $(OUT_DIR)/*
endif

# ---------------------------------------------------------
# Main Targets
# ---------------------------------------------------------

all: build directories verify format convert minify compress decompress draw searchword searchtopic
	@echo "All tasks completed successfully. Check the '$(OUT_DIR)' folder."

# Create the output directory
directories:
	@$(MKDIR)

# The build target
build:
	$(CXX) $(SRCS) $(INCLUDES) -o $(TARGET)

# ---------------------------------------------------------
# Run Targets
# ---------------------------------------------------------

verify: $(TARGET) directories
	@echo "--- Verifying ---"
	$(EXEC) verify -i $(INPUT_XML) -o $(VERIFY_OUTPUT)

format: $(TARGET) directories
	@echo "--- Formatting ---"
	$(EXEC) format -i $(INPUT_XML) -o $(FORMAT_OUTPUT)

convert: $(TARGET) directories
	@echo "--- Converting to JSON ---"
	$(EXEC) json -i $(INPUT_XML) -o $(JSON_FILE)

minify: $(TARGET) directories
	@echo "--- Minifying ---"
	$(EXEC) mini -i $(INPUT_XML) -o $(MINIFY_OUTPUT)

compress: $(TARGET) directories
	@echo "--- Compressing ---"
	$(EXEC) compress -i $(INPUT_XML) -o $(COMP_FILE)

decompress: $(TARGET) directories
	@echo "--- Decompressing ---"
	$(EXEC) decompress -i $(COMP_FILE) -o $(DECOMP_OUTPUT)

draw: $(TARGET) directories
	@echo "--- Drawing Network ---"
	$(EXEC) draw -i $(INPUT_XML) -o $(DRAW_OUTPUT)

searchword: $(TARGET) directories
	@echo "--- Searching ---"
	$(EXEC) search -w $(WORD) -i $(INPUT_XML) -o $(SEARCH_W_OUTPUT)

searchtopic: $(TARGET) directories
	@echo "--- Searching ---"
	$(EXEC) search -t $(TOPIC) -i $(INPUT_XML) -o $(SEARCH_T_OUTPUT)

# ---------------------------------------------------------
# Clean
# ---------------------------------------------------------
clean:
	$(RM) $(TARGET) $(TARGET).exe
	$(RM) $(TEMP_DOT)
	@echo "Cleaning output directory..."
    # We try to remove the files inside output, or the directory itself
	$(RMDIR) $(OUT_DIR)
	clear

.PHONY: clean all build directories