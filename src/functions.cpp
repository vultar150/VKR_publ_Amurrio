#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>

#include "functions.h"


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
            int id, priority, period, BCET, WCET; // i, j, major_frame, part_id, priority, processorNum, T, bcet, wcet
            eResult = pListTasks->QueryIntAttribute("id", &id);
            XMLCheckResult(eResult);
            if (maxId < id) maxId = id;
            eResult = pListTasks->QueryIntAttribute("prio", &priority);
            XMLCheckResult(eResult);
            eResult = pListTasks->QueryIntAttribute("period", &period);
            XMLCheckResult(eResult);
            eResult = pListTasks->QueryIntAttribute("bcet", &BCET);
            XMLCheckResult(eResult);
            eResult = pListTasks->QueryIntAttribute("wcet", &WCET);
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
                                     processorNum, major_frame, ET, ET);
                win->phi = left; win->J = 0;
                win->R_b = win->R = win->phi + ET;
                G_u[partitionId][count] = win;
                count++;
            }
            left = stop;
            pListWindows = pListWindows->NextSiblingElement("window");
        }
        int ET = major_frame - left;
        if (ET > 0) {
            Task* win = new Task(-1, count, major_frame, partitionId, -1,
                                 processorNum, major_frame, ET, ET);
            win->phi = left; win->J = 0;
            win->R_b = win->R = win->phi + ET;
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
    int period = 0;

    while (pListElement != nullptr) {
        int src, dst, bctt, wctt;
        eResult = pListElement->QueryIntAttribute("src", &src);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryIntAttribute("dist", &dst);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryIntAttribute("bctt", &bctt);
        XMLCheckResult(eResult);
        eResult = pListElement->QueryIntAttribute("wctt", &wctt);
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


void initPhaseAndJitters(Task * task, int & critPathWCET) {
    if (task->critPathWasComputed) {
        critPathWCET = task->R;
        return;
    }
    int predCritPathBCET = 0;
    int predCritPathWCET = 0;
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
            int critPathWCET = 0;
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


int mod(int a, int b) {
    return a >= 0 ? a % b : b - abs(a) % b;
}


int upper(int a, int b) {
    return a < 0 ? a / b : (a % b == 0 ? a / b : (a / b) + 1);
}


int delta_ijk(Task* task_ij, Task* task_ik) {
    return task_ij->T - mod(task_ik->phi + task_ik->J - task_ij->phi, task_ij->T);
}


int Wik(std::unordered_map<int, Task*> &hp_i, Task* k, int t) {
    int sum = 0;
    for (auto & j : hp_i) {
        int T = j.second->T;
        int delta = delta_ijk(j.second, k);
        sum += ( ( (j.second->J + delta) / T) + upper(t - delta, T)) * j.second->wcet;
    }
    return sum;
}


int interference(std::unordered_map<int, Task*> &hp_i, Task* k, int z) {
    // bool was_change = true;
    // was_change = false;
    // int old_W = W;
    // W = z + Wik(hp_i, k, W);
    // if (W != old_W) was_change = true;
    // int W = z;
    // while (was_change) {
    //     was_change = false;
    //     int old_W = W;
    //     W = z + Wik(hp_i, k, W);
    //     if (W != old_W) was_change = true;
    // }
    return Wik(hp_i, k, z);
}


int approximation_func(std::unordered_map<int, Task*> &hp_i, int z) {
    int max = 0;
    int I = 0;
    for (auto & k : hp_i) {
       I = interference(hp_i, k.second, z);
       if (max < I) { max = I; }
    }
    return max;
}


int compute_L_or_Wabcd(Task* target, Task* c, Task* d,
                       int p, int p0, int delta, int size,
                       bool is_L, WinType &G_u) {
    int res = p * target->wcet;
    std::vector<int> I(size, 0);
    int part_id = target->part_id;
    bool was_change = true;
    // std::cout << "curr tast: i = " << target->i << "  j = " << target->j << std::endl;
    // std::cout << "task c : i = " << c->i << " j = " << c->j << std::endl;
    // if (d) {
    //     std::cout << "task d : i = " << d->i << " j = " << d->j << std::endl;
    // }
    // std::cout << "/////////////////" << std::endl;
    // if (is_L) {
    //     std::cout << "L iterations" << std::endl;
    // } else {
    //     std::cout << "W iterations" << std::endl;
    // }
    // std::cout << "p0 = " << p0 << std::endl;

    while (was_change) {
        // if (is_L) {
        //     std::cout << "current L = " << res << std::endl;
        // } else {
        //     std::cout << "current W = " << res << std::endl;
        // }
        // std::cout << "Z:\n";
        // for (int i = 0; i < z.size(); i++) {
        //     std::cout << "\tz[" << i << "] = " << z[i] << std::endl; 
        // }
        // std::cout << "\tz_u = " << z_u << std::endl; 
        // std::cout << "//////\n";
        // std::cout << "I:\n";
        // for (int i = 0; i < z.size(); i++) {
        //     std::cout << "\tI[" << i << "] = " << I[i] << std::endl; 
        // }
        // std::cout << "\tI_u = " << I_u << std::endl; 
        // std::cout << "######\n";
        was_change = false;
        int a = target->i;
        int new_I = 0;
        for (int i = 0; i < size; i++) {
            if (i != a) {
                new_I = approximation_func(target->hp[i], res);
                // if (I[i] != new_I) was_change = true;
                I[i] = new_I;
            }
        }
        // new_I = approximation_func(G_u[part_id], res);
        // if (I_u != new_I) was_change = true;
        // I_u = new_I;

        
        int I_ac = interference(target->hp[a], c, res);
        int I_ud = interference(G_u[part_id], d, res);

        int sum = 0;
        for (int i = 0; i < size; i++) {
            if (i != a) sum += I[i];
        }

        int old_L = res;
        int term = is_L ? upper(res - delta, target->T) : p;
        // if (term < 50 and term > -50) {
        //     std::cout << "term = " << term << std::endl;
        // }
        res = (term - p0 + 1) * target->wcet + I_ac + I_ud + sum;
        // std::cout << "res = " << res << std::endl;
        if (old_L != res) was_change = true;
        // for (int i = 0; i < size; i++) {
        //     z[i] = res - I[i];
        // }
        // z_u = res - I_u;
    }
    // std::cout << "/////////////////" << std::endl;
    return res;
}


int Rabcd(Task* target, Task* c, Task* d, int p, int p0, int delta, int size, WinType &G_u) {
    int W = compute_L_or_Wabcd(target, c, d, p, p0, delta, size, false, G_u);
    return W - delta - (p - 1) * target->T + target->phi;
}


int R_max_p(Task* target, Task* c, Task* d, int p0, int delta, int size, WinType &G_u) {
    int L = compute_L_or_Wabcd(target, c, d, 1, p0, delta, size, true, G_u);
    int pL = upper(L - delta, target->T);
    int max_p = 0;
    int R = 0;
    for (int p = p0; p <= pL; p++) {
        R = Rabcd(target, c, d, p, p0, delta, size, G_u);
        if (max_p < R) max_p = R;
    }
    R = max_p;
    return R;
}


int R_max_d(Task* target, Task* c, int size, WinType &G_u) {
    int delta = delta_ijk(target, c);
    int p0 = - ((target->J + delta) / target->T) + 1;
    int R = 0;
    int part_id = target->part_id;
    int max_d = 0;
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

int R_max_c(Task* target, int size, WinType &G_u) {
    int max_c = 0;
    int R = 0;
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


int Rab(Task* target, int size, WinType &G_u) {
    int R = R_max_c(target, size, G_u);
    return R;
}


void update_jitters(std::vector<std::unordered_map<int, Task*>> &graphs) {
    for (auto & graph : graphs) {
        for (auto & task : graph) {
            int max_R = 0;
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
                // std::cout << "CURRENT task: i = " << task.second->i << "  j = " << task.second->j << std::endl;
                int R_ab = Rab(task.second, graphs.size(), G_u);
                int old_R = task.second->R;
                task.second->R = R_ab;
                if (task.second->R != old_R) was_change = true;
            }
        }
        update_jitters(graphs);
        it++;
    }
}

