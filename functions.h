#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "functions.h"
#include "utils.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QPixmap>
#include <QRegularExpression>
#include <map>
#include <functional>
#include <string>
#include <iostream>
#include <cstdlib>  // For strtol
#include <sstream>
#include <vector>

using namespace std;

// Map to hold function pointers from functions.h
map<string, function<string(const string &)>> function_map;
// Track if last output was binary (for compression/decompression)
bool lastOutputWasBinary = false;
string lastBinaryOutput;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->imageLabel->setVisible(false);
    
    // Initialize function map with all functions from functions.h
    function_map["verify"] = verify;
    function_map["format"] = format;
    function_map["json"] = json;
    function_map["mini"] = mini;
    function_map["compress"] = compress;
    function_map["decompress"] = decompress;
    function_map["draw"] = draw;
    function_map["fixation"] = fixation;
    function_map["most_active"] = most_active;
    function_map["most_influencer"] = most_influencer;
    // Note: mutual, suggest, and search functions have parameters so they're handled separately
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Display binary data as clean hex string
QString binaryToHexString(const string& binaryData) {
    QString hexString;
    
    for (size_t i = 0; i < binaryData.size(); i++) {
        // Format as 2-digit hex with leading zero, uppercase
        hexString += QString("%1").arg((unsigned char)binaryData[i], 2, 16, QChar('0')).toUpper();
        
        // Add space between bytes for readability
        if (i < binaryData.size() - 1) {
            hexString += " ";
        }
    }
    
    return hexString;
}

// Convert hex string back to binary
string hexStringToBinary(const string& hexStr) {
    string binary;
    string hexChars;
    
    // Remove spaces from hex string
    for (char c : hexStr) {
        if (c != ' ') {
            hexChars += c;
        }
    }
    
    // Check if length is even (2 chars per byte)
    if (hexChars.length() % 2 != 0) {
        return "";
    }
    
    // Convert hex pairs to bytes
    for (size_t i = 0; i < hexChars.length(); i += 2) {
        string byteStr = hexChars.substr(i, 2);
        char byte = (char)strtol(byteStr.c_str(), nullptr, 16);
        binary += byte;
    }
    
    return binary;
}

// Helper function to execute a specific function
void MainWindow::executeFunction(const string& funcName) {
    QString inputText = ui->textEdit->toPlainText();
    
    // Reset binary flag
    lastOutputWasBinary = false;
    lastBinaryOutput.clear();
    
    // If no input in text field, try to open a file
    if (inputText.isEmpty()) {
        QString filter;
        if (funcName == "decompress") {
            filter = "Hex Files (*.hex);;Text Files (*.txt);;All Files (*.*)";
        } else {
            filter = "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)";
        }
        
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", filter);
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide input or select a file.");
            return;
        }
        
        // Read file using utils.h function
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    // Now process the input text
    try {
        string inputStdString;
        
        if (funcName == "decompress") {
            // For decompression, convert hex to binary
            string hexData = inputText.toStdString();
            inputStdString = hexStringToBinary(hexData);
            
            if (inputStdString.empty()) {
                ui->textEdit_2->setText("Error: Invalid hex input for decompression!");
                return;
            }
        } else {
            // For other functions, use text as-is
            inputStdString = inputText.toStdString();
        }
        
        // Execute the function
        string result = function_map[funcName](inputStdString);
        
        // Check if result is empty (function not implemented in functions.cpp)
        if (result.empty()) {
            if (funcName == "draw") {
                // Special message for draw function (map to DOT)
                ui->textEdit_2->setText("Graph visualization (DOT) function not implemented yet!\n\n"
                                        "To implement, you need to:\n"
                                        "1. Parse the XML to extract user-follower relationships\n"
                                        "2. Generate DOT graph format like:\n"
                                        "   digraph G {\n"
                                        "      \"User1\" -> \"User2\"\n"
                                        "      \"User2\" -> \"User3\"\n"
                                        "   }");
            } else {
                ui->textEdit_2->setText("This function is not yet implemented!");
            }
            return;
        }
        
        // Handle output
        if (funcName == "compress") {
            // Compression: show hex
            lastOutputWasBinary = true;
            lastBinaryOutput = result;
            ui->textEdit_2->setText(binaryToHexString(result));
        } else if (funcName == "decompress") {
            // Decompression: check if result is XML
            if (result.find("<") != string::npos) {
                // Looks like XML - show as text
                ui->textEdit_2->setText(QString::fromStdString(result));
            } else {
                // Still binary - show as hex
                lastOutputWasBinary = true;
                lastBinaryOutput = result;
                ui->textEdit_2->setText(binaryToHexString(result));
            }
        } else {
            // Other functions: show as text
            ui->textEdit_2->setText(QString::fromStdString(result));
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// === XML Operations ===
void MainWindow::on_pushButton_clicked() { executeFunction("verify"); }
void MainWindow::on_pushButton_2_clicked() { executeFunction("json"); }
void MainWindow::on_pushButton_3_clicked() { executeFunction("compress"); }
void MainWindow::on_pushButton_4_clicked() { executeFunction("decompress"); }
void MainWindow::on_pushButton_5_clicked() { executeFunction("mini"); }
void MainWindow::on_pushButton_6_clicked() { executeFunction("format"); }
void MainWindow::on_pushButton_fixation_clicked() { executeFunction("fixation"); }

// === Network Analysis ===
void MainWindow::on_pushButton_11_clicked() { executeFunction("most_active"); }
void MainWindow::on_pushButton_9_clicked() { executeFunction("most_influencer"); }

// Mutual Users with parameter - USING strIDs2int from functions.h
void MainWindow::on_pushButton_mutual_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide input or select a file.");
            return;
        }
        
        // Read file
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    // Get user IDs from lineEdit (comma-separated)
    QString idsInput = ui->lineEdit->text();
    if (idsInput.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter user IDs (comma-separated) in the search field.");
        return;
    }
    
    // USE strIDs2int function from functions.h
    vector<int> ids = strIDs2int(idsInput.toStdString());
    if (ids.empty()) {
        ui->textEdit_2->setText("Error: No valid user IDs provided. Please enter comma-separated integers.");
        return;
    }
    
    try {
        string result = mutual(inputText.toStdString(), ids);
        ui->textEdit_2->setText(QString::fromStdString(result));
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// Suggested Users with parameter - USING strIDs2int from functions.h
void MainWindow::on_pushButton_suggest_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide input or select a file.");
            return;
        }
        
        // Read file
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    // Get user ID from lineEdit
    QString userIdInput = ui->lineEdit->text();
    if (userIdInput.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a user ID in the search field.");
        return;
    }
    
    // USE strIDs2int for consistency (even for single ID)
    vector<int> ids = strIDs2int(userIdInput.toStdString());
    if (ids.empty()) {
        ui->textEdit_2->setText("Error: Please enter a valid user ID integer.");
        return;
    }
    
    if (ids.size() > 1) {
        ui->textEdit_2->setText("Error: Please enter only one user ID for suggestions.");
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

// Search posts by word - USING searchPostsByWord from functions.h
void MainWindow::on_pushButton_searchWord_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide input or select a file.");
            return;
        }
        
        // Read file
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    QString word = ui->lineEdit->text();
    if (word.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a word to search.");
        return;
    }
    
    try {
        // Use searchPostsByWord from functions.h
        vector<string> results = searchPostsByWord(inputText.toStdString(), word.toStdString());
        
        if (results.empty()) {
            ui->textEdit_2->setText("No posts found containing the word: \"" + word + "\"");
        } else {
            QString output = "Found " + QString::number(results.size()) + " post(s) containing \"" + word + "\":\n\n";
            for (size_t i = 0; i < results.size(); i++) {
                output += "Post " + QString::number(i + 1) + ":\n";
                output += QString::fromStdString(results[i]) + "\n";
                output += "---\n";
            }
            ui->textEdit_2->setText(output);
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error in search: " + QString(e.what()));
    }
}

// Search posts by topic - USING searchPostsByTopic from functions.h
void MainWindow::on_pushButton_searchTopic_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide input or select a file.");
            return;
        }
        
        // Read file
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    
    QString topic = ui->lineEdit->text();
    if (topic.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a topic to search.");
        return;
    }
    
    try {
        // Use searchPostsByTopic from functions.h
        vector<string> results = searchPostsByTopic(inputText.toStdString(), topic.toStdString());
        
        if (results.empty()) {
            ui->textEdit_2->setText("No posts found with topic: \"" + topic + "\"");
        } else {
            QString output = "Found " + QString::number(results.size()) + " post(s) with topic \"" + topic + "\":\n\n";
            for (size_t i = 0; i < results.size(); i++) {
                output += "Post " + QString::number(i + 1) + ":\n";
                output += QString::fromStdString(results[i]) + "\n";
                output += "---\n";
            }
            ui->textEdit_2->setText(output);
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error in search: " + QString(e.what()));
    }
}

// === Visualization ===
void MainWindow::on_pushButton_8_clicked() { 
    // Map to DOT (draw function)
    executeFunction("draw");
}

void MainWindow::on_pushButton_15_clicked() { 
    // Convert to JPG (uses draw function)
    executeFunction("draw");
    
    QString output = ui->textEdit_2->toPlainText();
    if (!output.contains("not yet implemented") && !output.isEmpty()) {
        QMessageBox::information(this, "Graph Visualization", 
            "DOT file generated!\nTo convert to JPG/PNG:\n"
            "1. Save the output as a .dot file\n"
            "2. Install GraphViz from https://graphviz.org/\n"
            "3. Run: dot -Tjpg filename.dot -o output.jpg\n"
            "4. Or use: dot -Tpng filename.dot -o output.png");
    }
}

void MainWindow::on_pushButton_14_clicked() { 
    ui->imageLabel->setVisible(false);
}

// === Save Output ===
void MainWindow::on_pushButton_7_clicked() { 
    QString outputText = ui->textEdit_2->toPlainText();
    
    if (outputText.isEmpty() || outputText.contains("not yet implemented")) {
        QMessageBox::information(this, "No Output", "There is no valid output to save.");
        return;
    }
    
    QString filter;
    
    // Determine file type based on content
    if (outputText.contains("{\"") && outputText.contains("}")) {
        filter = "JSON Files (*.json);;Text Files (*.txt);;All Files (*.*)";
    } else if (outputText.contains("digraph") || outputText.contains("->")) {
        filter = "DOT Files (*.dot);;Text Files (*.txt);;All Files (*.*)";
    } else if (outputText.contains("XML is valid") || outputText.contains("Error at line")) {
        filter = "Text Files (*.txt);;Log Files (*.log);;All Files (*.*)";
    } else if (outputText.contains("<") && outputText.contains(">")) {
        filter = "XML Files (*.xml);;Text Files (*.txt);;All Files (*.*)";
    } else if (lastOutputWasBinary || 
               (outputText.length() > 0 && 
                QRegularExpression("^[0-9A-F]{2}( [0-9A-F]{2})*$").match(outputText).hasMatch())) {
        // Looks like hex data (compression/decompression output)
        filter = "Hex Files (*.hex);;Text Files (*.txt);;All Files (*.*)";
    } else {
        filter = "Text Files (*.txt);;All Files (*.*)";
    }
    
    QString outputFilePath = QFileDialog::getSaveFileName(this, 
        "Save Output File", "", filter);

    if (outputFilePath.isEmpty()) {
        QMessageBox::information(this, "No File Selected", "Please select a location to save the output.");
        return;
    }

    if (lastOutputWasBinary && !lastBinaryOutput.empty()) {
        // Save as .hex file (text file with hex content)
        QFile outputFile(outputFilePath);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << outputText;  // Save hex text
            outputFile.close();
            QMessageBox::information(this, "Success", "Hex data saved to:\n" + outputFilePath);
        } else {
            QMessageBox::critical(this, "Error", "Unable to save file!");
        }
    } else {
        // Save text data
        QFile outputFile(outputFilePath);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << outputText;
            outputFile.close();
            QMessageBox::information(this, "Success", "Output saved to:\n" + outputFilePath);
        } else {
            QMessageBox::critical(this, "Error", "Unable to save output file!");
        }
    }
}