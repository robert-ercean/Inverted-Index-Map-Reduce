#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
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
} filesControlBlock;

class Reducer : public AbstractThread {
public:
    Reducer(filesControlBlock *fcb, int id) : fcb(fcb), id(id) {}
protected:
    /* Builds the heap of the alphabet character located at charIdx and
     * then writes it to the respective file */
    void buildHeap(int charIdx) {
        priority_queue<pair<string, vector<int>>, vector<pair<string, vector<int>>>, decltype(&pqComparator)> heap(pqComparator);
        char c = 'a' + charIdx;
        for (auto &pair : fcb->aggregateList) {
            string word = pair.first;
            char ch = word[0];
            if (ch != c) continue;
            sort(pair.second.begin(), pair.second.end());
            heap.push(pair);
        }
        /* Write the heap to its file */
        string filename = "out/" + string(1, c) + ".txt";
        ofstream out(filename);
        if (!out.is_open()) {
            cerr << "Error in opening file " << filename << " during writing phase in heapify()" << endl;
            return;
        }
        while(!heap.empty()) {
            auto &entry = heap.top();
            int arrSize = (int) entry.second.size();
            string line = entry.first + ":[";
            for (int i = 0; i < arrSize; i++) {
                line += to_string(entry.second[i]);
                string delim = (i == arrSize - 1) ? "" : " ";
                line += delim;
            }
            line += "]\n";
            out << line;
            heap.pop();
        }
        out.close();
    }
    void heapify() {
        /* Wait for all the Reducer Threads to finish building the shared aggregateList */
        pthread_barrier_wait(&fcb->heapifyBarrier);
        int static_idx = id;
        if (static_idx >= ALPHABET_SIZE) {
            cerr << "Reducer " << id << " STATIC idx exceeds max alphabet size" << endl;
            return;
        }
        buildHeap(static_idx);
        while (true) {
            int dynamic_idx = fcb->alphabetIdx.fetch_add(1);
            if (dynamic_idx >= ALPHABET_SIZE) {
                cerr << "Reducer " << id << " DYNAMIC idx exceeds max alphabet size" << endl;
                return;
            }
            buildHeap(dynamic_idx);
        }
    }
    void buildAggregateList(int idx) {
        unordered_map<string, int> &partialList = fcb->partialLists[idx];
        unordered_map<string, vector<int>> &aggregateList = fcb->aggregateList;
        
        for (auto &pair : partialList) {
            string word = pair.first;
            int fileId = pair.second;
            pthread_mutex_lock(&fcb->aggregateListMutex);
            aggregateList[word].emplace_back(fileId);
            pthread_mutex_unlock(&fcb->aggregateListMutex);
        }
    }
    virtual void InternalThreadFunc() {
        pthread_barrier_wait(&fcb->barrier);
        cout << "Hello from reducer : " << id << endl;

        /* Begin the static index partial list aggregation */
        int static_idx = id;
        if (static_idx >= (int) fcb->partialLists.size()) {
            cerr << "[STATIC] idx of Reducer " << id << " exceeds partial lists size" << endl;
            heapify();
            return;
        }
        buildAggregateList(static_idx);

        /* Begin the dynamic index partial list aggregation */
        while (true) {
            int dynamic_idx = fcb->partialListsIdx.fetch_add(1);
            if (dynamic_idx >= (int) fcb->partialLists.size()) {
                cerr << "[DYNAMIC] idx of Reducer " << id << " exceeds partial lists size" << endl;
                heapify();
                return;
            }
            buildAggregateList(dynamic_idx);
        }
    }
private:
    filesControlBlock *fcb;
    int id;
};

class Mapper : public AbstractThread {
public:
    Mapper(int id, filesControlBlock *fcb) : id(id), fcb(fcb) {}

protected:
    void strip_word(string &word) {
        for (int i = 0; i < (int) word.size(); i++) {
            char c = word[i];
            if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
                word.erase(i, 1);
                i--;
            }
            word[i] = tolower(word[i]);
        }
    }

    int processFile(int idx) {
        unordered_map<string, int> partialList;
        cout << "Mapper " << id << " is processing file: " << fcb->files[idx] << endl;

        ifstream input("../checker/" + fcb->files[idx]);
        if (!input.is_open()) {
            return 0;
        }
   
        string word;
        while (input >> word) {
            strip_word(word);
            int fileId = idx + 1;
            partialList[word] = fileId;
        }
        /* Now push the newly created partial list to the shared
         * partial list vector in the file control block */
        pthread_mutex_lock(&fcb->partialListsMutex);
        fcb->partialLists.push_back(partialList);
        pthread_mutex_unlock(&fcb->partialListsMutex);

        input.close();
        return 1;
    }

    virtual void InternalThreadFunc() override {
        int static_idx = id;

        /* Process the statically assigned file id first */
        if (static_idx >= (int) fcb->files.size()) {
            cerr << "Static IDX exceeds maximum file number, exiting Mapper with ID: " << id << endl;
            return;
        }
        if (!processFile(static_idx)) {
            cerr << "[STATIC]Mapper failed to open the file with id: " << static_idx << endl;
            return;
        }
        /* If there are anymore files left to 
         * process, fetch their atomic id and start parsing */ 
        while (true) {
            int dynamic_idx = fcb->fileIdx.fetch_add(1);
            
            if (dynamic_idx >= (int) fcb->files.size())
                break;
            
            if (!processFile(dynamic_idx)) {
                cerr << "[DYNAMIC]Mapper failed to open the file with id: " << dynamic_idx << endl;
                return;
            }
        }
        pthread_barrier_wait(&fcb->barrier);
    }
private:
    int id;
    filesControlBlock *fcb;
};