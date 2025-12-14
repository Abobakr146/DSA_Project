#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;

bool extract_content(const string& path, string& xml) {
    // 1. Open the file for reading
    ifstream inputFile(path);

    // 2. Check if the file was successfully opened
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open input file at path: " << path << endl;
        xml = ""; // Ensure the output string is empty on failure
        return false;
    }

    // 3. Read the entire file content into a stringstream
    stringstream buffer;
    buffer << inputFile.rdbuf();

    // 4. Store the stringstream's content into the reference string
    xml = buffer.str();

    // 5. Close the file (optional, as ifstream destructor handles it, but good practice)
    inputFile.close();
    
    return true;
}

bool extract_binary_content(const string& path, string& content) {
    // Open the file in binary mode
    ifstream inputFile(path, ios::binary | ios::ate);

    // Check if the file was successfully opened
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open input file at path: " << path << endl;
        content = "";
        return false;
    }

    // Get file size
    streamsize size = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    // Read file into a vector
    vector<char> buffer(size);
    if (!inputFile.read(buffer.data(), size)) {
        cerr << "Error: Failed to read binary file" << endl;
        content = "";
        return false;
    }

    // Convert to string
    content = string(buffer.begin(), buffer.end());

    inputFile.close();
    
    return true;
}

bool writeToFile(const string& outputPath, const string& content) {
    ofstream file(outputPath);

    if (!file.is_open()) {
        cerr << "Error: Could not open output file: " << outputPath << endl;
        return false;
    }

    // Write content
    file << content;

    file.close();
    return true;
}

bool writeBinaryToFile(const string& outputPath, const string& content) {
    ofstream file(outputPath, ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open output file: " << outputPath << endl;
        return false;
    }

    // Write binary content
    file.write(content.c_str(), content.length());

    file.close();
    return true;
}
