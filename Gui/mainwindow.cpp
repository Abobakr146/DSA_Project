#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "functions.h"
#include "utils.h"
#include "graph.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QPixmap>
#include <QResizeEvent>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <map>
#include <functional>
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <fstream>

using namespace std;

// Map to hold function pointers from functions.h (except draw)
map<string, function<string(const string &)>> function_map;
// Track if last output was binary (for compression/decompression)
bool lastOutputWasBinary = false;
string lastBinaryOutput;
// Current graph files
string currentDotFile;
string currentJpgFile;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Set placeholders for search bars
    ui->lineEdit_mutual->setPlaceholderText("Enter comma-separated user IDs (e.g., 1,2,3)");
    ui->lineEdit_suggest->setPlaceholderText("Enter a single user ID (e.g., 1)");
    ui->lineEdit_postSearch->setPlaceholderText("Enter word or topic to search in posts");
    
    // Style the graph label with better appearance
    ui->graphImageLabel->setStyleSheet(
        "QLabel { "
        "background-color: #fafafa; "
        "border: 2px dashed #ccc; "
        "border-radius: 4px; "
        "color: #666; "
        "font-size: 12px; "
        "}"
    );
    
    // Initialize function map with all functions from functions.h EXCEPT draw
    function_map["verify"] = verify;
    function_map["format"] = format;
    function_map["json"] = json;
    function_map["mini"] = mini;
    function_map["compress"] = compress;
    function_map["decompress"] = decompress;
    function_map["fixation"] = fixation;
    function_map["most_active"] = most_active;
    function_map["most_influencer"] = most_influencer;
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Display binary data as clean hex string
QString MainWindow::binaryToHexString(const string& binaryData) {
    QString hexString;
    
    for (size_t i = 0; i < binaryData.size(); i++) {
        hexString += QString("%1").arg((unsigned char)binaryData[i], 2, 16, QChar('0')).toUpper();
        if (i < binaryData.size() - 1) {
            hexString += " ";
        }
    }
    
    return hexString;
}

// Convert hex string back to binary
string MainWindow::hexStringToBinary(const string& hexStr) {
    string binary;
    string hexChars;
    
    for (char c : hexStr) {
        if (c != ' ') {
            hexChars += c;
        }
    }
    
    if (hexChars.length() % 2 != 0) {
        return "";
    }
    
    for (size_t i = 0; i < hexChars.length(); i += 2) {
        string byteStr = hexChars.substr(i, 2);
        char byte = (char)strtol(byteStr.c_str(), nullptr, 16);
        binary += byte;
    }
    
    return binary;
}

// Display image in GUI with better quality
void MainWindow::displayImageInGUI(const QString& imagePath) {
    QPixmap pixmap(imagePath);
    
    if (pixmap.isNull()) {
        ui->graphImageLabel->setText("Error: Could not load image\n" + imagePath);
        ui->graphImageLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px solid #ccc; }");
        return;
    }
    
    // Get the label size with some padding
    QSize labelSize = ui->graphImageLabel->size();
    int padding = 10;
    QSize targetSize(labelSize.width() - padding * 2, labelSize.height() - padding * 2);
    
    // Scale the image to fit the label while maintaining aspect ratio
    // Use SmoothTransformation for better quality
    QPixmap scaledPixmap = pixmap.scaled(targetSize, 
                                         Qt::KeepAspectRatio, 
                                         Qt::SmoothTransformation);
    
    ui->graphImageLabel->setPixmap(scaledPixmap);
    ui->graphImageLabel->setAlignment(Qt::AlignCenter);
    ui->graphImageLabel->setStyleSheet(
        "QLabel { "
        "background-color: white; "
        "border: 2px solid #2196F3; "
        "border-radius: 4px; "
        "padding: 5px; "
        "}"
    );
    ui->graphImageLabel->setText("");  // Clear any text
}

// Handle window resize to rescale image
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    
    // If we have a current JPG file, refresh the image display
    if (!currentJpgFile.empty()) {
        QString jpgPath = QString::fromStdString(currentJpgFile);
        if (QFile::exists(jpgPath)) {
            displayImageInGUI(jpgPath);
        }
    }
}

// Generate and show ONLY DOT content (no extra text) - for "map it" button
string MainWindow::generateDotContent(const string& xmlContent) {
    try {
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyyMMdd_hhmmss");
        currentDotFile = "graph_" + timestamp.toStdString() + ".dot";
        currentJpgFile = "graph_" + timestamp.toStdString() + ".jpg";
        
        Graph graph = buildGraphFromXML(xmlContent);
        
        if (graph.empty()) {
            return "Error: No user/follower data found in XML.";
        }
        
        exportToDot(graph, currentDotFile);
        
        ifstream dotIn(currentDotFile);
        if (!dotIn.is_open()) {
            return "Error: Could not create DOT file.";
        }
        
        stringstream dotContent;
        dotContent << dotIn.rdbuf();
        dotIn.close();
        
        // Return ONLY the DOT content, nothing else
        return dotContent.str();
        
    } catch (const exception& e) {
        return string("Error: ") + e.what();
    }
}

// "map it" button handler - Generates DOT from input XML and allows saving as .dot file
void MainWindow::on_pushButton_8_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    // If input is empty, allow user to select XML file
    if (inputText.isEmpty()) {
        QString filePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                        "XML Files (*.xml);;Text Files (*.txt);;All Files (*.*)");
        if (filePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input.");
            return;
        }
        
        string fileContent;
        if (!extract_content(filePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    try {
        string dotContent = generateDotContent(inputText.toStdString());
        
        if (dotContent.find("Error:") == 0) {
            ui->textEdit_2->setText(QString::fromStdString(dotContent));
        } else {
            // Show the DOT content
            ui->textEdit_2->setText(QString::fromStdString(dotContent));
            
            // Store the DOT content for potential saving
            lastOutputWasBinary = false;
            lastBinaryOutput.clear();
            
            // Show success message with saving instructions
            QString result = "✓ DOT graph generated successfully!\n\n";
            result += "DOT Content:\n";
            result += "----------------------------------------\n";
            result += QString::fromStdString(dotContent).left(500); // Show first 500 chars
            if (dotContent.length() > 500) {
                result += "\n... [truncated - full content in output area]\n";
            }
            result += "\n----------------------------------------\n\n";
            result += "You can now:\n";
            result += "1. Copy the DOT content from the output area\n";
            result += "2. Click 'Save Output' to save as .dot file\n";
            result += "3. Click 'to jpg' to convert to image\n";
            result += "4. Click 'View Graph' to generate and view graph directly\n";
            
            ui->textEdit_2->setText(result);
        }
        
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// "to jpg" button handler - Converts DOT to JPG
void MainWindow::on_pushButton_15_clicked() {
    QString dotFilePath;
    
    // Always prompt user to select a DOT file
    dotFilePath = QFileDialog::getOpenFileName(this, 
        "Select DOT File to Convert", 
        "", 
        "DOT Files (*.dot);;All Files (*.*)");
    
    if (dotFilePath.isEmpty()) {
        QMessageBox::information(this, "No File Selected", 
            "Please select a DOT file to convert to JPG.");
        return;
    }
    
    // Verify the file exists
    if (!QFile::exists(dotFilePath)) {
        QMessageBox::critical(this, "Error", "The selected file does not exist!");
        return;
    }
    
    // Read and validate DOT file content
    QFile dotFile(dotFilePath);
    if (!dotFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to read DOT file!");
        return;
    }
    
    QString dotContent = dotFile.readAll();
    dotFile.close();
    
    // Validate DOT content
    if (!dotContent.contains("digraph") && !dotContent.contains("graph")) {
        QMessageBox::warning(this, "Invalid DOT File", 
            "The selected file doesn't appear to be a valid DOT file.\n"
            "DOT files should contain 'digraph' or 'graph' declarations.");
        return;
    }
    
    // Display DOT content in input area for reference
    ui->textEdit->setText(dotContent);
    
    try {
        // Ask user where to save the JPG
        QString defaultName = QFileInfo(dotFilePath).baseName() + ".jpg";
        QString savePath = QFileDialog::getSaveFileName(this, 
            "Save Graph as JPG", 
            defaultName, 
            "JPEG Images (*.jpg *.jpeg);;PNG Images (*.png);;All Files (*.*)");
        
        if (savePath.isEmpty()) {
            return;
        }
        
        // Convert DOT to JPG using the existing DOT file
        renderGraph(dotFilePath.toStdString(), savePath.toStdString());
        
        // Update current files
        currentDotFile = dotFilePath.toStdString();
        currentJpgFile = savePath.toStdString();
        
        // Display the image in GUI
        displayImageInGUI(savePath);
        
        // Show success message
        QString result = "✓ DOT file converted to JPG successfully!\n\n";
        result += "Source DOT: " + QFileInfo(dotFilePath).fileName() + "\n";
        result += "Output JPG: " + savePath + "\n\n";
        result += "Graph displayed above ↑\n\n";
        result += "The DOT content has been loaded into the input area.";
        
        ui->textEdit_2->setText(result);
        
        QMessageBox::information(this, "Success", 
            "DOT file successfully converted to JPG!\n\nSaved to: " + savePath);
        
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error during conversion: " + QString(e.what()));
        QMessageBox::critical(this, "Conversion Error", 
            "Failed to convert DOT to JPG:\n" + QString(e.what()));
    }
}

// "View Graph" button handler - Uses the 3 separate graph functions
void MainWindow::on_pushButton_14_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    // If input is empty, allow user to select XML file
    if (inputText.isEmpty()) {
        QString filePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                        "XML Files (*.xml);;Text Files (*.txt);;All Files (*.*)");
        if (filePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input.");
            return;
        }
        
        string fileContent;
        if (!extract_content(filePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    try {
        string xmlContent = inputText.toStdString();
        
        // Generate unique filenames with timestamp
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyyMMdd_hhmmss");
        QString dotFileName = "graph_" + timestamp + ".dot";
        QString jpgFileName = "graph_" + timestamp + ".jpg";
        
        // Use the 3 separate functions from graph.h
        // 1. Build graph from XML
        Graph graph = buildGraphFromXML(xmlContent);
        
        if (graph.empty()) {
            ui->textEdit_2->setText("Error: No user/follower data found in XML.");
            return;
        }
        
        // 2. Export to DOT file
        exportToDot(graph, dotFileName.toStdString());
        
        // 3. Render DOT to JPG
        renderGraph(dotFileName.toStdString(), jpgFileName.toStdString());
        
        // Update current files
        currentDotFile = dotFileName.toStdString();
        currentJpgFile = jpgFileName.toStdString();
        
        // Display the image in GUI
        displayImageInGUI(jpgFileName);
        
        // Show success message with file locations
        QString result = "✓ Graph generated successfully!\n\n";
        result += "Generated files:\n";
        result += "• DOT file: " + dotFileName + "\n";
        result += "• JPG file: " + jpgFileName + "\n\n";
        result += "Graph displayed above ↑\n";
        result += "Files are saved in the application directory.";
        
        ui->textEdit_2->setText(result);
        
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// === XML Operations ===
void MainWindow::on_pushButton_clicked() { 
    executeFunction("verify", true); 
}

void MainWindow::on_pushButton_2_clicked() { 
    executeFunction("json", true); 
}

void MainWindow::on_pushButton_3_clicked() { 
    executeFunction("compress", true); 
}

void MainWindow::on_pushButton_4_clicked() { 
    executeFunction("decompress", true); 
}

void MainWindow::on_pushButton_5_clicked() { 
    executeFunction("mini", true); 
}

void MainWindow::on_pushButton_6_clicked() { 
    executeFunction("format", true); 
}

void MainWindow::on_pushButton_fixation_clicked() { 
    executeFunction("fixation", true); 
}

// === Network Analysis ===
void MainWindow::on_pushButton_11_clicked() { 
    executeFunction("most_active", true); 
}

void MainWindow::on_pushButton_9_clicked() { 
    executeFunction("most_influencer", true); 
}

// Mutual Users
void MainWindow::on_pushButton_mutual_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                            "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input or select a file.");
            return;
        }
        
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    QString idsInput = ui->lineEdit_mutual->text();
    if (idsInput.isEmpty()) {
        QMessageBox::warning(this, "Input Error", 
            "Please enter user IDs (comma-separated) in the search field.");
        return;
    }
    
    vector<int> ids = strIDs2int(idsInput.toStdString());
    if (ids.empty()) {
        ui->textEdit_2->setText("Error: No valid user IDs provided.");
        return;
    }
    
    try {
        string result = mutual(inputText.toStdString(), ids);
        ui->textEdit_2->setText(QString::fromStdString(result));
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// Suggested Users
void MainWindow::on_pushButton_suggest_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                            "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input or select a file.");
            return;
        }
        
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    QString userIdInput = ui->lineEdit_suggest->text();
    if (userIdInput.isEmpty()) {
        QMessageBox::warning(this, "Input Error", 
            "Please enter a single user ID in the search field.");
        return;
    }
    
    vector<int> ids = strIDs2int(userIdInput.toStdString());
    if (ids.empty()) {
        ui->textEdit_2->setText("Error: Please enter a valid user ID.");
        return;
    }
    
    if (ids.size() > 1) {
        ui->textEdit_2->setText("Error: Please enter only ONE user ID.");
        return;
    }
    
    int userId = ids[0];
    
    try {
        string result = suggest(inputText.toStdString(), userId);
        ui->textEdit_2->setText(QString::fromStdString(result));
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// Search posts by word
void MainWindow::on_pushButton_searchWord_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                            "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input or select a file.");
            return;
        }
        
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    QString word = ui->lineEdit_postSearch->text();
    if (word.isEmpty()) {
        QMessageBox::warning(this, "Input Error", 
            "Please enter a word to search in posts.");
        return;
    }
    
    try {
        vector<string> results = searchPostsByWord(inputText.toStdString(), word.toStdString());
        
        if (results.empty()) {
            ui->textEdit_2->setText("No posts found containing: \"" + word + "\"");
        } else {
            QString output = "Found " + QString::number(results.size()) + " post(s):\n\n";
            for (size_t i = 0; i < results.size(); i++) {
                output += "Post " + QString::number(i + 1) + ":\n";
                output += QString::fromStdString(results[i]) + "\n";
                output += "---\n";
            }
            ui->textEdit_2->setText(output);
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// Search posts by topic
void MainWindow::on_pushButton_searchTopic_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                            "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input or select a file.");
            return;
        }
        
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    QString topic = ui->lineEdit_postSearch->text();
    if (topic.isEmpty()) {
        QMessageBox::warning(this, "Input Error", 
            "Please enter a topic to search in posts.");
        return;
    }
    
    try {
        vector<string> results = searchPostsByTopic(inputText.toStdString(), topic.toStdString());
        
        if (results.empty()) {
            ui->textEdit_2->setText("No posts found with topic: \"" + topic + "\"");
        } else {
            QString output = "Found " + QString::number(results.size()) + " post(s):\n\n";
            for (size_t i = 0; i < results.size(); i++) {
                output += "Post " + QString::number(i + 1) + ":\n";
                output += QString::fromStdString(results[i]) + "\n";
                output += "---\n";
            }
            ui->textEdit_2->setText(output);
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// === Save Output ===
void MainWindow::on_pushButton_7_clicked() { 
    QString outputText = ui->textEdit_2->toPlainText();
    
    // Check if we have a current JPG file to save
    if (!currentJpgFile.empty() && QFile::exists(QString::fromStdString(currentJpgFile))) {
        QString defaultName = QFileInfo(QString::fromStdString(currentJpgFile)).fileName();
        QString savePath = QFileDialog::getSaveFileName(this, 
            "Save Graph Image", 
            defaultName,
            "JPEG Images (*.jpg *.jpeg);;PNG Images (*.png);;All Files (*.*)");
        
        if (savePath.isEmpty()) {
            return;
        }
        
        // Copy the JPG file to the new location
        if (QFile::copy(QString::fromStdString(currentJpgFile), savePath)) {
            QMessageBox::information(this, "Success", 
                "Graph image saved to:\n" + savePath);
        } else {
            QMessageBox::critical(this, "Error", "Failed to save image!");
        }
        return;
    }
    
    // Check if we have DOT content in the output
    bool hasDotContent = (outputText.contains("digraph") || outputText.contains("graph")) && 
                         (outputText.contains("->") || outputText.contains("--"));
    
    // Check if we have a current DOT file
    if (!currentDotFile.empty() && QFile::exists(QString::fromStdString(currentDotFile)) && hasDotContent) {
        QString defaultName = QFileInfo(QString::fromStdString(currentDotFile)).fileName();
        QString savePath = QFileDialog::getSaveFileName(this, 
            "Save DOT File", 
            defaultName,
            "DOT Files (*.dot);;Text Files (*.txt);;All Files (*.*)");
        
        if (savePath.isEmpty()) {
            return;
        }
        
        // Copy the DOT file to the new location
        if (QFile::copy(QString::fromStdString(currentDotFile), savePath)) {
            QMessageBox::information(this, "Success", 
                "DOT file saved to:\n" + savePath);
            return;
        }
    }
    
    // Otherwise save text output
    if (outputText.isEmpty()) {
        QMessageBox::information(this, "No Output", "There is no output to save.");
        return;
    }
    
    // Determine file type based on content
    QString filter;
    
    if (hasDotContent) {
        filter = "DOT Graph Files (*.dot);;Text Files (*.txt);;All Files (*.*)";
    } else if (outputText.contains("{\"") && outputText.contains("}")) {
        filter = "JSON Files (*.json);;Text Files (*.txt);;All Files (*.*)";
    } else if (outputText.contains("<") && outputText.contains(">")) {
        filter = "XML Files (*.xml);;Text Files (*.txt);;All Files (*.*)";
    } else {
        // DEFAULT: Just save as text file
        filter = "Text Files (*.txt);;All Files (*.*)";
    }
    
    QString outputFilePath = QFileDialog::getSaveFileName(this, 
        "Save Output File", 
        "output.txt", 
        filter);

    if (outputFilePath.isEmpty()) {
        return;
    }

    // Handle text output
    QFile outputFile(outputFilePath);
    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&outputFile);
        
        // For DOT files, extract just the graph content if it's mixed with other text
        if (hasDotContent && outputFilePath.endsWith(".dot", Qt::CaseInsensitive)) {
            QString dotContent = outputText;
            // Try to extract just the graph definition
            if (dotContent.contains("digraph") && dotContent.contains("}")) {
                int start = dotContent.indexOf("digraph");
                int end = dotContent.lastIndexOf("}") + 1;
                if (start != -1 && end != -1 && end > start) {
                    dotContent = dotContent.mid(start, end - start);
                }
            }
            out << dotContent;
        } else {
            out << outputText;
        }
        
        outputFile.close();
        QMessageBox::information(this, "Success", "Output saved to:\n" + outputFilePath);
    } else {
        QMessageBox::critical(this, "Error", "Unable to save output file!");
    }
}

// Utility function to read file
string MainWindow::readFile(const string& filePath) {
    ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Execute function from function_map
void MainWindow::executeFunction(const string& funcName, bool allowFileDialog) {
    QString inputText = ui->textEdit->toPlainText();
    
    // If input is empty and file dialog is allowed, prompt user to select a file
    if (inputText.isEmpty() && allowFileDialog) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, "Select XML File", "", 
                                                            "XML Files (*.xml);;Text Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input or select a file.");
            return;
        }
        
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    // Check if input is still empty
    if (inputText.isEmpty()) {
        QMessageBox::information(this, "No Input", "Please provide XML input.");
        return;
    }
    
    // Check if function exists in map
    if (function_map.find(funcName) == function_map.end()) {
        ui->textEdit_2->setText("Error: Function '" + QString::fromStdString(funcName) + "' not found.");
        return;
    }
    
    try {
        string input = inputText.toStdString();
        
        // Special handling for decompress - convert hex to binary
        if (funcName == "decompress") {
            // Check if input looks like hex
            QString trimmedInput = inputText.trimmed();
            if (QRegularExpression("^[0-9A-Fa-f\\s]+$").match(trimmedInput).hasMatch()) {
                input = hexStringToBinary(trimmedInput.toStdString());
                if (input.empty()) {
                    ui->textEdit_2->setText("Error: Invalid hex format for decompression.");
                    return;
                }
            }
        }
        
        // Execute the function
        string result = function_map[funcName](input);
        
        // Special handling for compress - display as hex
        if (funcName == "compress") {
            lastOutputWasBinary = true;
            lastBinaryOutput = result;
            QString hexOutput = binaryToHexString(result);
            ui->textEdit_2->setText(hexOutput);
        } else {
            lastOutputWasBinary = false;
            lastBinaryOutput.clear();
            ui->textEdit_2->setText(QString::fromStdString(result));
        }
        
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}