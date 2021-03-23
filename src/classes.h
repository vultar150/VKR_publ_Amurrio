#ifndef CLASSES_H
#define CLASSES_H

#include <vector>
#include <unordered_map>

#ifndef XMLCheckResult
    #define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("Error: %i\n", a_eResult); exit(a_eResult); }
#endif


struct TaskPosition
{
    int _graphId;
    int _taskId;
    TaskPosition(int a = 0, int b = 0): _graphId(a), _taskId(b) {}
    bool operator==(const TaskPosition & taskPos) {
        return _graphId == taskPos._graphId && _taskId == taskPos._taskId;
    }
};


class Task {
public:
    int i; // graph id
    int j; // task id
    int major_frame;
    int part_id;
    int priority;
    int processorNum;
    int T; // period
    int bcet;
    int wcet;
    int J; // jitter
    int phi; // phase
    int R; // WCRT
    int R_b; // lower bound of BCRT
    bool isMessage;
    bool critPathWasComputed;
    std::vector<Task*> predecessors;
    std::vector<Task*> successors;
    std::vector<std::unordered_map<int, Task*>> hp;

    Task(int i, int j, int major_frame, int part_id, int priority,
         int processorNum, int T, int bcet, int wcet, bool isMessage = false, 
         bool critPathWasComputed = false);
    void print();
};


using WinType = std::unordered_map<int, std::unordered_map<int, Task*>>;


class Processors: public std::unordered_map<int, std::vector<TaskPosition> >
{
    public:
        void outInfo() const;
};

#endif // CLASSES_H
