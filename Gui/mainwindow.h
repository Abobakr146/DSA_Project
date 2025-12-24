#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <vector>
#include <map>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // XML Operations
    void on_pushButton_clicked();          // is_balanced (verify)
    void on_pushButton_2_clicked();        // To json (json)
    void on_pushButton_3_clicked();        // compression (compress)
    void on_pushButton_4_clicked();        // decompression (decompress)
    void on_pushButton_5_clicked();        // minifying (mini)
    void on_pushButton_6_clicked();        // prettify (format)
    void on_pushButton_fixation_clicked(); // fixation
    
    // Network Analysis
    void on_pushButton_11_clicked();       // most active
    void on_pushButton_9_clicked();        // most influencer
    void on_pushButton_mutual_clicked();   // mutual users
    void on_pushButton_suggest_clicked();  // suggested users
    
    // Search Functions
    void on_pushButton_searchWord_clicked();  // search by word
    void on_pushButton_searchTopic_clicked(); // search by topic
    
    // Visualization
    void on_pushButton_8_clicked();        // map it (generates DOT)
    void on_pushButton_15_clicked();       // to jpg (converts DOT to JPG)
    void on_pushButton_14_clicked();       // View Graph (uses drawXMLGraph)
    
    // Save Output
    void on_pushButton_7_clicked();        // save

private:
    Ui::MainWindow *ui;
    
    // Function map for XML operations (EXCLUDES draw)
    std::map<std::string, std::function<std::string(const std::string&)>> function_map;
    
    // Track binary output state
    bool lastOutputWasBinary;
    std::string lastBinaryOutput;
    
    // Current graph files
    std::string currentDotFile;
    std::string currentJpgFile;
    
    // Helper functions
    void executeFunction(const std::string& funcName, bool allowFileDialog = true);
    
    // Binary/Hex conversion
    QString binaryToHexString(const std::string& binaryData);
    std::string hexStringToBinary(const std::string& hexStr);
    
    // Graph helper functions
    std::string generateDotContent(const std::string& xmlContent);
    
    // Image display function
    void displayImageInGUI(const QString& imagePath);
    
    // Utility functions
    std::string readFile(const std::string& filePath);

    protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H