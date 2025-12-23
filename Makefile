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
INPUT_XML       = input_file.xml

VERIFY   		= verify_file.xml
FORMAT   		= format_file.xml
MINIFY   		= minify_file.xml
DECOMP   		= decompress_file.xml
JSON       		= output_file.json
COMP       		= output_file.comp
DRAW     		= output_file.jpg
TEMP_DOT        = temp_graph.dot
SEARCH_W 		= search_word_file.xml
SEARCH_T 		= search_topic_file.xml
MOST_ACTIVE 	= most_active_file.xml
MOST_INFLUENCER = most_influencer_file.xml
MUTUAL 			= mutual_file.xml
SUGGEST 		= suggest_file.xml

FIX=1
WORD = Hello
TOPIC = education
IDS=1,2,3
USER_ID=1
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

all: clean build directories verify format convert minify compress decompress draw searchword searchtopic active influencer mutual suggest
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
ifeq ($(FIX), 1)
	$(EXEC) verify -i $(INPUT_XML) -f -o $(OUT_DIR)/$(VERIFY)
else
	$(EXEC) verify -i $(INPUT_XML) -o $(OUT_DIR)/$(VERIFY)
endif

format: $(TARGET) directories
	@echo "--- Formatting ---"
	$(EXEC) format -i $(INPUT_XML) -o $(OUT_DIR)/$(FORMAT)

convert: $(TARGET) directories
	@echo "--- Converting to JSON ---"
	$(EXEC) json -i $(INPUT_XML) -o $(OUT_DIR)/$(JSON)

minify: $(TARGET) directories
	@echo "--- Minifying ---"
	$(EXEC) mini -i $(INPUT_XML) -o $(OUT_DIR)/$(MINIFY)

compress: $(TARGET) directories
	@echo "--- Compressing ---"
	$(EXEC) compress -i $(INPUT_XML) -o $(OUT_DIR)/$(COMP)

decompress: $(TARGET) directories
	@echo "--- Decompressing ---"
	$(EXEC) decompress -i $(OUT_DIR)/$(COMP) -o $(OUT_DIR)/$(DECOMP)

draw: $(TARGET) directories
	@echo "--- Drawing Network ---"
	$(EXEC) draw -i $(INPUT_XML) -o $(OUT_DIR)/$(DRAW)

searchword: $(TARGET) directories
	@echo "--- Searching ---"
	$(EXEC) search -w $(WORD) -i $(INPUT_XML) -o $(OUT_DIR)/$(SEARCH_W)

searchtopic: $(TARGET) directories
	@echo "--- Searching ---"
	$(EXEC) search -t $(TOPIC) -i $(INPUT_XML) -o $(OUT_DIR)/$(SEARCH_T)

active: $(TARGET) directories
	@echo "--- Most Active User ---"
	$(EXEC) most_active -i $(INPUT_XML) -o $(OUT_DIR)/$(MOST_ACTIVE)

influencer: $(TARGET) directories
	@echo "--- Most Influencer User ---"
	$(EXEC) most_influencer -i $(INPUT_XML) -o $(OUT_DIR)/$(MOST_INFLUENCER)

mutual: $(TARGET) directories
	@echo "--- Mutual users between users with ids: $(IDS) ---"
	$(EXEC) mutual -i $(INPUT_XML) -ids $(IDS) -o $(OUT_DIR)/$(MUTUAL)

suggest: $(TARGET) directories
	@echo "--- Writes a list of suggested users for user with id: $(USER_ID) ---"
	$(EXEC) suggest -i $(INPUT_XML) -id $(USER_ID) -o $(OUT_DIR)/$(SUGGEST)

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

.PHONY: all build directories