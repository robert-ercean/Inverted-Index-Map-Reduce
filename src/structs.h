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

typedef struct {
    string word;
    vector<int> ids;
} entry;

bool pqComparator(const entry &a, const entry &b) {
    if (a.ids.size() != b.ids.size()) {
        return a.ids.size() < b.ids.size();
    }
    return a.word > b.word;
}

typedef struct {
    string filename;
    int id;
    intmax_t size;
} file;


bool filesPqComparator(const file& a, const file& b) {
    return a.size < b.size;
}

bool comp(const priority_queue<entry, vector<entry>, decltype(&pqComparator)>&a, const priority_queue<entry, vector<entry>, decltype(&pqComparator)>& b) {
    return a.size() > b.size();
}

typedef struct {
    pthread_barrier_t reduceBarrier;
    pthread_barrier_t writeBarrier;
    priority_queue<file, vector<file>, decltype(&filesPqComparator)> filesPq{filesPqComparator};

    vector<vector<priority_queue<entry, vector<entry>, decltype(&pqComparator)>>> heaps;
    vector<priority_queue<entry, vector<entry>, decltype(&pqComparator)>> heapsf;
    atomic<int>idx{0};
    
    vector<file> allFiles;
} filesControlBlock;