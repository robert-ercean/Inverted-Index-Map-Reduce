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
        heaps.resize(26); // 26 characters in the alphabet
        for (int i = 0; i < 26; ++i) {
            heaps[i] = priority_queue<entry, vector<entry>, decltype(&pqComparator)>(pqComparator);
        }
    }

protected:
    // Helper function to clean up words
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
        // Process each file assigned to this Mapper
        for (const file &f : files) {
            ifstream in(f.filename);
            if (!in.is_open()) {
                cerr << "Failed to open file in Mapper: " << id << endl;
                continue;
            }

            string word;
            while (in >> word) {
                if (!strip_word(word)) continue;
                max++;
                if (find(map[word].begin(), map[word].end(), f.id) == map[word].end())
                    map[word].push_back(f.id);
            }
        }
        for (auto &pair : map) {
            entry e;
            e.word = pair.first;
            e.ids = pair.second;
            char ch = pair.first[0];
            heaps[ch - 'a'].push(e);
        }
        // Move the local heaps to the shared structure
        for (int i = 0; i < 26; ++i) {
            fcb->heaps[i][id] = move(heaps[i]);
        }

        cout << "Mapper " << id << " processed a total of " << max << " words" << endl;
        pthread_barrier_wait(&fcb->reduceBarrier);
    }

private:
    filesControlBlock *fcb;
    int id;
    int reducersCount;
    vector<file> files;

    // One heap per character (26 heaps for 'a' to 'z')
    vector<priority_queue<entry, vector<entry>, decltype(&pqComparator)>> heaps;
};
