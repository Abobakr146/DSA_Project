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
    function_map["mutual"] = mutual;
    function_map["suggest"] = suggest;
    function_map["search"] = static_cast<string(*)(const string&)>(search);
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
        
        if (result.empty()) {
            ui->textEdit_2->setText("This function is not yet implemented!");
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
void MainWindow::on_pushButton_clicked() // is_balanced (verify)
{
    executeFunction("verify");
}

void MainWindow::on_pushButton_2_clicked() // To json
{
    executeFunction("json");
}

void MainWindow::on_pushButton_3_clicked() // compression
{
    executeFunction("compress");
}

void MainWindow::on_pushButton_4_clicked() // decompression
{
    executeFunction("decompress");
}

void MainWindow::on_pushButton_5_clicked() // minifying
{
    executeFunction("mini");
}

void MainWindow::on_pushButton_6_clicked() // prettify (format)
{
    executeFunction("format");
}

// === Network Analysis ===
void MainWindow::on_pushButton_11_clicked() // most active
{
    executeFunction("most_active");
}

void MainWindow::on_pushButton_9_clicked() // most influencer
{
    executeFunction("most_influencer");
}

void MainWindow::on_pushButton_10_clicked() // mutual users
{
    executeFunction("mutual");
}

void MainWindow::on_pushButton_12_clicked() // suggested users
{
    executeFunction("suggest");
}

void MainWindow::on_pushButton_13_clicked() // search
{
    QString word = ui->lineEdit->text();
    if (word.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a word to search.");
        return;
    }
    
    executeFunction("search");
    
    if (!ui->textEdit_2->toPlainText().contains("not yet implemented")) {
        ui->textEdit_2->append("\n\nNote: Searched for word: " + word);
    }
}

// === Visualization ===
void MainWindow::on_pushButton_8_clicked() // map to DOT (draw)
{
    executeFunction("draw");
}

void MainWindow::on_pushButton_15_clicked() // to jpg
{
    executeFunction("draw");
    
    QString output = ui->textEdit_2->toPlainText();
    if (!output.contains("not yet implemented") && !output.isEmpty()) {
        QMessageBox::information(this, "Graph Visualization", 
            "DOT file generated!\nUse 'Save' to save as .dot file.");
    }
}

void MainWindow::on_pushButton_14_clicked() // close image
{
    ui->imageLabel->setVisible(false);
}

// === Save Output ===
void MainWindow::on_pushButton_7_clicked() // save
{
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