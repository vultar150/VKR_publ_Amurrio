#include <iostream>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <fstream>
#include <ctime>
#include <map>

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

    time = clock() - time;

    std::ofstream fout("output.txt", std::ios::app);
    fout << "WCRT = " << tasks[targetTask]->R << std::endl;
    fout << "Period = " << tasks[targetTask]->T << std::endl;
    fout << "Task num = " << targetTask << std::endl;
    fout << "Time = " << static_cast<float>(time)/CLOCKS_PER_SEC << std::endl;
    fout << std::endl;
    fout.close();

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

    int averageWCRT = 0;
    int maxWCRT = 0;
    int count = 0;
    bool schedulability = true;
    int countUnschTasks = 0;

    std::map<int, int> wcrts;
    // std::cout << "Graphs info: /////////////////////////" << std::endl;
    for (auto & graph : graphs) {
        for (auto & task : graph) {
            if (not task.second->isMessage) {
                // task.second->print();
                if (maxWCRT < task.second->R) {
                    maxWCRT = task.second->R;
                }
                averageWCRT += task.second->R;
                if (task.second->R > task.second->T) {
                    schedulability = false;
                    countUnschTasks++;
                }
                count++;
                wcrts[task.second->j] = task.second->R;
            }
        }
        // std::cout << std::endl;
    }
    averageWCRT /= count;

    for (auto task : wcrts) {
        std::cout << "t_" << task.first << "  R = " << task.second << std::endl;
    }

    // std::cout << std::endl;
    // std::cout << "WCRT = " << tasks[targetTask]->R << std::endl;
    // std::cout << "Period = " << tasks[targetTask]->T << std::endl;
    // std::cout << "Task num = " << targetTask << std::endl;
    // std::cout << "Time = " << static_cast<float>(time)/CLOCKS_PER_SEC << std::endl;
    // std::cout << "Schedulability: " << schedulability << std::endl;
    // std::cout << "Number of tasks: " << count << std::endl;
    // std::cout << "Number of unsched tasks: " << countUnschTasks << std::endl;
    std::cout << "Average WCRT: " << averageWCRT << std::endl;
    std::cout << "max WCRT: " << maxWCRT << std::endl;

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