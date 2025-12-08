#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <algorithm>
#include <cstring>
#include <map>
#include <functional>

using namespace std;

// --- BPE Structure Definitions ---

// Stores the mapping: New Byte maps to (first, second) pair.
struct DictionaryEntry
{
    unsigned char first;
    unsigned char second;
};

// Global container to store the replacement dictionary
extern vector<DictionaryEntry> BPE_DICTIONARY;

// --- BPE Helper Functions ---
vector<unsigned char> readFileToBytes(const string &filename);
bool writeCompressedFile(const string &filename, const vector<unsigned char> &data, const vector<DictionaryEntry> &dictionary);
bool oneIterationBPE(vector<unsigned char> &data, unsigned char &nextFreeByte);
vector<unsigned char> stringToBytes(const string &str);
string bytesToString(const vector<unsigned char> &bytes);

// --- XML Processing Functions ---
string verify(const string &xml);
string format(const string &xml);
string json(const string &xml);
string mini(const string &xml);
string compress(const string &xml);
string decompress(const string &xml);
string draw(const string &xml);
string fixation(const string &xml);
string most_active(const string &xml);
string most_influencer(const string &xml);
string mutual(const string &xml);
string suggest(const string &xml);
string search(const string &xml);

#endif