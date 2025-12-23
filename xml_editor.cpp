#include "functions.h"
#include "utils.h"
#include "graph.h"

int main(int argc, char* argv[]) {
    string input_path;
    string output_path = "output.xml";
    string xml_content;
    string operation;
    bool fix = false;
    int userId = -1;
    string post_seach;
    bool isWord; // true => word, false => topic
    vector<string> s_posts;
    vector<int> Ids;
    string strIDs;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "-i" && i + 1 < argc) {
            input_path = argv[i + 1];
            i++; 
        }
        else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[i + 1];
            i++; 
        }
        else if (arg == "-id" && i + 1 < argc){
            userId = stoi(argv[i + 1]);
            i++;
        }
        else if (arg == "-f") {
            fix = true;
        }
        else if (arg == "-w") {
            post_seach = argv[i+1];
            i++;
            isWord = true;
        }
        else if (arg == "-t") {
            post_seach = argv[i+1];
            i++;
            isWord = false;
        }
        else if (arg == "-ids") {
            strIDs = argv[i+1];
            i++;
            cout << "strIDs: " << strIDs << endl;
        }
        else if (i == 1) {
            operation = arg;
        }
        
    }

    // Validate input
    if (input_path.empty()) {
        cerr << "Usage: " << argv[0] << " <order> -i <path/to/input.xml> [-o <path/to/output.xml>]" << endl;
        cerr << "Error: Input file path (-i) is missing." << endl;
        return 1;
    }
    
    cout << "Simulating XML Editor with operation: '" << operation << "'" << endl;
    if (operation == "verify") {
        if (fix)
            cout << "With Fixation" << endl;
        else 
            cout << "Without Fixation" << endl;
    }
    
    cout << "Attempting to read XML file: " << input_path << endl;

    // Read input file (binary for decompress, text for everything else)
    bool extractSuccess;
    if (operation == "decompress") {
        extractSuccess = extract_binary_content(input_path, xml_content);
    } else {
        extractSuccess = extract_content(input_path, xml_content);
    }
    
    if (extractSuccess) {
        cout << "\n--- Successfully extracted XML content into 'xml_content' string ---\n" << endl;
    } else {
        return 1;
    }

    // Process based on operation
    string updated_xml;
    if(operation == "verify") {
        updated_xml = verify(xml_content);
        if (fix)
            updated_xml = fixation(updated_xml);
    }
    else if(operation == "format") {
        updated_xml = format(xml_content);
    }
    else if(operation == "json") {
        updated_xml = json(xml_content);
    }
    else if(operation == "mini") {
        updated_xml = mini(xml_content);
    }
    else if(operation == "compress") {
        updated_xml = compress(xml_content);
    }
    else if(operation == "decompress") {
        updated_xml = decompress(xml_content);
    }
    else if (operation == "suggest"){
        if (userId == -1)
        {
            cerr << "Error: User ID (-id) is required for suggest operation." << endl;
            return 1;
        }
        updated_xml = suggest(xml_content, userId);
        // cout << "Suggested users for User " << userId << ":" << endl;
        // cout << updated_xml;
    }
    else if (operation == "search")
    {
        if (isWord == false) {
            s_posts = searchPostsByTopic(xml_content, post_seach);
        }
        else {
            s_posts = searchPostsByWord(xml_content, post_seach);
        }

        for (auto& post : s_posts) {
            updated_xml += post;
            updated_xml += "\n";
        }
    }
    else if (operation == "most_active")
    {
        updated_xml = most_active(xml_content);
    }
    else if (operation == "most_influencer")
    {
        updated_xml = most_influencer(xml_content);
    }
    else if (operation == "mutual")
    {
        Ids = strIDs2int(strIDs);
        updated_xml = mutual(xml_content, Ids);
    }
    
    

    // Write output file (binary for compress, text for everything else)
    bool writeSuccess;
    if (operation == "compress") {
        writeSuccess = writeBinaryToFile(output_path, updated_xml);
    } 
    else if (operation == "draw") {
        drawXMLGraph(xml_content, output_path);
        writeSuccess = true;
    }
    else {
        writeSuccess = writeToFile(output_path, updated_xml);
    }
    
    if (writeSuccess) {
        cout << "File created successfully!\n";
    } else {
        cout << "Failed to create file.\n";
    }
    
    return 0;
}