#include "structs.h"
#include <sstream>

using namespace std;

class Reducer : public AbstractThread {
public:
    Reducer(filesControlBlock *fcb, int id, int reducersCount) : fcb(fcb), id(id), reducersCount(reducersCount) {}
protected:
    void heapify(vector<vector<partial_entry>> &reducerArrs) {
        unordered_map<string, vector<int>> map;
        for (auto &mapperArr : reducerArrs) {
            for (partial_entry &pe : mapperArr) {
                string word = pe.word;
                int id = pe.id;
                map[word].push_back(id);
            }
        }
        for (auto &pair : map) {
            char ch = pair.first[0];
            entry e;
            e.word = pair.first;
            e.ids = pair.second;
            sort(e.ids.begin(), e.ids.end());

            pthread_mutex_lock(&fcb->heapsMutexes[ch - 'a']);
            fcb->heaps[ch - 'a'].push(e);
            pthread_mutex_unlock(&fcb->heapsMutexes[ch - 'a']);
        }
    }
    
    void writeToFile(int idx) {
        auto &pq = fcb->heaps[idx];
        ostringstream buffer;
        while (!pq.empty()) {
            entry e = pq.top();
            buffer << e.word << ":[";
            for (size_t i = 0; i < e.ids.size(); i++) {
                buffer << e.ids[i];
                if (i < e.ids.size() - 1) {
                    buffer << " ";
                }
            }
            buffer << "]\n";
            pq.pop();
        }
        ofstream out(string(1, idx + 'a') + ".txt");
        out << buffer.str();
    }

    virtual void InternalThreadFunc() {
        pthread_barrier_wait(&fcb->reduceBarrier);

        auto &reducerArrs = fcb->aggregateLists[id];
        unsigned int total_size = 0;
        for (auto &mapArr : fcb->aggregateLists[id]) {
            total_size += mapArr.size();
        }
        cout << "Reducer " << id << "processing total size of " << total_size << endl;
        heapify(reducerArrs);
        pthread_barrier_wait(&fcb->heapifyBarrier);
        if (id == 0) {
           std::sort(fcb->heapIndices.begin(), fcb->heapIndices.end(),
                [&](int i1, int i2) {
                    return fcb->heaps[i1].size() > fcb->heaps[i2].size();
                });
        }
        pthread_barrier_wait(&fcb->writeBarrier);
        while (true){
            int idx = fcb->idx.fetch_add(1);
            if (idx >= ALPHABET_SIZE) break;
            writeToFile(fcb->heapIndices[idx]);
        }
    }
private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
};  