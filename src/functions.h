#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "tinyxml2.h" // additional library (XML Parser)
#include "classes.h"

using namespace tinyxml2;


void setTasks(XMLNode * xmlNode,
              std::unordered_map<int,Task*> & tasks,
              std::unordered_map<int, bool> & usd,
              Processors & processors,
              int & maxId,
              WinType & G_u);

void setLinks(XMLNode *xmlNode, std::unordered_map<int,Task*> & tasks,
              std::unordered_map<int, bool> & usd, int & maxId);

void setNumGraph(const int & id,
                 std::unordered_map<int, Task*> & tasks,
                 std::unordered_map<int, bool> & usd,
                 const int & graphNum,
                 std::vector<std::unordered_map<int, Task*>> & graphs);

void initPhaseAndJitters(Task * task, float & critPathWCET);

void initGraphsJittersAndPhases(
                  std::unordered_map<int, Task*> & tasks,
                  std::unordered_map<int, bool> & usd,
                  std::vector<std::unordered_map<int, Task*>> & graphs,
                  Processors & processors);

void setHp(std::vector<std::unordered_map<int, Task*>> & graphs,
           Task * task, Processors & processors);

void assignHigherPrioritySet(std::vector<std::unordered_map<int, Task*>> & graphs,
                             Processors & processors);

float mod(float a, float b);

float delta_ijk(Task* task_ij, Task* task_ik);

float Wik(std::unordered_map<int, Task*> &hp_i, Task* k, float t);

float interference(std::unordered_map<int, Task*> &hp_i, Task* k, float z);

float approximation_func(std::unordered_map<int, Task*> &hp_i, float z);

float compute_L_or_Wabcd(Task* target, Task* c, Task* d, 
                       int p, int p0, float delta, int size, 
                       bool is_L, WinType &G_u);

float Rabcd(Task* target, Task* c, Task* d, int p, int p0, float delta, int size, WinType &G_u);

float R_max_p(Task* target, Task* c, Task* d, int p0, float delta, int size, WinType &G_u);

float R_max_d(Task* target, Task* c, int size, WinType &G_u);

float R_max_c(Task* target, int size, WinType &G_u);

float Rab(Task* target, int size, WinType &G_u);

void update_jitters(std::vector<std::unordered_map<int, Task*>> &graphs);

void WCDO(std::vector<std::unordered_map<int, Task*>> &graphs, WinType &G_u);

#endif // FUNCTIONS_H
