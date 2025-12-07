#ifndef UTILS_H
#define UTILS_H

#include <string>
using namespace std;

// File reading functions
bool extract_content(const string& path, string& xml);
bool extract_binary_content(const string& path, string& content);

// File writing functions
bool writeToFile(const string& outputPath, const string& content);
bool writeBinaryToFile(const string& outputPath, const string& content);

#endif
