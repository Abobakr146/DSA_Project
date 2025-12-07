#include "functions.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    string input_path;
    string output_path;
    string xml_content;
    string operation;
    bool fix = false;
    
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
        else if (arg == "-f") {
            fix = true;
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

    // Write output file (binary for compress, text for everything else)
    bool writeSuccess;
    if (operation == "compress") {
        writeSuccess = writeBinaryToFile(output_path, updated_xml);
    } else {
        writeSuccess = writeToFile(output_path, updated_xml);
    }
    
    if (writeSuccess) {
        cout << "File created successfully!\n";
    } else {
        cout << "Failed to create file.\n";
    }
    
    return 0;
}