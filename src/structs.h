#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "AbstractThread.h"

#define ALPHABET_SIZE 26

using namespace std;

bool pqComparator(const pair<string, vector<int>> &a, const pair<string, vector<int>> &b) {
    if (a.second.size() != b.second.size()) {
        return a.second.size() < b.second.size();
    }
    return a.first > b.first;
}

typedef struct {
    vector<pthread_mutex_t> heapMutex;
    vector<priority_queue<pair<string, vector<int>>, vector<pair<string, vector<int>>>, decltype(&pqComparator)>> heaps;
} heaps;

typedef struct {
    vector<string> files;
    atomic<int> fileIdx;
    
    atomic<int> partialListsIdx;
    vector<unordered_map<string, int>> partialLists;

    pthread_barrier_t barrier;
    pthread_barrier_t heapifyBarrier;
    pthread_mutex_t partialListsMutex;
    pthread_mutex_t aggregateListMutex;

    unordered_map<string, vector<int>> aggregateList;
    atomic<int> alphabetIdx;
    heaps pqs;
} filesControlBlock;