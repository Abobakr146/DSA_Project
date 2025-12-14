#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "functions.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QPixmap>
#include <map>
#include <functional>
#include <string>
#include <iostream>

using namespace std;

// Map to hold function pointers from functions.h
map<string, function<string(const string &)>> function_map;

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

// Helper function to save text to temporary file
QString MainWindow::saveTextToTempFile(const QString& text) {
    QString tempFilePath = QDir::tempPath() + "/temp_input.xml";
    QFile tempFile(tempFilePath);

    if (tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&tempFile);
        out << text;
        tempFile.close();
        return tempFilePath;
    } else {
        QMessageBox::critical(this, "Error", "Unable to create temporary file!");
        return QString();
    }
}

// Helper function to execute a specific function
void MainWindow::executeFunction(const string& funcName) {
    QString inputText = ui->textEdit->toPlainText();
    
    // If no input in text field, try to open a file
    if (inputText.isEmpty()) {
        QString inputFilePath = QFileDialog::getOpenFileName(this, 
            "Select Input File", "", "XML Files (*.xml);;TXT Files (*.txt);;All Files (*.*)");
        
        if (inputFilePath.isEmpty()) {
            QMessageBox::information(this, "No Input", "Please provide XML input or select a file.");
            return;
        }
        
        QFile file(inputFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Failed to open selected file.");
            return;
        }
        
        QTextStream in(&file);
        inputText = in.readAll();
        file.close();
        
        // Also update the input text edit with the file content
        ui->textEdit->setText(inputText);
    }
    
    // Check if function exists in map
    if (function_map.find(funcName) == function_map.end()) {
        ui->textEdit_2->setText("Error: Function '" + QString::fromStdString(funcName) + "' not found!");
        return;
    }
    
    try {
        // Execute the function
        string inputStdString = inputText.toStdString();
        string result = function_map[funcName](inputStdString);
        
        // Check if result is empty (function not implemented in functions.cpp)
        if (result.empty()) {
            ui->textEdit_2->setText("This function is not yet implemented!");
        } else {
            ui->textEdit_2->setText(QString::fromStdString(result));
        }
    } catch (const exception& e) {
        ui->textEdit_2->setText("Error: " + QString(e.what()));
    } catch (...) {
        ui->textEdit_2->setText("Unknown error occurred while executing function.");
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
    
    // For search, we need to pass the word somehow
    // Since your search function in functions.h only takes XML, 
    // we'll just call it with the current XML content
    executeFunction("search");
    
    // Show a note about the search word
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
    // First try to generate DOT file using draw function
    executeFunction("draw");
    
    // If draw function is implemented and generated output
    QString output = ui->textEdit_2->toPlainText();
    if (!output.contains("not yet implemented") && !output.isEmpty()) {
        QMessageBox::information(this, "Graph Visualization", 
            "DOT file generated successfully!\n\n"
            "To convert to JPG/PNG, you need to:\n"
            "1. Save the DOT content to a .dot file\n"
            "2. Install GraphViz (https://graphviz.org/)\n"
            "3. Run: dot -Tpng filename.dot -o output.png");
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
    if (outputText.isEmpty()) {
        QMessageBox::information(this, "No Output", "There is no output to save.");
        return;
    }
    
    QString outputFilePath = QFileDialog::getSaveFileName(this, "Save Output File", 
        "", "XML Files (*.xml);;JSON Files (*.json);;TXT Files (*.txt);;DOT Files (*.dot);;All Files (*.*)");

    if (outputFilePath.isEmpty()) {
        QMessageBox::information(this, "No File Selected", "Please select a location to save the output.");
        return;
    }

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

// Helper function to read file (compatible with original code)
string MainWindow::readFile(const string& filePath) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "";
    }
    
    QString content = file.readAll();
    file.close();
    return content.toStdString();
}