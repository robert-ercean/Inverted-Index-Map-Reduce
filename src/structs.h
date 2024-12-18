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

typedef struct {
    string filename;
    int id;
    intmax_t size;
} file;

/* Comparator used in building the partial / merged char-specific heaps */
bool pqComparator(const entry &a, const entry &b) {
    if (a.ids.size() != b.ids.size()) {
        return a.ids.size() < b.ids.size();
    }
    return a.word > b.word;
}


bool comp(const priority_queue<entry, vector<entry>, decltype(&pqComparator)>&a, const priority_queue<entry, vector<entry>, decltype(&pqComparator)>& b) {
    return a.size() > b.size();
}

typedef struct {
    int mappersCount;
    pthread_barrier_t reduceBarrier;

    /* Indexed by [ALPHABET_CHAR][MAPPER_ID][ENTRY] */
    vector<vector<vector<entry>>> partialEntries;
    vector<priority_queue<entry, vector<entry>, decltype(&pqComparator)>> mergedHeaps;
    vector<vector<intmax_t>> chFreq;

} filesControlBlock;