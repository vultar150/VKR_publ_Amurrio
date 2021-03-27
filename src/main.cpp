#include <iostream>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <ctime>

#include "classes.h"
#include "functions.h"



int main(int argc, char *argv[]) {
    clock_t time;
    time = clock();

    XMLError eResult;

    WinType G_u; // inverse unavailability function for each partiotion
    std::vector<std::unordered_map<int, Task*>> graphs;
    std::unordered_map<int,Task*> tasks;
    Processors processors;
    XMLDocument xmlDocument;

    eResult = xmlDocument.LoadFile(argv[1]); // load XML file
    XMLCheckResult(eResult);

    int targetTask = strtol(argv[2], 0, 10);

//// XML parsing//////////////////////////////////////////
    XMLNode * xmlNode = xmlDocument.FirstChildElement(); // root tag
    if (xmlNode == nullptr) return XML_ERROR_FILE_READ_ERROR;

    std::unordered_map<int, bool> usd; // for creating graphs
    int maxId = -1;
    setTasks(xmlNode, tasks, usd, processors, maxId, G_u);
    setLinks(xmlNode, tasks, usd, maxId);
    initGraphsJittersAndPhases(tasks, usd, graphs, processors);
    assignHigherPrioritySet(graphs, processors);

    WCDO(graphs, G_u);

    // std::cout << "G_u info: /////////////////////////" << std::endl;
    // for (auto & part : G_u) {
    //     std::cout << "partition id = " << part.first << std::endl;
    //     for (auto & win : part.second) {
    //         win.second->print();
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "///////////////////////////////////" << std::endl;
    // std::cout << std::endl;
    // std::cout << std::endl;

    std::cout << "Graphs info: /////////////////////////" << std::endl;
    for (auto & graph : graphs) {
        for (auto & task : graph) {
            task.second->print();
        }
        std::cout << std::endl;
    }

    for (auto & task : tasks) {
        delete task.second;
    }
    // std::cout << "//////////////////////////////////////" << std::endl;
    // std::cout << std::endl;
    // std::cout << std::endl;

    // std::cout << "PROCESSORS info: /////////////////" << std::endl;
    // processors.outInfo();
    // std::cout << "//////////////////////////////////" << std::endl;
    return 0;
}