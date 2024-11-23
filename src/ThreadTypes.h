#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include "AbstractThread.h"

using namespace std;

class Mapper : public AbstractThread {
public:
    Mapper(int id, vector<string> files, atomic<int> *idx) : id(id), files(files), fileIdx(idx) {}
    unordered_map<string, vector<int>> res;
    /* Debugging purposes */
    void printRes() {
        cout << "-----------------------------------" << endl;
        cout << "Mapper " << id << endl;
        for (auto &pair : res) {
            cout << pair.first << ": ";
            for (int &id : pair.second) {
                cout << id << " ";
            }
            cout << endl;
        }
    }

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
        cout << "Mapper " << id << " is processing file: " << files[idx] << endl;

        ifstream input("../checker/" + files[idx]);
        if (!input.is_open()) {
            return 0;
        }
   
        string word;
        while (input >> word) {
            strip_word(word);
            vector<int> &arr = res[word];
            int fileId = idx + 1;
            if (find(arr.begin(), arr.end(), fileId) == arr.end()) {
                arr.push_back(fileId);
            }
        }
   
        input.close();
        return 1;
    }

    virtual void InternalThreadFunc() override {
        int static_idx = id;

        /* Process the statically assigned file id first */
        if (static_idx >= (int) files.size()) {
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
            int dynamic_idx = fileIdx->fetch_add(1);
            
            if (dynamic_idx >= (int) files.size())
                break;
            
            if (!processFile(dynamic_idx)) {
                cerr << "[DYNAMIC]Mapper failed to open the file with id: " << dynamic_idx << endl;
                return;
            }
        }
    }
private:
    int id;
    vector<string> files;
    atomic<int> *fileIdx;
};
