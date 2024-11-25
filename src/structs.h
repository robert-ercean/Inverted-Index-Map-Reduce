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
    string filename;
    int id;
    long size;
} file;

typedef struct {
    string word;
    int id;
} partial_entry;

bool filesPqComparator(const file& a, const file& b) {
    return a.size < b.size;
}

typedef struct {
    pthread_barrier_t reduceBarrier;
    pthread_mutex_t filesMutex;

    priority_queue<file, vector<file>, decltype(&filesPqComparator)> filesPq{filesPqComparator};
    vector<vector<vector<partial_entry>>> aggregateLists;
    atomic<int>idx;
} filesControlBlock;