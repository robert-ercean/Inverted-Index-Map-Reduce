#include <string>
#include <pthread.h>
#include <vector>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <atomic>

using namespace std;

typedef struct {
    string filename;
    int id;
} indexed_file;

class InFile {
    public:
        InFile(string filename, int next_file_idx) : in_filename(filename), next_file_idx(next_file_idx) {}
        
        void parse_filenames() {
            ifstream in("../checker/" + in_filename);
            if (!in.is_open()) {
                in.close();
                throw runtime_error("Err: Input File failed to open: " + in_filename);
            }
            int count = 1;
            string line;
            if (getline(in, line)) {
                file_count = stoi(line);
            } else {
                in.close();
                throw runtime_error("Err: Failed to read first line of in_file");
            }
            while (getline(in, line)) {
                indexed_file tmp;
                tmp.filename = line;
                tmp.id = count++;
                files.push_back(tmp);
            }
            in.close();
        }

        string in_filename;
        int file_count;
        vector<indexed_file> files;
        atomic<int>next_file_idx;
};

typedef struct {
    InFile *in_file;
    int id;
    unordered_map<string, vector<int>> res;
} mappers_arg;
