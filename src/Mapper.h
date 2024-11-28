#include "structs.h"
#include <unordered_set>

using namespace std;

#include "structs.h"
#include <unordered_set>
#include <queue>
#include <cctype> // for tolower
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

class Mapper : public AbstractThread {
public:
    Mapper(filesControlBlock *fcb, int id, int reducersCount, vector<file> &&files)
        : fcb(fcb), id(id), reducersCount(reducersCount), files(move(files)) {
    }

protected:
    /* Helper function to clean up words, returns true unless the processed word is an empty string */
    bool strip_word(string &word) {
        for (int i = 0; i < (int) word.size(); i++) {
            char c = word[i];
            if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
                word.erase(i, 1);
                i--;
            }
            word[i] = tolower(word[i]);
        }
        return !word.empty();
    }

    // Internal thread function
    virtual void InternalThreadFunc() override {
        intmax_t max = 0;
        unordered_map<string, vector<int>> map;
        /* Process each file assigned to this Mapper */
        for (const file &f : files) {
            ifstream in(f.filename);
            if (!in.is_open()) {
                cerr << "Failed to open file in Mapper: " << id << endl;
                continue;
            }

            string word;
            while (in >> word) {
                max++;
                if (!strip_word(word)) continue;
                /* First place the entries in a map for quick indexing and checking if the
                 * file number associated with that word already exists in its id array */
                if (find(map[word].begin(), map[word].end(), f.id) == map[word].end())
                    map[word].push_back(f.id);
            }
        }
        /* After a Mapper finishes processing its files, it iterates over its local map and 
         * places the each entry in its corresponding place inside the partialEntries */
        for (auto &pair : map) {
            entry e;
            e.word = pair.first;
            e.ids = pair.second;
            char ch = pair.first[0];
            fcb->chFreq[ch - 'a']++;
            fcb->partialEntries[ch - 'a'][id].push_back(e);
        }
        cout << "Mapper " << id << " processed a total of " << max << " words" << endl;
        pthread_barrier_wait(&fcb->reduceBarrier);
    }

private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
    vector<file> files;
};