#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>

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
    void on_pushButton_clicked();     // is_balanced (verify)
    void on_pushButton_2_clicked();   // To json (json)
    void on_pushButton_3_clicked();   // compression (compress)
    void on_pushButton_4_clicked();   // decompression (decompress)
    void on_pushButton_5_clicked();   // minifying (mini)
    void on_pushButton_6_clicked();   // prettify (format)
    
    // Network Analysis
    void on_pushButton_11_clicked();  // most active
    void on_pushButton_9_clicked();   // most influencer
    void on_pushButton_10_clicked();  // mutual users
    void on_pushButton_12_clicked();  // suggested users
    void on_pushButton_13_clicked();  // search
    
    // Visualization
    void on_pushButton_8_clicked();   // map to DOT (draw)
    void on_pushButton_15_clicked();  // to jpg (uses draw)
    void on_pushButton_14_clicked();  // close image
    
    // Save Output
    void on_pushButton_7_clicked();   // save

private:
    Ui::MainWindow *ui;
    
    // Helper functions
    QString saveTextToTempFile(const QString& text);
    void executeFunction(const std::string& funcName);
    std::string readFile(const std::string& filePath); // For compatibility
};

#endif // MAINWINDOW_H