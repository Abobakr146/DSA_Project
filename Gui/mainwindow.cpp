#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "functions.h"
#include "utils.h"
#include "graph.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QPixmap>
#include <QRegularExpression>
#include <QDateTime>
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
    // Note: draw is NOT in function_map - handled separately
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

// Helper function to execute a specific function
void MainWindow::executeFunction(const string& funcName) {
    QString inputText = ui->textEdit->toPlainText();
    
    lastOutputWasBinary = false;
    lastBinaryOutput.clear();
    
    // For compression/decompression functions, allow file selection if input is empty
    if (inputText.isEmpty() && (funcName == "compress" || funcName == "decompress")) {
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
        
        string fileContent;
        if (!extract_content(inputFilePath.toStdString(), fileContent)) {
            QMessageBox::critical(this, "Error", "Failed to read file!");
            return;
        }
        
        inputText = QString::fromStdString(fileContent);
        ui->textEdit->setText(inputText);
    }
    // For other functions (including map it), just require input in the text field
    else if (inputText.isEmpty()) {
        QMessageBox::information(this, "No Input", "Please provide input in the text field.");
        return;
    }
    
    try {
        string inputStdString;
        
        if (funcName == "decompress") {
            string hexData = inputText.toStdString();
            inputStdString = hexStringToBinary(hexData);
            
            if (inputStdString.empty()) {
                ui->textEdit_2->setText("Error: Invalid hex input for decompression!");
                return;
            }
        } else {
            inputStdString = inputText.toStdString();
        }
        
        string result = function_map[funcName](inputStdString);
        
        if (result.empty()) {
            ui->textEdit_2->setText("This function is not yet implemented!");
            return;
        }
        
        if (funcName == "compress") {
            lastOutputWasBinary = true;
            lastBinaryOutput = result;
            ui->textEdit_2->setText(binaryToHexString(result));
        } else if (funcName == "decompress") {
            if (result.find("<") != string::npos) {
                ui->textEdit_2->setText(QString::fromStdString(result));
            } else {
                lastOutputWasBinary = true;
                lastBinaryOutput = result;
                ui->textEdit_2->setText(binaryToHexString(result));
            }
        } else {
            ui->textEdit_2->setText(QString::fromStdString(result));
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
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

// "map it" button handler - Generates DOT from input XML
void MainWindow::on_pushButton_8_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QMessageBox::information(this, "No Input", "Please provide XML input in the text field.");
        return;
    }
    
    try {
        string dotContent = generateDotContent(inputText.toStdString());
        
        if (dotContent.find("Error:") == 0) {
            ui->textEdit_2->setText(QString::fromStdString(dotContent));
        } else {
            // Show ONLY the DOT content
            ui->textEdit_2->setText(QString::fromStdString(dotContent));
        }
        
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// "to jpg" button handler - Converts DOT to JPG
void MainWindow::on_pushButton_15_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QMessageBox::information(this, "No Input", 
            "Please provide DOT content in the input field or generate it using 'map it'.");
        return;
    }
    
    try {
        QString dotContent = inputText;
        
        // Check if the input looks like DOT content
        if (!dotContent.contains("digraph") && !dotContent.contains("->")) {
            QMessageBox::warning(this, "Invalid Input", 
                "Input doesn't appear to be valid DOT content.\n"
                "Please generate DOT content using 'map it' first.");
            return;
        }
        
        // Save DOT content to a temporary file
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyyMMdd_hhmmss");
        QString tempDotFile = "temp_graph_" + timestamp + ".dot";
        QString tempJpgFile = "graph_" + timestamp + ".jpg";
        
        QFile dotFile(tempDotFile);
        if (dotFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&dotFile);
            out << dotContent;
            dotFile.close();
        } else {
            ui->textEdit_2->setText("Error: Could not create temporary DOT file.");
            return;
        }
        
        // Ask user where to save the JPG
        QString savePath = QFileDialog::getSaveFileName(this, 
            "Save Graph as JPG", 
            "graph.jpg", 
            "JPEG Images (*.jpg *.jpeg);;PNG Images (*.png);;All Files (*.*)");
        
        if (savePath.isEmpty()) {
            // Clean up temp file
            QFile::remove(tempDotFile);
            return;
        }
        
        // Convert DOT to JPG
        renderGraph(tempDotFile.toStdString(), savePath.toStdString());
        
        // Clean up temp file
        QFile::remove(tempDotFile);
        
        // Update current files
        currentDotFile = tempDotFile.toStdString();
        currentJpgFile = savePath.toStdString();
        
        QMessageBox::information(this, "Success", 
            "Graph saved as JPG:\n" + savePath);
            
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// "View Graph" button handler - Uses drawXMLGraph
void MainWindow::on_pushButton_14_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QMessageBox::information(this, "No Input", "Please provide XML input in the text field.");
        return;
    }
    
    try {
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyyMMdd_hhmmss");
        currentJpgFile = "graph_" + timestamp.toStdString() + ".jpg";
        
        string xmlContent = inputText.toStdString();
        
        // Use the drawXMLGraph function from graph.h
        drawXMLGraph(xmlContent, currentJpgFile);
        
        // Show success message
        QString result = "✓ Graph generated successfully!\n";
        result += "Image saved as: " + QString::fromStdString(currentJpgFile) + "\n";
        result += "You can find the image in the application directory.";
        
        ui->textEdit_2->setText(result);
        
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    }
}

// === XML Operations ===
void MainWindow::on_pushButton_clicked() { 
    executeFunction("verify"); 
}

void MainWindow::on_pushButton_2_clicked() { 
    executeFunction("json"); 
}

void MainWindow::on_pushButton_3_clicked() { 
    executeFunction("compress"); 
}

void MainWindow::on_pushButton_4_clicked() { 
    executeFunction("decompress"); 
}

void MainWindow::on_pushButton_5_clicked() { 
    executeFunction("mini"); 
}

void MainWindow::on_pushButton_6_clicked() { 
    executeFunction("format"); 
}

void MainWindow::on_pushButton_fixation_clicked() { 
    executeFunction("fixation"); 
}

// === Network Analysis ===
void MainWindow::on_pushButton_11_clicked() { 
    executeFunction("most_active"); 
}

void MainWindow::on_pushButton_9_clicked() { 
    executeFunction("most_influencer"); 
}

// Mutual Users
void MainWindow::on_pushButton_mutual_clicked() {
    QString inputText = ui->textEdit->toPlainText();
    
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
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
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
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
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
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
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
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
    
    if (outputText.isEmpty()) {
        QMessageBox::information(this, "No Output", "There is no output to save.");
        return;
    }
    
    QString filter;
    
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
        filter = "Hex Files (*.hex);;Text Files (*.txt);;All Files (*.*)";
    } else {
        filter = "Text Files (*.txt);;All Files (*.*)";
    }
    
    QString outputFilePath = QFileDialog::getSaveFileName(this, 
        "Save Output File", "", filter);

    if (outputFilePath.isEmpty()) {
        return;
    }

    if (lastOutputWasBinary && !lastBinaryOutput.empty()) {
        QFile outputFile(outputFilePath);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << outputText;
            outputFile.close();
            QMessageBox::information(this, "Success", "File saved to:\n" + outputFilePath);
        } else {
            QMessageBox::critical(this, "Error", "Unable to save file!");
        }
    } else {
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