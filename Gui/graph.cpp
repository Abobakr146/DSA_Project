#include "graph.h"

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
  Render DOT using bundled Graphviz
--------------------------------------------------*/
void renderGraph(const string& dotFile, const string& outputImage)
{
    // Use system-installed Graphviz (assumes dot is in PATH)
    string command = "dot -Tjpg " + dotFile + " -o " + outputImage;
    int ret = system(command.c_str());
    if (ret != 0) {
        cerr << "Error: Graphviz failed with exit code " << ret << endl;
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
