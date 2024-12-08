#include "structs.h"
#include <sstream>
#include <chrono>
#include <iostream>
#include <vector>

using namespace std;

class Reducer : public AbstractThread {
public:
    Reducer(filesControlBlock *fcb, int id, int reducersCount)
        : fcb(fcb), id(id), reducersCount(reducersCount) {}

protected:
    void writeCharFile(int ch) {
        auto &heap = fcb->mergedHeaps[ch];
        /* Write the heap to its file */
        string filename = string(1, 'a' + ch) + ".txt";
        ofstream out(filename);
        if (!out.is_open()) {
            cerr << "Error in opening file " << filename << " during writing phase" << endl;
            return;
        }
        /* Format each entry */
        while(!heap.empty()) {
            entry e = heap.top();
            int arrSize = (int) e.ids.size();
            string line = e.word + ":[";
            for (int i = 0; i < arrSize; i++) {
                line += to_string(e.ids[i]);
                string delim = (i == arrSize - 1) ? "" : " ";
                line += delim;
            }
            line += "]\n";
            out << line;
            heap.pop();
        }
        out.close();
    }

    void mergeHeap(int ch) {
        /* Use a map for ~O(1) key(word) access */
        unordered_map<string, vector<int>> mergedEntries;
        int mappersCount = fcb->mappersCount;
        /* Construct the char-specific heap by iterating over each Mapper's partial entry sublist 
         * of that character */
        for (int mapperId = 0; mapperId < mappersCount; ++mapperId) {
            auto &mapperCharSublist = fcb->partialEntries[ch][mapperId];

            for(auto &e : mapperCharSublist) {
                total++;
                /* Merge the fileIDs arrays for the same word */
                mergedEntries[e.word].insert(mergedEntries[e.word].end(),
                                             e.ids.begin(), e.ids.end());
            }
        }
        for (auto &[word, ids] : mergedEntries) {
            sort(ids.begin(), ids.end());
            fcb->mergedHeaps[ch].push({word, ids});
        }
        /* Call the file-writing to disk procedure */
        writeCharFile(ch);
    }

    virtual void InternalThreadFunc() {
        pthread_barrier_wait(&fcb->reduceBarrier);

        int letter[ALPHABET_SIZE];
        vector<intmax_t> reducerLoads(reducersCount, 0);

        /* Assign letters to reducers based on frequencies using a greedy approach */
        for (int ch = 0; ch < ALPHABET_SIZE; ++ch) {
            /* Find the reducer with the minimum current load and assign it the current char */
            int minReducerId = 0;
            intmax_t minLoad = INT64_MAX;
            for (int i = 1; i < reducersCount; ++i) {
                if (reducerLoads[i] < minLoad) {
                    minReducerId = i;
                    minLoad = reducerLoads[i];
                }
            }
            letter[ch] = minReducerId;
            /* Combine the frequencies for each character obtained by each Mapper */
            for (int i = 0; i < fcb->mappersCount; ++i) {
                reducerLoads[minReducerId] += fcb->chFreq[i][ch];
            }
        }

        /* Build the heap and write it to file for each char assigned to this Reducer */
        for (int ch = 0; ch < ALPHABET_SIZE; ++ch) {
            if (letter[ch] == id) {
                mergeHeap(ch);
            }
        }

        cout << "Reducer " << id << " processed total of " << total << " entries" << endl;
    }

private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
    intmax_t total{0};
};
