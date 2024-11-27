#include "structs.h"
#include <sstream>
#include <chrono>
using namespace std;

class Reducer : public AbstractThread {
public:
    Reducer(filesControlBlock *fcb, int id, int reducersCount) : fcb(fcb), id(id), reducersCount(reducersCount) {}
protected:
    void heapify(int idx) {
        unordered_map<string, vector<int>> mergedEntries;

        for (int mapperId = 0; mapperId < (int)fcb->heaps[idx].size(); ++mapperId) {
            auto &localHeap = fcb->heaps[idx][mapperId];

            while (!localHeap.empty()) {
                entry e = localHeap.top();
                localHeap.pop();

                // Merge the IDs for the same word
                for (int &n : e.ids)
                    mergedEntries[e.word].push_back(n);
            }
        }
        for (auto &[word, ids] : mergedEntries) {
            total++;
            sort(ids.begin(), ids.end());
            fcb->heapsf[idx].push({word, ids});
        }
    }
    virtual void InternalThreadFunc() {
        pthread_barrier_wait(&fcb->reduceBarrier);
        while(true) {
            int idx = fcb->idx.fetch_add(1);
            if (idx >= ALPHABET_SIZE) break;
            heapify(idx);
        }
        cout << "Reducer " << id << " processed total of " << total << " entries" << endl;
        pthread_barrier_wait(&fcb->writeBarrier);
    }
private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
    intmax_t total{0};
};  