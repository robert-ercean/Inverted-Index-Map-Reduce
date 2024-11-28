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

/* Comparator used in building the partial / merged char-specific heaps */
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

    /* Indexed by [ALPHABET_CHAR][MAPPER_ID][ENTRY] */
    vector<vector<vector<entry>>> partialEntries;
    vector<priority_queue<entry, vector<entry>, decltype(&pqComparator)>> mergedHeaps;
    atomic<int>idx{0};
    vector<intmax_t> chFreq;
    vector<file> allFiles;
} filesControlBlock;