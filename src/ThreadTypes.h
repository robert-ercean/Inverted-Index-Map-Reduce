#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include "AbstractThread.h"

using namespace std;

typedef struct {
    vector<string> files;
    atomic<int> fileIdx;
    
    atomic<int> partialListsIdx;
    vector<unordered_map<string, vector<int>>> partialLists;

    pthread_barrier_t barrier;
    pthread_mutex_t partialListsMutex;
    pthread_mutex_t aggregateListMutex;

    unordered_map<string, vector<int>> aggregateList;
} filesControlBlock;


class Reducer : public AbstractThread {
public:
    Reducer(filesControlBlock *fcb, int id) : fcb(fcb), id(id) {} 
protected:
    void buildAggregateList(int idx) {
        unordered_map<string, vector<int>> &partialList = fcb->partialLists[idx];
        unordered_map<string, vector<int>> &aggregateList = fcb->aggregateList;
        
        for (auto &pair : partialList) {
            string word = pair.first;
            for (int &id : pair.second) {
                pthread_mutex_lock(&fcb->aggregateListMutex);
                aggregateList[word].emplace_back(id);
                pthread_mutex_unlock(&fcb->aggregateListMutex);
            }
        }
    }
    virtual void InternalThreadFunc() {
        pthread_barrier_wait(&fcb->barrier);
        cout << "Hello from reducer : " << id << endl;

        /* Begin the static index partial list aggregation */
        int static_idx = id;
        if (static_idx >= (int) fcb->partialLists.size()) {
            cerr << "[STATIC] idx of Reducer " << id << " exceeds partial lists size" << endl;
            return;
        }
        buildAggregateList(static_idx);

        /* Begin the dynamic index partial list aggregation */
        while (true) {
            int dynamic_idx = fcb->partialListsIdx.fetch_add(1);
            if (dynamic_idx >= (int) fcb->partialLists.size()) {
                cerr << "[DYNAMIC] idx of Reducer " << id << " exceeds partial lists size" << endl;
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

    int processFile(int idx, unordered_map<string, vector<int>> *partialList) {
        cout << "Mapper " << id << " is processing file: " << fcb->files[idx] << endl;

        ifstream input("../checker/" + fcb->files[idx]);
        if (!input.is_open()) {
            return 0;
        }
   
        string word;
        while (input >> word) {
            strip_word(word);
            vector<int> &arr = (*partialList)[word];
            int fileId = idx + 1;
            if (find(arr.begin(), arr.end(), fileId) == arr.end()) {
                arr.push_back(fileId);
            }
        }
   
        input.close();
        return 1;
    }

    virtual void InternalThreadFunc() override {
        unordered_map<string, vector<int>> partialList;
        int static_idx = id;

        /* Process the statically assigned file id first */
        if (static_idx >= (int) fcb->files.size()) {
            cerr << "Static IDX exceeds maximum file number, exiting Mapper with ID: " << id << endl;
            return;
        }
        if (!processFile(static_idx, &partialList)) {
            cerr << "[STATIC]Mapper failed to open the file with id: " << static_idx << endl;
            return;
        }
        /* If there are anymore files left to 
         * process, fetch their atomic id and start parsing */ 
        while (true) {
            int dynamic_idx = fcb->fileIdx.fetch_add(1);
            
            if (dynamic_idx >= (int) fcb->files.size())
                break;
            
            if (!processFile(dynamic_idx, &partialList)) {
                cerr << "[DYNAMIC]Mapper failed to open the file with id: " << dynamic_idx << endl;
                return;
            }
        }
        /* Now push the newly created partial list to the shared
         * partial list vector in the file control block */
        pthread_mutex_lock(&fcb->partialListsMutex);
        fcb->partialLists.push_back(partialList);
        pthread_mutex_unlock(&fcb->partialListsMutex);
        pthread_barrier_wait(&fcb->barrier);
    }
private:
    int id;
    filesControlBlock *fcb;
};
