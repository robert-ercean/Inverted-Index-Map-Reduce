#include <iostream>
#include <fstream>
#include "AbstractThread.h"
#include "ThreadTypes.h"

using namespace std;

vector<string> parse_filenames(string in_filename) {
    vector<string> files;
    ifstream in("../checker/" + in_filename);
    if (!in.is_open()) {
        in.close();
        throw runtime_error("Err: Input File failed to open: " + in_filename);
    }
    string line;
    /* Ingore the first line since we don't need the file count */
    getline(in, line);
    while (getline(in, line)) {
        files.push_back(line);
    }
    in.close();
    return files;
}

int main(int argc, char **argv)
{
    int mappers_count = atoi(argv[1]);

    atomic<int> fileIdx(mappers_count);
    // pthread_barrier_t barrier;

    vector<Mapper> mappers;
    vector<string> files;
    try {
        files = parse_filenames(argv[3]);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    for (int i = 0; i < mappers_count; ++i) {
        mappers.emplace_back(Mapper(i, files, &fileIdx));
    }

    for (int i = 0; i < mappers_count; ++i) {
        if (!mappers[i].startInternalThread()) {
            cerr << "Starting Mapper Threads failed!" << endl;
        }
    }


    for (int i = 0; i < mappers_count; ++i) {
        mappers[i].WaitForInternalThreadToExit();
        // mappers[i].printRes();
    }

    return 0;
}