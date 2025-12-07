#include "functions.h"
#include <stack>
#include <sstream>
#include <cstring>

using namespace std;

// Global BPE Dictionary
vector<DictionaryEntry> BPE_DICTIONARY;

// ==================== BPE Helper Functions ====================

vector<unsigned char> stringToBytes(const string& str) {
    return vector<unsigned char>(str.begin(), str.end());
}

string bytesToString(const vector<unsigned char>& bytes) {
    return string(bytes.begin(), bytes.end());
}

vector<unsigned char> readFileToBytes(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Could not open input file " << filename << endl;
        return {};
    }
    return vector<unsigned char>((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

bool writeCompressedFile(const string& filename, const vector<unsigned char>& data, const vector<DictionaryEntry>& dictionary) {
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) {
        cerr << "Error: Could not open output file " << filename << endl;
        return false;
    }

    // Write the number of dictionary entries
    size_t dict_size = dictionary.size();
    outfile.write((const char*)&dict_size, sizeof(dict_size));

    // Write the dictionary entries
    for (const auto& entry : dictionary) {
        outfile.write((const char*)&entry.first, sizeof(entry.first));
        outfile.write((const char*)&entry.second, sizeof(entry.second));
    }

    // Write compressed data
    outfile.write((const char*)data.data(), data.size());

    outfile.close();
    return true;
}

bool oneIterationBPE(vector<unsigned char>& data, unsigned char& nextFreeByte) {
    if (data.size() < 2 || nextFreeByte > 255) {
        return false;
    }

    // PairCount[i][j] stores the count of the pair (char)i followed by (char)j.
    int pairCount[256][256] = {0};
    
    int maxCount = 0;
    unsigned char max_i = 0, max_j = 0;

    // 1. Count Frequencies
    for (size_t k = 0; k < data.size() - 1; ++k) {
        pairCount[data[k]][data[k+1]]++;
    }

    // 2. Find Max Pair
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 256; ++j) {
            // A pair must appear at least twice to be a "compression" benefit (maxCount > 1).
            if (pairCount[i][j] > maxCount) {
                maxCount = pairCount[i][j];
                max_i = (unsigned char)i;
                max_j = (unsigned char)j;
            }
        }
    }

    if (maxCount < 2) {
        return false; 
    }

    // 3. Perform Replacement
    BPE_DICTIONARY.push_back({max_i, max_j});
    unsigned char replacementByte = nextFreeByte;

    vector<unsigned char> new_data;
    for (size_t k = 0; k < data.size(); ++k) {
        if (k + 1 < data.size() && data[k] == max_i && data[k+1] == max_j) {
            new_data.push_back(replacementByte); // Append single replacement byte
            k++; // Skip the next character
        } else {
            new_data.push_back(data[k]); // Append original character
        }
    }

    data = new_data;
    nextFreeByte++; 
    
    return true;
}

// ==================== XML Processing Functions ====================

string verify(const string& xml) {
    stack<string> tagStack;
    string result = "XML is valid - all tags are properly matched\n";
    int i = 0;
    int lineNum = 1;
    while(i < xml.length()){
        if (xml[i] == '\n') {
            lineNum++;
        }
        if(xml[i] == '<'){
            int nextOpenTag = xml.find('<', i + 1);
            int tagEnd = xml.find_first_of('>', i);
            if((nextOpenTag != string::npos && nextOpenTag < tagEnd) || tagEnd == string::npos){
                result = "Error at line " + to_string(lineNum) + ": Unclosed tag bracket\n";
                return result;
            }
            string tagContent = xml.substr(i + 1, tagEnd - i - 1);
            if(tagContent[0] == '?' || tagContent[0] == '!'){
                // Skip XML declaration and comments
                // Validate XML declaration ends with ?
                if(tagContent.back() != '?') {
                    result = "Error at line " + to_string(lineNum) + ": Malformed XML declaration\n";
                    return result;
                }
                i = tagEnd + 1;
                continue;
            }
            if(tagContent.back() == '/'){
                // Self-closing tag, do nothing
                i = tagEnd + 1;
                continue;
            }
            if(tagContent[0] == '/'){
                string closingTag = tagContent.substr(1);
                int spaceIndex = closingTag.find(' ');
                if(spaceIndex != string::npos){
                    closingTag = closingTag.substr(0, spaceIndex);
                }
                if(tagStack.empty()){
                    result = "Error at line " + to_string(lineNum) + ": No matching opening tag for </" + closingTag + ">\n";
                    return result;
                }
                string expectedTag = tagStack.top();
                if(expectedTag != closingTag){
                    result = "Error at line " + to_string(lineNum) + ": Mismatched tags. Expected </" + expectedTag + "> but found </" + closingTag + ">\n";
                    return result;
                }
                tagStack.pop();
            }
            else{
                string openTag = tagContent;
                int spaceIndex = openTag.find(' ');
                if(spaceIndex != string::npos){
                    openTag = openTag.substr(0, spaceIndex);
                }
                tagStack.push(openTag);
            }
            i = tagEnd + 1;
        }
        else{
            i++;
        }
    }
    return result;
}

string format(const string& xml) {
    return "";
}

string json(const string& xml) {
    return "";
}

string mini(const string& xml) {
    return "";
}

string compress(const string& xml) {
    // Convert string to bytes
    vector<unsigned char> data = stringToBytes(xml);
    
    if (data.empty()) {
        cerr << "Error: Empty input for compression" << endl;
        return "";
    }

    // Clear dictionary and perform BPE
    BPE_DICTIONARY.clear();
    unsigned char next_free = 128; // Start replacement symbols at 128
    
    // Loop until no more pairs are found or we run out of free bytes (255)
    while (oneIterationBPE(data, next_free));

    // Build binary output
    stringstream result;
    
    // Write dictionary size (as binary)
    size_t dict_size = BPE_DICTIONARY.size();
    result.write(reinterpret_cast<const char*>(&dict_size), sizeof(dict_size));
    
    // Write dictionary entries (raw bytes)
    for (const auto& entry : BPE_DICTIONARY) {
        result.write(reinterpret_cast<const char*>(&entry.first), sizeof(entry.first));
        result.write(reinterpret_cast<const char*>(&entry.second), sizeof(entry.second));
    }
    
    // Write compressed data (raw bytes)
    result.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    size_t total_size = sizeof(dict_size) + (BPE_DICTIONARY.size() * 2) + data.size();
    cout << "Compression complete. Original size: " << xml.length() 
         << " bytes, Compressed size: " << total_size 
         << " bytes (Saved: " << (int)xml.length() - (int)total_size << " bytes)" << endl;
    
    return result.str();
}

string decompress(const string& xml) {
    // 1. Basic Validation
    if (xml.empty()) 
         cerr << "Error: Empty input for decompression" << endl;
        return "";

    stringstream ss(xml);

    // 2. Read Dictionary Size
    size_t dict_size;
    ss.read(reinterpret_cast<char*>(&dict_size), sizeof(dict_size));
    
    if (ss.fail()) {
        cerr << "Error: Failed to read dictionary size." << endl;
        return "";
    }

    // 3. Reconstruct Dictionary
    // We use a vector because the keys are implicit. 
    // Index 0 maps to byte 128, Index 1 maps to 129
    vector<pair<unsigned char, unsigned char>> dict;
    dict.reserve(dict_size);

    for (size_t i = 0; i < dict_size; ++i) {
        unsigned char first, second;
        ss.read(reinterpret_cast<char*>(&first), sizeof(first));
        ss.read(reinterpret_cast<char*>(&second), sizeof(second));
        dict.push_back({first, second});
    }

    // 4. Read Compressed Byte Data
    // Get the remaining string starting from the current position
    streampos current_pos = ss.tellg();
    string remaining_data = xml.substr(current_pos);
    
    // Use  helper function to convert the string to a byte vector
    vector<unsigned char> data = stringToBytes(remaining_data);

    // 5. Recursive Expansion Logic
    string output;
    
    // Recursive lambda to handle nested compressions
    // "128" is the starting offset for BPE replacement symbols
    function<void(unsigned char)> expand = [&](unsigned char b) {
        int index = (int)b - 128;

        // Check if 'b' is a replacement symbol (>= 128) AND exists in our dictionary
        if (index >= 0 && index < (int)dict.size()) {
            // It is a symbol: Recursively expand the pair it represents
            expand(dict[index].first);
            expand(dict[index].second);
        } else {
            // It is a literal character: Append to output
            output += (char)b;
        }
    };

    // 6. Generate Decompressed String
    for (unsigned char b : data) {
        expand(b);
    }

    return output;
}

string draw(const string& xml) {
    return "";
}

string fixation(const string& xml) {
    return "";
}

string most_active(const string& xml) {
    return "";
}

string most_influencer(const string& xml) {
    return "";
}

string mutual(const string& xml) {
    return "";
}

string suggest(const string& xml) {
    return "";
}

string search(const string& xml) {
    return "";
}
