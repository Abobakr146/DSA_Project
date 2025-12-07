#include "functions.h"


int main(int argc, char* argv[]) {
    string input_path;
    string output_path;
    string xml_content;
    string operation;
    bool fix;
    
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
        else if (arg == "-f" && i + 1 < argc) {
            fix = true;
        }
        else if (i == 1) {
            operation = arg;
        }
    }

    // --- 2. Validation and Execution ---
    
    if (input_path.empty()) {
        cerr << "Usage: " << argv[0] << " <order> -i <path/to/input.xml> [-o <path/to/output.xml>]" << endl;
        cerr << "Error: Input file path (-i) is missing." << endl;
        return 1;
    }
    
    // Inform the user what operation is being simulated
    cout << "Simulating XML Editor with operation: '" << operation << "'" << endl;
    if (operation == "verify") {
        if (fix)
            cout << "With Fixation" << endl;
        else 
            cout << "Without Fixation" << endl;
    }
    
    cout << "Attempting to read XML file: " << input_path << endl;

    // --- 3. Call the extraction function ---
    if (extract_content(input_path, xml_content)) {
        cout << "\n--- Successfully extracted XML content into 'xml_content' string ---\n" << endl;
        
    } else {
        // Error
        return 1;
    }

    string updated_xml;
    if(operation == "verify") {
        updated_xml = verify(xml_content);
        if (fix)
            updated_xml = fixation(updated_xml);
    }
    if(operation == "format") {
        updated_xml = format(xml_content);
    }
    if(operation == "json") {
        updated_xml = json(xml_content);
    }
    if(operation == "mini") {
        updated_xml = mini(xml_content);
    }
    if(operation == "compress") {
        updated_xml = compress(xml_content);
    }
    if(operation == "decompress") {
        updated_xml = decompress(xml_content);
    }

    if (writeToFile(output_path, xml_content)) {
        cout << "File created successfully!\n";
    } else {
        cout << "Failed to create file.\n";
    }
    
    return 0;
}