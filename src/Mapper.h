#include "structs.h"
#include <unordered_set>

using namespace std;

class Mapper : public AbstractThread {
public:
    Mapper(filesControlBlock *fcb, int id, int reducersCount) : fcb(fcb), id(id), reducersCount(reducersCount) {}

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

    virtual void InternalThreadFunc() override {
        /* Get file to process */
        vector<vector<partial_entry>> tmp(reducersCount);
        while (true) {
            unordered_set<string> seen;
            pthread_mutex_lock(&fcb->filesMutex);
            if (fcb->filesPq.empty()) {
                pthread_mutex_unlock(&fcb->filesMutex);
                break;
            }
            file f = fcb->filesPq.top();
            string filename = f.filename;
            cout << "Mapper " << id << " processing " << filename << endl;
            int id = f.id;
            fcb->filesPq.pop();
            pthread_mutex_unlock(&fcb->filesMutex);

            string word;
            ifstream in(filename);
            if (!in.is_open()) {
                cerr << "Failed to open file in Mapper: " << id << endl;
                return;
            }
            while (in >> word) {
                strip_word(word);
                char ch = word[0];
                if (ch < 'a' || ch > 'z') continue;
                /* Only add the word if it hasn't been already added */
                if (seen.insert(word).second) {
                    partial_entry e;
                    e.word = word;
                    e.id = f.id;
                    int reducerId = hash<string>{}(word) % reducersCount;
                    tmp[reducerId].push_back(e);
                }
            }
        }
        for (int r = 0; r < reducersCount; ++r) {
            fcb->aggregateLists[r][id] = move(tmp[r]);
        }
        pthread_barrier_wait(&fcb->reduceBarrier);
    }
private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
};