#include "structs.h"
#include <sstream>

using namespace std;

class Reducer : public AbstractThread {
public:
    Reducer(filesControlBlock *fcb, int id, int reducersCount) : fcb(fcb), id(id), reducersCount(reducersCount) {}
protected:
    priority_queue<pair<string, vector<int>>, vector<pair<string, vector<int>>>, decltype(&pqComparator)>
    sortCharSpecificArray(vector<vector<partial_entry>> &chArr) {
        unordered_map<string, vector<int>> wordIdxList;

        for (const auto &vec : chArr) {
            for (const auto &entry : vec) {
                wordIdxList[entry.word].push_back(entry.id);
            }
        }

        vector<pair<string, vector<int>>> result;
        priority_queue<pair<string, vector<int>>, vector<pair<string, vector<int>>>, decltype(&pqComparator)> pq(pqComparator);
        for (auto &pair : wordIdxList) {
            sort(pair.second.begin(), pair.second.end());
            pq.push(move(pair));
        }

        return pq;
    }

    void writeToFile(priority_queue<pair<string, vector<int>>, vector<pair<string, vector<int>>>, decltype(&pqComparator)> &pq, int c) {
        ostringstream buffer;
        while (!pq.empty()) {
            auto &pair = pq.top();
            buffer << pair.first << ":[";
            for (size_t i = 0; i < pair.second.size(); i++) {
                buffer << pair.second[i];
                if (i < pair.second.size() - 1) {
                    buffer << " ";
                }
            }
            buffer << "]\n";
            pq.pop();
        }
        ofstream out(string(1, c + 'a') + ".txt");
        out << buffer.str();
    }

    virtual void InternalThreadFunc() {
        pthread_barrier_wait(&fcb->reduceBarrier);
        int chunkSize = ALPHABET_SIZE / reducersCount;
        int start = id * chunkSize;
        int end = (id == (reducersCount - 1)) ? ALPHABET_SIZE: (id + 1) * chunkSize;
        for (int c = start; c < end; c++) {
            auto out = sortCharSpecificArray(fcb->aggregateLists[c]);
            writeToFile(out, c);
        }
    }
private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
};  