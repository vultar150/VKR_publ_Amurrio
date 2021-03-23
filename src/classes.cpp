#include <iostream>
#include <cstdio>
#include "classes.h"


Task::Task(int i, int j, int major_frame, int part_id, int priority,
           int processorNum, int T, int bcet, int wcet, bool isMessage, 
           bool critPathWasComputed):
   i(i), j(j), major_frame(major_frame), part_id(part_id), 
   priority(priority), processorNum(processorNum), T(T), 
   bcet(bcet), wcet(wcet), isMessage(isMessage), 
   critPathWasComputed(critPathWasComputed) {}


void Task::print() {
    printf("t%d%d: phi = %d \tJ = %d \tR = %d \tR^b = %d\n", i, j, phi, J, R, R_b);
    // std::cout << "t" << i << j << ":\tphi = " << phi << "\tJ = " << J 
    //           << "\tR = " << R << "\tR_b = " << R_b << std::endl;
    // std::cout << "Task i=" << i << " j=" << j << std::endl;
    // std::cout << "major_frame = " << major_frame << std::endl;
    // std::cout << "part_id = " << part_id << std::endl;
    // std::cout << "priority = " << priority << std::endl;
    // std::cout << "processorNum = " << processorNum << std::endl;
    // std::cout << "T = " << T << std::endl;
    // std::cout << "BCET = " << bcet << std::endl;
    // std::cout << "WCET = " << wcet << std::endl;
    // std::cout << "Jitter = " << J << std::endl;
    // std::cout << "phi = " << phi << std::endl;
    // std::cout << "BCRT = " << R_b << std::endl;
    // std::cout << "WCRT = " << R << std::endl;
    // std::cout << "isMessage = " << isMessage << std::endl;
    // std::cout << "predecessors: " << std::endl;
    // for (int i = 0; i < predecessors.size(); i++) {
    //     std::cout << "\ti = " << predecessors[i]->i << ",  j = " << predecessors[i]->j << std::endl;
    // }
    // std::cout << "successors: " << std::endl;
    // for (int i = 0; i < successors.size(); i++) {
    //     std::cout << "\ti = " << successors[i]->i << ",  j = " << successors[i]->j << std::endl;
    // }
    // std::cout << "HP info:" << std::endl;
    // for (int i = 0; i < hp.size(); i++) {
    //     std::cout << "\tfor graph id = " << i << std::endl;
    //     for (auto & task : hp[i]) {
    //         std::cout << "\t\ti = " << task.second->i << ",  j = " << task.second->j << std::endl;
    //     }
    // }
    // std::cout << std::endl;
}


// Defining methods for "Processors" class
void Processors::outInfo() const
{
    for (auto p = begin(); p != end(); p++)
    {
        std::cout << "PE id = " << p->first << std::endl;
        std::cout << "Tasks: "  << std::endl;
        for (const auto & q : p->second)
        {
            printf("\tgraph id = %d , task id = %d\n", q._graphId, q._taskId);
        }
        std::cout << std::endl << std::endl;
    }
}
// end Defining methods for "Processors" class
