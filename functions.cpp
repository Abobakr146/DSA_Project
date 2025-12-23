#include "functions.h"
#include "graph.h"
using namespace std;

// ==================== Implement Post class functions ====================
// Constructor implementation
Post::Post(vector<string> topics, string content) {
    post_Topics = topics;
    post_Content = content;
}

// Method implementations
vector<string> Post::getTopics() {
    return post_Topics;
}

string Post::getContent() {
    return post_Content;
}

// Global BPE Dictionary
vector<DictionaryEntry> BPE_DICTIONARY;

// ==================== BPE Helper Functions ====================

vector<unsigned char> stringToBytes(const string &str)
{
    return vector<unsigned char>(str.begin(), str.end());
}

string bytesToString(const vector<unsigned char> &bytes)
{
    return string(bytes.begin(), bytes.end());
}

vector<unsigned char> readFileToBytes(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Could not open input file " << filename << endl;
        return {};
    }
    return vector<unsigned char>((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

bool writeCompressedFile(const string &filename, const vector<unsigned char> &data, const vector<DictionaryEntry> &dictionary)
{
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open())
    {
        cerr << "Error: Could not open output file " << filename << endl;
        return false;
    }

    // Write the number of dictionary entries
    size_t dict_size = dictionary.size();
    outfile.write((const char *)&dict_size, sizeof(dict_size));

    // Write the dictionary entries
    for (const auto &entry : dictionary)
    {
        outfile.write((const char *)&entry.first, sizeof(entry.first));
        outfile.write((const char *)&entry.second, sizeof(entry.second));
    }

    // Write compressed data
    outfile.write((const char *)data.data(), data.size());

    outfile.close();
    return true;
}

bool oneIterationBPE(vector<unsigned char> &data, unsigned char &nextFreeByte)
{

    if (BPE_DICTIONARY.size() >= 128)
    {
        return false;
    }

    if (data.size() < 2 || nextFreeByte > 255)
    {
        return false;
    }

    // PairCount[i][j] stores the count of the pair (char)i followed by (char)j.
    int pairCount[256][256] = {0};

    int maxCount = 0;
    unsigned char max_i = 0, max_j = 0;

    // 1. Count Frequencies
    for (size_t k = 0; k < data.size() - 1; ++k)
    {
        pairCount[data[k]][data[k + 1]]++;
    }

    // 2. Find Max Pair
    for (int i = 0; i < 256; ++i)
    {
        for (int j = 0; j < 256; ++j)
        {
            // A pair must appear at least twice to be a "compression" benefit (maxCount > 1).
            if (pairCount[i][j] > maxCount)
            {
                maxCount = pairCount[i][j];
                max_i = (unsigned char)i;
                max_j = (unsigned char)j;
            }
        }
    }

    if (maxCount < 2)
    {
        return false;
    }

    // 3. Perform Replacement
    BPE_DICTIONARY.push_back({max_i, max_j});
    unsigned char replacementByte = nextFreeByte;

    vector<unsigned char> new_data;
    for (size_t k = 0; k < data.size(); ++k)
    {
        if (k + 1 < data.size() && data[k] == max_i && data[k + 1] == max_j)
        {
            new_data.push_back(replacementByte); // Append single replacement byte
            k++;                                 // Skip the next character
        }
        else
        {
            new_data.push_back(data[k]); // Append original character
        }
    }

    data = new_data;
    nextFreeByte++;

    return true;
}

// ==================== XML Processing Functions ====================

string verify(const string &xml) {
    stack<string> tagStack;
    string result = "";
    int i = 0;
    int lineNum = 1;
    int numberOfErrors = 0;
    
    while (i < xml.length()) {
        if (xml[i] == '\n') {
            lineNum++;
        }
        
        if(xml[i] == '>') {
            // Check if this '>' has a matching '<' before it
            bool hasMatchingOpen = false;
            int searchPos = i - 1;

            // Search backwards for '<'
            while (searchPos >= 0 && xml[searchPos] != '<') {
                if (xml[searchPos] == '<') {
                    hasMatchingOpen = true;
                    break;
                }
                if (xml[searchPos] == '>') {
                    // Found another '>' first, so this '>' is orphaned
                    break;
                }
                searchPos--;
            }

            if (!hasMatchingOpen) {
                result += "Error at line " + to_string(lineNum) + ": Missing '<' for '>'\n";
                numberOfErrors++;
            }
            i++;

        } else if (xml[i] == '<') {
            int nextOpenTag = xml.find('<', i + 1);
            int tagEnd = xml.find_first_of('>', i);
            
            if ((nextOpenTag != string::npos && nextOpenTag < tagEnd) || tagEnd == string::npos) {
                result += "Error at line " + to_string(lineNum) + ": Unclosed tag bracket\n";
                numberOfErrors++;
                i++;  // Move forward to avoid infinite loop
                continue;
            }
            
            string tagContent = xml.substr(i + 1, tagEnd - i - 1);
            
            if (tagContent[0] == '?') {
                // XML declaration must end with '?'
                if (tagContent.back() != '?') {
                    result += "Error at line " + to_string(lineNum) + ": Malformed XML declaration\n";
                    numberOfErrors++;
                }
                i = tagEnd + 1;
                continue;
            }
            
            if (tagContent[0] == '!' || tagContent.back() == '/') {
                // skip comments and self-closing tags
                i = tagEnd + 1;
                continue;
            }
            
            if (tagContent[0] == '/') {
                // closing tag
                string closingTag = tagContent.substr(1);
                int spaceIndex = closingTag.find(' ');
                if (spaceIndex != string::npos) {
                    closingTag = closingTag.substr(0, spaceIndex);
                }
                
                if (tagStack.empty()) {
                    result += "Error at line " + to_string(lineNum) + ": No matching opening tag for </" + closingTag + ">\n";
                    numberOfErrors++;
                } else {
                    string expectedTag = tagStack.top();
                    if (expectedTag != closingTag) {
                        result += "Error at line " + to_string(lineNum) + ": Mismatched tags\n";
                        numberOfErrors++;
                    } else {
                        tagStack.pop();
                    }
                }
            } else {
                // opening tag
                string openTag = tagContent;
                int spaceIndex = openTag.find(' ');
                if (spaceIndex != string::npos) {
                    openTag = openTag.substr(0, spaceIndex);
                }
                tagStack.push(openTag);
            }
            
            i = tagEnd + 1;
        } else {
            i++;
        }
    }
    
    // check for unclosed tags
    if (!tagStack.empty()) {
        result += "Error: Unclosed tags found:\n";
        while (!tagStack.empty()) {
            result += "  - <" + tagStack.top() + ">\n";
            tagStack.pop();
            numberOfErrors++;
        }
    }
    
    // final output to console
    if (numberOfErrors == 0) {
        result = "Valid";
        cout << result << endl;
    } else {
        result = "Invalid\nTotal Errors: " + to_string(numberOfErrors) + "\n" + result;
        cout << result << endl;
    }
    
    return xml;
}

string fixation(const string &xml) {
    cout << "begin fixing\n";

    // XML has errors - fix the errors

    string fixedXml = xml;  // work on a copy
    stack<string> tagStack;
    stack<int> tagPositions;  // track positions for inserting closing tags
    int i = 0;
    int lineNum = 1;
    
    while (i < fixedXml.length()) {
        if (fixedXml[i] == '\n') {
            lineNum++;
        }
        
        if (fixedXml[i] == '<') {
            int nextOpenTag = fixedXml.find('<', i + 1);
            int tagEnd = fixedXml.find_first_of('>', i);
            int previousCloseBracket = fixedXml.rfind('>', i - 1);
            
            // fix 1: unclosed tag bracket
            // fix 1: unclosed tag bracket
            if ((nextOpenTag != string::npos && nextOpenTag < tagEnd) || tagEnd == string::npos) {
                cout << "Missing close bracket\n";
                if (tagEnd == string::npos) {
                    // no '>' found at all - find where tag name/attributes end
                    // look for whitespace or newline after the tag name
                    cout << "Add missing close bracket at file end\n";
                    int insertPos = i + 1;
                    while (insertPos < fixedXml.length() && 
                            fixedXml[insertPos] != ' ' && 
                            fixedXml[insertPos] != '\n' && 
                            fixedXml[insertPos] != '\t' &&
                            fixedXml[insertPos] != '<') {
                        insertPos++;
                    }
                    fixedXml.insert(insertPos, ">");
                    tagEnd = insertPos;
                    cout << "Finished adding missing close bracket at file end\n";
                } else {
                    // there's a '<' before the '>'
                    // insert '>' right after tag name/attributes, not at nextOpenTag
                    cout << "Add missing close bracket at line: " << lineNum << "\n";
                    int insertPos = i + 1;
                    while (insertPos < nextOpenTag && 
                            fixedXml[insertPos] != '\n' && 
                            fixedXml[insertPos] != '<') {
                        insertPos++;
                    }
                    // back up to last non-whitespace character
                    while (insertPos > i + 1 && 
                            (fixedXml[insertPos - 1] == ' ' || 
                            fixedXml[insertPos - 1] == '\t')) {
                        insertPos--;
                    }
                    fixedXml.insert(insertPos, ">");
                    tagEnd = insertPos;
                    cout << "Finished adding missing close bracket at line: " << lineNum << "\n";
                }
            }
            
            string tagContent = fixedXml.substr(i + 1, tagEnd - i - 1);
            
            // fix 2: malformed XML declaration
            if (tagContent[0] == '?') {
                if (tagContent.back() != '?') {
                    cout << "Malformed XML declaration found\n";
                    fixedXml.insert(tagEnd, "?");
                    tagEnd++;
                    cout << "Fixed Malformed XML declaration\n";
                }
                i = tagEnd + 1;
                continue;
            }
            
            // skip comments and self-closing tags
            if (tagContent[0] == '!' || tagContent.back() == '/') {
                cout << "skip comments and self-closing tags\n";
                i = tagEnd + 1;
                continue;
            }
            
            // handle closing tags
            if (tagContent[0] == '/') {
                string closingTag = tagContent.substr(1);
                int spaceIndex = closingTag.find(' ');
                if (spaceIndex != string::npos) {
                    closingTag = closingTag.substr(0, spaceIndex);
                }
                
                // fix 3: extra closing tag without opening tag
                if (tagStack.empty()) {
                    // remove the extra closing tag
                    fixedXml.erase(i, tagEnd - i + 1);
                    continue;  // don't increment i, as we removed characters
                }
                
                string expectedTag = tagStack.top();
                
                // fix 4: mismatched closing tag
                if (expectedTag != closingTag) {
                    // replace wrong closing tag with correct one
                    string correctClosing = "</" + expectedTag + ">";
                    fixedXml.replace(i, tagEnd - i + 1, correctClosing);
                    tagEnd = i + correctClosing.length() - 1;
                }
                
                tagStack.pop();
                if (!tagPositions.empty()) {
                    tagPositions.pop();
                }
            } 
            // handle opening tags
            else {
                string openTag = tagContent;
                int spaceIndex = openTag.find(' ');
                if (spaceIndex != string::npos) {
                    openTag = openTag.substr(0, spaceIndex);
                }
                tagStack.push(openTag);
                tagPositions.push(tagEnd + 1);
            }
            
            i = tagEnd + 1;
        } else if (fixedXml[i] == '>') {
            // check if this '>' has a matching '<' before it
            bool hasMatchingOpen = false;
            int searchPos = i - 1;
    
            // search backwards for '<'
            while (searchPos >= 0 && fixedXml[searchPos] != '<') {
                if (fixedXml[searchPos] == '<') {
                    hasMatchingOpen = true;
                    break;
                }
                if (fixedXml[searchPos] == '>') {
                    // found another '>' first, so this '>' is orphaned
                    break;
                }
                searchPos--;
            }
    
            if (!hasMatchingOpen) {
                cout << "Missing '<' for '>' at line: " << lineNum << "\n";
                bool isClosingTag = false;
        
                // find where tag name starts (backwards from '>')
                int tagStart = i - 1;
                while (tagStart > 0 && 
                        fixedXml[tagStart] != '\n' &&
                        fixedXml[tagStart] != '>' &&
                        fixedXml[tagStart] != '<' &&
                        fixedXml[tagStart] != '/') {
                    tagStart--;
                }
                // Move forward to the first non-whitespace character after the found position and stop at '/' if encountered
                while (tagStart < i && 
                        (fixedXml[tagStart] == ' ' ||
                        fixedXml[tagStart] == '\t' ||
                        fixedXml[tagStart] == '\n' ||
                        fixedXml[tagStart] == '/')) {
                    if(fixedXml[tagStart] == '/') {
                        // if we hit a '/' before finding '<', it means it's a closing tag
                        // so we should insert '<' before the '/'
                        isClosingTag = true;
                        break;
                    } else {
                        tagStart++;
                    }
                }
        
                fixedXml.insert(tagStart, "<");
                i++; // Adjust position after insertion

                // push open tag in stack
                if (!isClosingTag) {
                    // extract tag name
                    int tagEnd = fixedXml.find_first_of('>', tagStart);
                    string tagContent = fixedXml.substr(tagStart + 1, tagEnd - tagStart - 1);
                    int spaceIndex = tagContent.find(' ');
                    if (spaceIndex != string::npos) {
                        tagContent = tagContent.substr(0, spaceIndex);
                    }
                    tagStack.push(tagContent);
                    tagPositions.push(tagEnd + 1);
                } else {
                    // it's a closing tag, so pop from stack
                    if (!tagStack.empty()) {
                        tagStack.pop();
                        if (!tagPositions.empty()) {
                            tagPositions.pop();
                        }
                    }
                }
        
                cout << "Added missing '<'\n";
            }
            i++;
        } else {
            i++;
        }
    }
    
    // fix 5: add missing closing tags at the end
    while (!tagStack.empty()) {
        cout << "Adding missing closing tag for <" << tagStack.top() << ">\n";
        string missingTag = tagStack.top();
        tagStack.pop();
        fixedXml += "</" + missingTag + ">";
    }

    if(fixedXml == xml) {
        cout << "No fixes were necessary.\n";
    } else {
        cout << "Fixing complete.\n";
    }
    
    return fixedXml;
}

string trim_copy(const string &s)
{
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

string extract_tag_name(const string &tag)
{
    size_t i = 0;
    if (tag[i] == '/') i++;
    while (i < tag.size() && isspace((unsigned char)tag[i])) i++;

    size_t start = i;
    while (i < tag.size() && !isspace((unsigned char)tag[i]) && tag[i] != '/')
        i++;

    return tag.substr(start, i - start);
}


string format(const string &xml)
{
    string out;
    stack<string> st;
    const string indent = "  ";
    size_t i = 0, n = xml.size();

    while (i < n) {

        // skip whitespace
        if (isspace((unsigned char)xml[i])) {
            i++;
            continue;
        }

        // ================= OPENING TAG =================
        if (xml[i] == '<' && i + 1 < n && xml[i + 1] != '/') {

            // find tag end
            size_t j = xml.find('>', i);
            string tag = xml.substr(i, j - i + 1);
            string tagContent = xml.substr(i + 1, j - i - 1);

            string tagName = extract_tag_name(tagContent);

            // look ahead for INLINE case
            size_t textStart = j + 1;
            size_t nextTag = xml.find('<', textStart);

            bool inlineCase = false;
            string text;

            if (nextTag != string::npos &&
                xml.compare(nextTag, 2, "</") == 0) {

                text = trim_copy(xml.substr(textStart, nextTag - textStart));

                size_t closeEnd = xml.find('>', nextTag);
                string closingName = extract_tag_name(
                    xml.substr(nextTag + 2, closeEnd - nextTag - 2));

                if (!text.empty() && closingName == tagName)
                    inlineCase = true;
            }

            // INLINE TAG
            if (inlineCase) {
                for (size_t k = 0; k < st.size(); k++) out += indent;
                out += "<" + tagName + ">" + text + "</" + tagName + ">\n";

                // skip text + closing tag
                i = xml.find('>', nextTag) + 1;
                continue;
            }

            // NORMAL OPENING TAG
            for (size_t k = 0; k < st.size(); k++) out += indent;
            out += tag + "\n";
            st.push(tagName);

            i = j + 1;
            continue;
        }

        // ================= CLOSING TAG =================
        if (xml[i] == '<' && i + 1 < n && xml[i + 1] == '/') {
            size_t j = xml.find('>', i);
            string tag = xml.substr(i, j - i + 1);

            if (!st.empty()) st.pop();

            for (size_t k = 0; k < st.size(); k++) out += indent;
            out += tag + "\n";

            i = j + 1;
            continue;
        }

        // ================= MULTI-LINE TEXT =================
        size_t nextTag = xml.find('<', i);
        if (nextTag == string::npos) nextTag = n;

        string text = trim_copy(xml.substr(i, nextTag - i));
        if (!text.empty()) {
            for (size_t k = 0; k < st.size(); k++) out += indent;
            out += text + "\n";
        }

        i = nextTag;
    }

    return out;
}

// Helper to generate indentation spaces based on depth
string getIndent(int level)
{
    return string(level * 4, ' '); // 4 spaces per level
}

struct JsonXMLNode
{
    string name;
    string content;
    vector<JsonXMLNode *> children;
    JsonXMLNode(string n) : name(n), content("") {} // new way to implement the constructor, it will intialize the values while making the object

    ~JsonXMLNode()
    {
        for (auto child : children)
        {
            delete child;
        }
    }
};

string trim(const string &str)
{ //  عملت ال function دي عشان ميسجلش ال \n والt وال \r
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

JsonXMLNode *parseXML(const string &xml)
{
    JsonXMLNode *root = nullptr;
    stack<JsonXMLNode *> nodeStack;
    size_t pos = 0;
    while (pos < xml.length())
    {
        size_t lt = xml.find('<', pos);
        if (lt == string::npos)
            break;
        if (lt > pos)
        {
            string text = trim(xml.substr(pos, lt - pos));
            if (!text.empty() && !nodeStack.empty())
            {
                nodeStack.top()->content = text;
            }
        }
        size_t gt = xml.find('>', lt);
        if (gt == string::npos)
            break;
        string tagContent = xml.substr(lt + 1, gt - lt - 1);
        if (tagContent[0] == '/')
        {
            if (!nodeStack.empty())
            {
                nodeStack.pop();
            }
        }
        else
        {
            string tagName = tagContent;

            JsonXMLNode *newNode = new JsonXMLNode(tagName);

            if (nodeStack.empty())
            {
                root = newNode;
            }
            else
            {
                nodeStack.top()->children.push_back(newNode); // هنا سواء انا كنت parent او
                // children ما دام في children جديدة جت هيبقا اخر حاجة موجود في ال node stack هو ال parent بتاعها
            }

            nodeStack.push(newNode);
        }

        pos = gt + 1;
    }

    return root;
}

void nodeToJSON(JsonXMLNode *node, stringstream &ss, int level)
{
    if (!node)
        return;

    // Case 1: It's a leaf node (just text content, no children)
    if (node->children.empty())
    {
        ss << "\"" << node->content << "\"";
        return;
    }

    // Case 2: It's an object (has children)
    ss << "{\n";

    map<string, vector<JsonXMLNode *>> groups;
    vector<string> order;

    // Group children by tag name to handle arrays
    for (auto child : node->children)
    {
        if (groups.find(child->name) == groups.end())
        {
            order.push_back(child->name);
        }
        groups[child->name].push_back(child);
    }

    // Iterate through the grouped children
    for (size_t i = 0; i < order.size(); ++i)
    {
        string key = order[i];
        const auto &list = groups[key];

        // Print Indentation + Key
        ss << getIndent(level + 1) << "\"" << key << "\": ";

        if (list.size() > 1)
        {
            // --- Handle Arrays ---
            ss << "[\n";
            for (size_t k = 0; k < list.size(); ++k)
            {
                ss << getIndent(level + 2); // Indent array items further
                nodeToJSON(list[k], ss, level + 2);

                if (k < list.size() - 1)
                    ss << ",\n"; // Comma between items
                else
                    ss << "\n";
            }
            ss << getIndent(level + 1) << "]";
        }
        else
        {
            // --- Handle Single Objects ---
            nodeToJSON(list[0], ss, level + 1);
        }

        // Comma between keys (if not the last one)
        if (i < order.size() - 1)
            ss << ",\n";
        else
            ss << "\n";
    }

    // Closing brace with proper indentation
    ss << getIndent(level) << "}";
}

string json(const string &xml)
{
    JsonXMLNode *root = parseXML(xml);
    if (!root)
        return "{}";

    stringstream ss;
    ss << "{\n";

    // Fixed a small bug here: You had a space inside the quote "\" " which made keys look like " users"
    ss << getIndent(1) << "\"" << root->name << "\": ";

    nodeToJSON(root, ss, 1); // Start recursion at level 1

    ss << "\n}"; // Close the main object

    delete root;
    return ss.str();
}

string mini(const string &xml)
{
    string output = "";

    bool inTag = false;
    bool inText = false;

    for (char c : xml)
    {
        if (c == '<')
        {
            inTag = true;
            inText = false;
            output += c;
        }
        else if (c == '>')
        {
            inTag = false;
            inText = true;
            output += c;
        }
        else
        {
            if (inTag)
            {
                if (c != ' ' && c != '\n' && c != '\t' && c != '\r')
                    output += c;
            }
            else if (inText)
            {
                if (c == '\n' || c == '\t' || c == '\r')
                    continue;

                // Simulate output.back() without using back()
                bool lastIsSpace = false;
                bool lastIsTag = false;

                if (output.length() > 0)
                {
                    char last = output[output.length() - 1];
                    lastIsSpace = (last == ' ');
                    lastIsTag = (last == '>');
                }

                if (!(c == ' ' && (lastIsSpace || lastIsTag)))
                    output += c;
            }
        }
    }

    return output;
}

string compress(const string &xml)
{
    // Convert string to bytes
    vector<unsigned char> data = stringToBytes(xml);

    if (data.empty())
    {
        cerr << "Error: Empty input for compression" << endl;
        return "";
    }

    // Clear dictionary and perform BPE
    BPE_DICTIONARY.clear();
    unsigned char next_free = 128; // Start replacement symbols at 128

    // Loop until no more pairs are found or we run out of free bytes (255)
    while (oneIterationBPE(data, next_free))
        ;

    // Build binary output
    stringstream result;

    // Write dictionary size (as binary)
    size_t dict_size = BPE_DICTIONARY.size();
    result.write(reinterpret_cast<const char *>(&dict_size), sizeof(dict_size));

    // Write dictionary entries (raw bytes)
    for (const auto &entry : BPE_DICTIONARY)
    {
        result.write(reinterpret_cast<const char *>(&entry.first), sizeof(entry.first));
        result.write(reinterpret_cast<const char *>(&entry.second), sizeof(entry.second));
    }

    // Write compressed data (raw bytes)
    result.write(reinterpret_cast<const char *>(data.data()), data.size());

    size_t total_size = sizeof(dict_size) + (BPE_DICTIONARY.size() * 2) + data.size();
    cout << "Compression complete. Original size: " << xml.length()
         << " bytes, Compressed size: " << total_size
         << " bytes (Saved: " << (int)xml.length() - (int)total_size << " bytes)" << endl;

    return result.str();
}

string decompress(const string &xml)
{
    // 1. Basic Validation
    if (xml.empty())
    {
        cerr << "Error: Empty input passed to decompress function!" << endl;
        return "";
    }

    // Print input size to debug "Text Mode" reading issues
    cout << "Debug: Decompress received " << xml.size() << " bytes." << endl;

    stringstream ss(xml);

    // 2. Read Dictionary Size
    size_t dict_size;
    ss.read(reinterpret_cast<char *>(&dict_size), sizeof(dict_size));

    if (ss.fail())
    {
        cerr << "Error: Failed to read dictionary size. Input too short?" << endl;
        return "";
    }

    cout << "Debug: Dictionary size is " << dict_size << " entries." << endl;

    // 3. Reconstruct Dictionary
    vector<pair<unsigned char, unsigned char>> dict;
    dict.reserve(dict_size);

    for (size_t i = 0; i < dict_size; ++i)
    {
        unsigned char first, second;
        ss.read(reinterpret_cast<char *>(&first), sizeof(first));
        ss.read(reinterpret_cast<char *>(&second), sizeof(second));
        dict.push_back({first, second});
    }

    // 4. Read Compressed Byte Data
    //  read directly from the current position to the end
    streampos data_start = ss.tellg();

    // Read the rest of the stream into a vector directly
    string remaining_str = xml.substr(data_start);
    vector<unsigned char> data(remaining_str.begin(), remaining_str.end());

    cout << "Debug: Processing " << data.size() << " bytes of compressed data." << endl;

    // 5. Recursive Expansion Logic
    string output;

    // Recursive lambda
    function<void(unsigned char)> expand = [&](unsigned char b)
    {
        int index = (int)b - 128;

        if (index >= 0 && index < (int)dict.size())
        {
            expand(dict[index].first);
            expand(dict[index].second);
        }
        else
        {
            output += (char)b;
        }
    };

    // 6. Generate Decompressed String
    for (unsigned char b : data)
    {
        expand(b);
    }

    cout << "Debug: Decompressed output size is " << output.size() << " bytes." << endl;
    return output;
}

string draw(const string &xml)
{
    return "";
}

string most_active(const string &xml)
{  
    
    struct User {
        string id;
        string name;
        int followCount = 0;
    };

    vector<User> users;

    // -------- 1) Read users (id + name) --------
    int i = 0;
    while (i < xml.length()) {

        if (xml.substr(i, 6) == "<user>") {
            i += 6;
            User u;

            while (xml.substr(i, 7) != "</user>") {

                if (xml.substr(i, 4) == "<id>" && u.id.empty()) {
                    i += 4;
                    while (xml.substr(i, 5) != "</id>")
                        u.id += xml[i++];
                    i += 5;
                }
                else if (xml.substr(i, 6) == "<name>") {
                    i += 6;
                    while (xml.substr(i, 7) != "</name>")
                        u.name += xml[i++];
                    i += 7;
                }
                else {
                    i++;
                }
            }

            users.push_back(u);
            i += 7; // </user>
        }
        else {
            i++;
        }
    }

    // -------- 2) Count follow actions (FIXED) --------
    i = 0;
    bool insideFollowers = false;

    while (i < xml.length()) {

        if (xml.substr(i, 11) == "<followers>") {
            insideFollowers = true;
            i += 11;
        }
        else if (xml.substr(i, 12) == "</followers>") {
            insideFollowers = false;
            i += 12;
        }
        else if (insideFollowers && xml.substr(i, 4) == "<id>") {
            i += 4;
            string fid;
            while (xml.substr(i, 5) != "</id>")
                fid += xml[i++];
            i += 5;

            for (auto &u : users)
                if (u.id == fid)
                    u.followCount++;
        }
        else {
            i++;
        }
    }

    // -------- 3) Find max --------
    int maxFollow = -1;
    for (auto &u : users)
        if (u.followCount > maxFollow)
            maxFollow = u.followCount;

    // -------- 4) Build result string --------
    string result;
    for (auto &u : users) {
        if (u.followCount == maxFollow && maxFollow > 0) {
            if (!result.empty()) result += " | ";
            result += "ID: " + u.id + ", Name: " + u.name;
        }
    }

    return result;
}

string most_influencer(const string &xml)
{    
    struct User {
        string id;
        string name;
    };
    
    vector<User> topUsers;
    int maxFollowers = -1;

    int i = 0;
    while (i < xml.length()) {
        if (xml.substr(i, 6) == "<user>") {
            i += 6;

            string id = "";
            string name = "";
            int followerCount = 0;
            bool idRead = false;
            bool nameRead = false;

            while (!(xml.substr(i, 7) == "</user>")) {
                if (!idRead && xml.substr(i, 4) == "<id>") {
                    i += 4;
                    while (!(xml.substr(i, 5) == "</id>")) {
                        id += xml[i++];
                    }
                    i += 5;
                    idRead = true;
                } else if (!nameRead && xml.substr(i, 6) == "<name>") {
                    i += 6;
                    while (!(xml.substr(i, 7) == "</name>")) {
                        name += xml[i++];
                    }
                    i += 7;
                    nameRead = true;
                } else if (xml.substr(i, 10) == "<follower>") {
                    followerCount++;
                    i += 10;
                } else {
                    i++;
                }
            }

            if (followerCount > maxFollowers) {
                topUsers.clear();
                topUsers.push_back({id, name});
                maxFollowers = followerCount;
            } else if (followerCount == maxFollowers) {
                topUsers.push_back({id, name});
            }

            i += 7; // skip </user>
        } else {
            i++;
        }
    }

    // Convert vector of users to single string
    string result;
    for (size_t j = 0; j < topUsers.size(); j++) {
        result += "ID: " + topUsers[j].id + ", Name: " + topUsers[j].name;
        if (j != topUsers.size() - 1)
            result += " | ";  // separator between users
    }

    return result;
}

string mutual(const string &xml, const vector<int> &ids)
{
    // --------- HANDLE EDGE CASES ---------
    if (ids.empty()) {
        return "Error: No user IDs provided.";
    }

    map<int, vector<int>> followers;

    // --------- PARSE XML ---------
    size_t pos = 0;
    while ((pos = xml.find("<user>", pos)) != string::npos) {

        // Find the end of this user block first to ensure we advance properly
        size_t userEnd = xml.find("</user>", pos);
        if (userEnd == string::npos) {
            break;  // No closing tag, exit to avoid infinite loop
        }

        size_t idStart = xml.find("<id>", pos);
        size_t idEnd   = xml.find("</id>", pos);
        
        // Validate id tags exist and are within this user block
        if (idStart == string::npos || idEnd == string::npos || idStart > userEnd) {
            pos = userEnd + 7;  // Move past </user>
            continue;
        }
        
        idStart += 4;  // Move past <id>
        int userId = stoi(xml.substr(idStart, idEnd - idStart));

        size_t followersStart = xml.find("<followers>", pos);
        size_t followersEnd   = xml.find("</followers>", pos);

        // Only process followers if both tags exist and are within this user block
        if (followersStart != string::npos && followersEnd != string::npos && 
            followersStart < userEnd && followersEnd < userEnd) {
            
            size_t fpos = followersStart;
            while ((fpos = xml.find("<follower>", fpos)) != string::npos &&
                   fpos < followersEnd) {

                size_t fStart = fpos + 10;  // Move past <follower>
                size_t fEnd = xml.find("</follower>", fStart);
                
                if (fEnd == string::npos || fEnd > followersEnd) {
                    break;
                }
                
                // Get the content between <follower> and </follower>
                string followerContent = xml.substr(fStart, fEnd - fStart);
                
                // Check if there's a nested <id> tag or direct content
                size_t nestedIdStart = followerContent.find("<id>");
                if (nestedIdStart != string::npos) {
                    // Nested id format: <follower><id>X</id></follower>
                    size_t nestedIdEnd = followerContent.find("</id>");
                    if (nestedIdEnd != string::npos) {
                        string idStr = followerContent.substr(nestedIdStart + 4, nestedIdEnd - nestedIdStart - 4);
                        int followerId = stoi(idStr);
                        followers[userId].push_back(followerId);
                    }
                } else {
                    // Direct content format: <follower>X</follower>
                    // Trim whitespace
                    size_t start = followerContent.find_first_not_of(" \t\n\r");
                    size_t end = followerContent.find_last_not_of(" \t\n\r");
                    if (start != string::npos && end != string::npos) {
                        string idStr = followerContent.substr(start, end - start + 1);
                        int followerId = stoi(idStr);
                        followers[userId].push_back(followerId);
                    }
                }

                fpos = fEnd + 11;  // Move past </follower>
            }
        }

        pos = userEnd + 7;  // Move past </user> to next user
    }

    // --------- HANDLE SINGLE USER CASE ---------
    if (ids.size() == 1) {
        int singleId = ids[0];
        if (followers.find(singleId) == followers.end() || followers[singleId].empty()) {
            return "User " + to_string(singleId) + " has no followers.";
        }
        
        stringstream out;
        out << "Followers of user " << singleId << ":\n";
        for (int u : followers[singleId])
            out << "User ID: " << u << "\n";
        return out.str();
    }

    // --------- FIND MUTUAL FOLLOWERS ---------
    vector<int> result = followers[ids[0]];

    for (size_t i = 1; i < ids.size(); i++) {
        vector<int> temp;
        for (int u : result) {
            if (find(followers[ids[i]].begin(),
                     followers[ids[i]].end(),
                     u) != followers[ids[i]].end()) {
                temp.push_back(u);
            }
        }
        result = temp;
    }

    // --------- FORMAT OUTPUT ---------
    if (result.empty())
        return "No mutual followers found.";

    stringstream out;
    out << "Mutual followers between users ";
    for (size_t i = 0; i < ids.size(); i++) {
        out << ids[i];
        if (i < ids.size() - 2)
            out << ", ";
        else if (i == ids.size() - 2)
            out << ", and ";
    }
    out << ":\n";
    for (int u : result)
        out << "User ID: " << u << "\n";

    return out.str();
}


string suggest(const string &xml, int userId)
{
    Graph followersGraph = buildGraphFromXML(xml);
    string result;

    Graph followingGraph;

    for (const auto &entry : followersGraph)
    {
        int followedUser = entry.first;
        const vector<int> &followers = entry.second;

        for (int follower : followers)
        {
            followingGraph[follower].push_back(followedUser);
        }
    }

    if (followingGraph.find(userId) == followingGraph.end())
    {
        return result;
    }

    const vector<int> &myFollowing = followingGraph[userId];

    unordered_map<int, bool> alreadyFollowing;
    for (int id : myFollowing)
    {
        alreadyFollowing[id] = true;
    }
    alreadyFollowing[userId] = true; 

    map<int, int> candidateScores;

    for (int friendId : myFollowing)
    {
        if (followingGraph.find(friendId) != followingGraph.end())
        {
            const vector<int> &friendsFollowing = followingGraph[friendId];
            for (int candidateId : friendsFollowing)
            {
                if (alreadyFollowing.find(candidateId) == alreadyFollowing.end())
                {
                    candidateScores[candidateId]++;
                }
            }
        }
    }

    vector<pair<int, int>> sortedCandidates;
    for (const auto &entry : candidateScores)
    {
        sortedCandidates.push_back(entry);
    }

    sort(sortedCandidates.begin(), sortedCandidates.end(),
         [](const pair<int, int> &a, const pair<int, int> &b)
         {
             return a.second > b.second; // Higher score first
         });

    for (const auto &candidate : sortedCandidates)
    {
        result += to_string(candidate.first) + "\n";
    }

    return result;
}

vector<string> searchPostsByWord(const string& xml, const string& word) {
    vector<string> results;

    const string post_open  = "<post>";
    const string post_close = "</post>";
    const string body_open  = "<body>";
    const string body_close = "</body>";

    size_t pos = 0;

    while (true) {
        size_t post_start = xml.find(post_open, pos);
        if (post_start == string::npos) break;

        size_t post_end = xml.find(post_close, post_start);
        if (post_end == string::npos) break;

        string post_block = xml.substr(
            post_start,
            post_end - post_start
        );

        size_t body_start = post_block.find(body_open);
        size_t body_end   = post_block.find(body_close);

        if (body_start != string::npos &&
            body_end   != string::npos &&
            body_end > body_start) {

            body_start += body_open.length();
            string body = post_block.substr(
                body_start,
                body_end - body_start
            );

            if (body.find(word) != string::npos) {
                results.push_back(body);
            }
        }

        pos = post_end + post_close.length();
    }

    return results;
}

vector<string> searchPostsByTopic(const string& xml, const string& topic) {
    vector<string> results;

    const string post_open  = "<post>";
    const string post_close = "</post>";
    const string body_open  = "<body>";
    const string body_close = "</body>";

    const string topic_tag =
        "<topic>" + topic + "</topic>";

    size_t pos = 0;

    while ((pos = xml.find(post_open, pos)) != string::npos) {
        size_t post_end = xml.find(post_close, pos);
        if (post_end == string::npos) break;

        string post_block =
            xml.substr(pos, post_end - pos);

        if (post_block.find(topic_tag) != string::npos) {
            size_t body_start = post_block.find(body_open);
            size_t body_end   = post_block.find(body_close);

            if (body_start != string::npos &&
                body_end   != string::npos) {

                body_start += body_open.length();
                results.push_back(
                    post_block.substr(body_start, body_end - body_start)
                );
            }
        }

        pos = post_end + post_close.length();
    }

    return results;
}

vector<int> strIDs2int(const string &ids) {
    vector<int> Ids;
    stringstream ss(ids);
    string segment;

    // Use getline to split the string by the comma delimiter
    while (getline(ss, segment, ',')) {
        // Convert the string segment to an integer and push it to the vector
        Ids.push_back(stoi(segment));
    }

    return Ids;
}

