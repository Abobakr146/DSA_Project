#ifndef GRAPGH_H
#define GRAPGH_H

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdlib>

#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

// Graph: user -> followers
using Graph = unordered_map<int, vector<int>>;

Graph buildGraphFromXML(const string& xmlContent);
void exportToDot(const Graph& graph, const string& dotFile);
void renderGraph(const string& dotFile, const string& outputImage);
void drawXMLGraph(const string& xmlContent, const string& outputImage);


#endif