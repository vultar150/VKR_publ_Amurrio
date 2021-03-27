#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>

#include "functions.h"


const float eps = 0.001;


void setTasks(XMLNode *xmlNode,
              std::unordered_map<int,Task*> & tasks,
              std::unordered_map<int, bool> & usd,
              Processors & processors,
              int & maxId,
              WinType & G_u) {
    XMLError eResult;

    XMLElement * pListElement = xmlNode->FirstChildElement("partition");

    if (pListElement == nullptr) exit(XML_ERROR_PARSING_ELEMENT);

    G_u[-1] = std::unordered_map<int, Task*>(); // dummy partition for messages
    while (pListElement != nullptr) {
        int partitionId, processorNum, major_frame;
        eResult = pListElement->QueryIntAttribute("id", &partitionId);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryIntAttribute("proc", &processorNum);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryIntAttribute("maj_fr", &major_frame);
        XMLCheckResult(eResult);
        std::vector<TaskPosition> taskPos;
        XMLElement * pListTasks = pListElement->FirstChildElement("task");
        while (pListTasks != nullptr) {
            int id, priority;
            float period, BCET, WCET; // i, j, major_frame, part_id, priority, processorNum, T, bcet, wcet
            eResult = pListTasks->QueryIntAttribute("id", &id);
            XMLCheckResult(eResult);
            if (maxId < id) maxId = id;
            eResult = pListTasks->QueryIntAttribute("prio", &priority);
            XMLCheckResult(eResult);
            eResult = pListTasks->QueryFloatAttribute("period", &period);
            XMLCheckResult(eResult);
            eResult = pListTasks->QueryFloatAttribute("bcet", &BCET);
            XMLCheckResult(eResult);
            eResult = pListTasks->QueryFloatAttribute("wcet", &WCET);
            XMLCheckResult(eResult);
            usd[id] = false;
            tasks[id] = new Task(-1, id, major_frame, partitionId, priority,
                                 processorNum, period, BCET, WCET);
            pListTasks = pListTasks->NextSiblingElement("task");
        }
        XMLElement * pListWindows = pListElement->FirstChildElement("window");
        G_u[partitionId] = std::unordered_map<int, Task*>();
        int count = 0;
        int left = 0;
        while (pListWindows != nullptr) {
            int start, stop;
            eResult = pListWindows->QueryIntAttribute("start", &start);
            XMLCheckResult(eResult);
            eResult = pListWindows->QueryIntAttribute("stop", &stop);
            XMLCheckResult(eResult);
            int ET = start - left;
            if (ET > 0) {
                Task* win = new Task(-1, count, major_frame, partitionId, -1,
                                     processorNum, (float)major_frame, (float)ET, (float)ET);
                win->phi = left; win->J = 0.;
                win->R_b = win->R = win->phi + (float)ET;
                G_u[partitionId][count] = win;
                count++;
            }
            left = stop;
            pListWindows = pListWindows->NextSiblingElement("window");
        }
        int ET = major_frame - left;
        if (ET > 0) {
            Task* win = new Task(-1, count, major_frame, partitionId, -1,
                                 processorNum, (float)major_frame, (float)ET, (float)ET);
            win->phi = left; win->J = 0.;
            win->R_b = win->R = win->phi + (float)ET;
            G_u[partitionId][count] = win;
            count++;
        }
        processors[processorNum] = taskPos;
        pListElement = pListElement->NextSiblingElement("partition");
    }
}


void setLinks(XMLNode *xmlNode, std::unordered_map<int,Task*> & tasks,
              std::unordered_map<int, bool> & usd, int & maxId) {
    XMLError eResult;

    XMLElement * pListElement = xmlNode->FirstChildElement("tlink");

    int mf = (*tasks.begin()).second->major_frame;
    int messageId = maxId + 1;
    float period = 0.;

    while (pListElement != nullptr) {
        int src, dst;
        float bctt, wctt;
        eResult = pListElement->QueryIntAttribute("src", &src);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryIntAttribute("dist", &dst);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryFloatAttribute("bctt", &bctt);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryFloatAttribute("wctt", &wctt);
        XMLCheckResult(eResult);
        if (wctt > 0) {
            period = tasks[src]->T;
            // i, j, major_frame, part_id, priority, processorNum, T, bcet, wcet, isMessage
            tasks[messageId] = new Task(-1, messageId, mf, -1, -1, -1,
                                        period, bctt, wctt, true);
            usd[messageId] = false;
            tasks[src]->successors.push_back(tasks[messageId]);
            tasks[messageId]->predecessors.push_back(tasks[src]);
            tasks[dst]->predecessors.push_back(tasks[messageId]);
            tasks[messageId]->successors.push_back(tasks[dst]);
            messageId++;
        } else {
            tasks[src]->successors.push_back(tasks[dst]);
            tasks[dst]->predecessors.push_back(tasks[src]);

        }
        pListElement = pListElement->NextSiblingElement("tlink");
    }
}


void setNumGraph(const int & id,
                 std::unordered_map<int, Task*> & tasks,
                 std::unordered_map<int, bool> & usd,
                 const int & graphNum,
                 std::vector<std::unordered_map<int, Task*>> & graphs) {
    usd[id] = true;
    tasks[id]->i = graphNum;
    graphs[graphNum][id] = tasks[id];

    for (auto & successor : tasks[id]->successors) {
        if (!usd[successor->j]) {
            setNumGraph(successor->j, tasks, usd, graphNum, graphs);
        }
    }
    for (auto & predecessor : tasks[id]->predecessors) {
        if (!usd[predecessor->j]) {
            setNumGraph(predecessor->j, tasks, usd, graphNum, graphs);
        }
    }
}


void initPhaseAndJitters(Task * task, float & critPathWCET) {
    if (task->critPathWasComputed) {
        critPathWCET = task->R;
        return;
    }
    float predCritPathBCET = 0;
    float predCritPathWCET = 0;
    for (auto & pred : task->predecessors) {
        initPhaseAndJitters(pred, critPathWCET);
        if (predCritPathBCET < pred->phi + pred->bcet) {
            predCritPathBCET = pred->phi + pred->bcet;
        }
        if (predCritPathWCET < critPathWCET) {
            predCritPathWCET = critPathWCET;
        }
    }
    task->phi = predCritPathBCET;
    task->R_b = predCritPathBCET + task->bcet;
    task->J = predCritPathWCET - task->phi;
    critPathWCET = task->R = predCritPathWCET + task->wcet;
    task->critPathWasComputed = true;
}


void initGraphsJittersAndPhases(
                  std::unordered_map<int, Task*> & tasks,
                  std::unordered_map<int, bool> & usd,
                  std::vector<std::unordered_map<int, Task*>> & graphs,
                  Processors & processors) {
    int graphNum = 0;
    for (auto & task : tasks) {
        int id = task.second->j;
        if (task.second->successors.empty())
        {
            float critPathWCET = 0;
            initPhaseAndJitters(task.second, critPathWCET);
        }
        if (!usd[id]) {
            graphs.push_back(std::unordered_map<int, Task*>());
            setNumGraph(id, tasks, usd, graphNum, graphs);
            graphNum++;
        }
        if (!task.second->isMessage) {
            TaskPosition taskPosition(task.second->i, id);
            processors[task.second->processorNum].push_back(taskPosition);
        }
    }
}


void setHp(std::vector<std::unordered_map<int, Task*>> & graphs,
           Task * task, Processors & processors) {
    int graphId = task->i;
    for (auto & taskPos : processors[task->processorNum]) {
        int simProcGraphId = taskPos._graphId;
        int simProcTaskId = taskPos._taskId;
        Task * taskOnSimilarProc = graphs[simProcGraphId][simProcTaskId];
        if ((task->priority > taskOnSimilarProc->priority) or (taskOnSimilarProc == task)) {
            continue;
        }
        else if (task->part_id == taskOnSimilarProc->part_id) {
            task->hp[simProcGraphId][simProcTaskId] = taskOnSimilarProc;
        }
    }
}


void assignHigherPrioritySet(std::vector<std::unordered_map<int, Task*>> & graphs,
                             Processors & processors) {
    int numOfGraphs = graphs.size();
    for (auto & graph : graphs) {
        for (auto & task : graph) {
            for(int i = 0; i < numOfGraphs; i++) {
                task.second->hp.push_back(std::unordered_map<int, Task*>());
            }
            if (!task.second->isMessage) {
                setHp(graphs, task.second, processors);
            }
        }
    }
}


float mod(float a, float b) {
    return a >= 0 ? fmod(a, b) : b - fmod(abs(a),b);
}


float delta_ijk(Task* task_ij, Task* task_ik) {
    return task_ij->T - mod(task_ik->phi + task_ik->J - task_ij->phi, task_ij->T);
}


float Wik(std::unordered_map<int, Task*> &hp_i, Task* k, float t) {
    float sum = 0;
    for (auto & j : hp_i) {
        float T = j.second->T;
        float delta = delta_ijk(j.second, k);
        sum += ( floor( (j.second->J + delta) / T ) + 
                 ceil( (t - delta) / T) ) * j.second->wcet;
    }
    return sum;
}


float interference(std::unordered_map<int, Task*> &hp_i, Task* k, float z) {
    return Wik(hp_i, k, z);
}


float approximation_func(std::unordered_map<int, Task*> &hp_i, float z) {
    float max = 0;
    float I = 0;
    for (auto & k : hp_i) {
       I = interference(hp_i, k.second, z);
       if (max < I) { max = I; }
    }
    return max;
}


float compute_L_or_Wabcd(Task* target, Task* c, Task* d,
                       int p, int p0, float delta, int size,
                       bool is_L, WinType &G_u) {
    float res = p * target->wcet;
    std::vector<float> I(size, 0.);
    int part_id = target->part_id;
    bool was_change = true;
    while (was_change) {
        was_change = false;
        int a = target->i;
        for (int i = 0; i < size; i++) {
            if (i != a) {
                I[i] = approximation_func(target->hp[i], res);
            }
        }
        float I_ac = interference(target->hp[a], c, res);
        float I_ud = interference(G_u[part_id], d, res);

        float sum = 0;
        for (int i = 0; i < size; i++) {
            if (i != a) sum += I[i];
        }

        float old_L = res;
        int term = is_L ? (int)ceil( (res - delta) / target->T ) : p;

        res = (term - p0 + 1) * target->wcet + I_ac + I_ud + sum;
        if (abs(old_L - res) > eps) was_change = true;
    }
    return res;
}


float Rabcd(Task* target, Task* c, Task* d, int p, int p0, float delta, int size, WinType &G_u) {
    float W = compute_L_or_Wabcd(target, c, d, p, p0, delta, size, false, G_u);
    return W - delta - (p - 1) * target->T + target->phi;
}


float R_max_p(Task* target, Task* c, Task* d, int p0, float delta, int size, WinType &G_u) {
    float L = compute_L_or_Wabcd(target, c, d, 1, p0, delta, size, true, G_u);
    int pL = (int)ceil((L - delta) / target->T);
    float max_p = 0;
    float R = 0;
    for (int p = p0; p <= pL; p++) {
        R = Rabcd(target, c, d, p, p0, delta, size, G_u);
        if (max_p < R) max_p = R;
    }
    R = max_p;
    return R;
}


float R_max_d(Task* target, Task* c, int size, WinType &G_u) {
    float delta = delta_ijk(target, c);
    int p0 = - (int)floor((target->J + delta) / target->T) + 1;
    float R = 0;
    int part_id = target->part_id;
    float max_d = 0;
    if (G_u[part_id].size() == 0) {
        R = R_max_p(target, c, nullptr, p0, delta, size, G_u);
    } else {
        for (auto & d : G_u[part_id]) {
            R = R_max_p(target, c, d.second, p0, delta, size, G_u);
            if (max_d < R) max_d = R;
        }
        R = max_d;
    }
    return R;
}


float R_max_c(Task* target, int size, WinType &G_u) {
    float max_c = 0;
    float R = 0;
    int a = target->i;
    for (auto & c : target->hp[a]) {
        R = R_max_d(target, c.second, size, G_u);
        if (max_c < R) max_c = R;
    }
    R = R_max_d(target, target, size, G_u);
    if (max_c < R) max_c = R;
    R = max_c;
    return R;
}


float Rab(Task* target, int size, WinType &G_u) {
    float R = R_max_c(target, size, G_u);
    return R;
}


void update_jitters(std::vector<std::unordered_map<int, Task*>> &graphs) {
    for (auto & graph : graphs) {
        for (auto & task : graph) {
            float max_R = 0;
            for (auto & pred : task.second->predecessors) {
                if (max_R < pred->R) {
                    max_R = pred->R;
                }
            }
            task.second->J = max_R - task.second->phi;
        }
    }
}


void WCDO(std::vector<std::unordered_map<int, Task*>> &graphs, WinType &G_u) {
    bool was_change = true;
    int it = 0;
    while (was_change) {
        was_change = false;
        std::cout << "it = " << it << std::endl;
        for (auto & graph : graphs) {
            for (auto & task : graph) {
                float R_ab = Rab(task.second, graphs.size(), G_u);
                float old_R = task.second->R;
                task.second->R = R_ab;
                if (abs(task.second->R - old_R) > eps) was_change = true;
            }
        }
        update_jitters(graphs);
        it++;
    }
}

