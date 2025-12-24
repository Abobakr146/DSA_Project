#include "graph.h"
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <iostream>
#include <fstream>

using namespace std;

/*--------------------------------------------------
  Parse XML content and build graph
--------------------------------------------------*/
Graph buildGraphFromXML(const string& xmlContent)
{
    Graph graph;
    XMLDocument doc;

    if (doc.Parse(xmlContent.c_str()) != XML_SUCCESS)
    {
        cerr << "Error: Invalid XML\n";
        return graph;
    }

    XMLElement* users = doc.FirstChildElement("users");
    if (!users) return graph;

    for (XMLElement* user = users->FirstChildElement("user");
         user;
         user = user->NextSiblingElement("user"))
    {
        XMLElement* idElem = user->FirstChildElement("id");
        if (!idElem || !idElem->GetText()) continue;

        int userId = stoi(idElem->GetText());
        graph[userId]; // ensure node exists

        XMLElement* followers = user->FirstChildElement("followers");
        if (!followers) continue;

        for (XMLElement* follower = followers->FirstChildElement("follower");
             follower;
             follower = follower->NextSiblingElement("follower"))
        {
            XMLElement* fid = follower->FirstChildElement("id");
            if (!fid || !fid->GetText()) continue;

            int followerId = stoi(fid->GetText());
            graph[userId].push_back(followerId);
        }
    }

    return graph;
}

/*--------------------------------------------------
  Export graph to DOT file
--------------------------------------------------*/
void exportToDot(const Graph& graph, const string& dotFile)
{
    ofstream out(dotFile);
    out << "digraph SocialNetwork {\n";
    out << "  node [shape=circle, style=filled, fillcolor=lightblue];\n";

    for (const auto& [user, followers] : graph)
    {
        for (int f : followers)
        {
            out << "  " << user << " -> " << f << ";\n";
        }
    }

    out << "}\n";
    out.close();
}

/*--------------------------------------------------
  Render DOT using Graphviz - FIXED VERSION
--------------------------------------------------*/
void renderGraph(const string& dotFile, const string& outputImage)
{
    // Try multiple possible locations for dot.exe
    QStringList possiblePaths = {
        "dot",  // If in PATH
        "C:\\Program Files\\Graphviz\\bin\\dot.exe",
        "C:\\Program Files (x86)\\Graphviz\\bin\\dot.exe",
        "C:\\Program Files\\Graphviz 2.44\\bin\\dot.exe",
        QCoreApplication::applicationDirPath() + "\\Graphviz\\bin\\dot.exe"
    };
    
    QString dotPath;
    QProcess testProcess;
    
    // Test each possible path
    for (const QString& path : possiblePaths) {
        testProcess.start(path, QStringList() << "-V");
        if (testProcess.waitForFinished(1000)) {
            if (testProcess.exitCode() == 0) {
                dotPath = path;
                qDebug() << "Found GraphViz at:" << dotPath;
                break;
            }
        }
        testProcess.close();
    }
    
    if (dotPath.isEmpty()) {
        qDebug() << "Error: GraphViz not found!";
        qDebug() << "Please install GraphViz from: https://graphviz.org/download/";
        qDebug() << "Or specify the full path to dot.exe";
        return;
    }
    
    // Now use the found path
    QProcess process;
    QStringList arguments;
    arguments << "-Tjpg" << QString::fromStdString(dotFile) 
              << "-o" << QString::fromStdString(outputImage);
    
    process.start(dotPath, arguments);
    
    if (!process.waitForStarted(3000)) {
        qDebug() << "Error: Could not start GraphViz process";
        return;
    }
    
    if (!process.waitForFinished(10000)) { // 10 second timeout
        qDebug() << "Error: GraphViz timeout";
        process.kill();
        return;
    }
    
    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        qDebug() << "GraphViz error:" << errorOutput;
        qDebug() << "Command:" << dotPath << arguments.join(" ");
    } else {
        qDebug() << "Graph successfully created:" << QString::fromStdString(outputImage);
    }
}

/*--------------------------------------------------
  Public API (matches assignment)
--------------------------------------------------*/
void drawXMLGraph(const string& xmlContent, const string& outputImage)
{
    const string dotFile = "temp_graph.dot";

    Graph graph = buildGraphFromXML(xmlContent);
    exportToDot(graph, dotFile);
    renderGraph(dotFile, outputImage);
}