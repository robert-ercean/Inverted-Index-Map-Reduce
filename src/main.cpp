#include <iostream>
#include <fstream>
#include "AbstractThread.h"
#include "ThreadTypes.h"
#include <unistd.h>

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
    int reducers_count = atoi(argv[2]);

    vector<Mapper> mappers;
    vector<Reducer> reducers;

    filesControlBlock fcb;
    pthread_mutex_init(&fcb.aggregateListMutex, NULL);
    pthread_mutex_init(&fcb.partialListsMutex, NULL);
    pthread_barrier_init(&fcb.barrier, NULL, mappers_count + reducers_count);
    fcb.fileIdx.store(mappers_count);
    fcb.partialListsIdx.store(reducers_count);

    try {
        fcb.files = parse_filenames(argv[3]);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers.emplace_back(Reducer(&fcb, i - mappers_count));
            continue;
        }
        mappers.emplace_back(Mapper(i, &fcb));
    }

    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            if (!reducers[i - mappers_count].startInternalThread()) {
                cerr << "Starting Reducer Threads failed!" << endl;
                return -1;
            }
            continue;    
        }
        if (!mappers[i].startInternalThread()) {
            cerr << "Starting Mapper Threads failed!" << endl;
            return -1;
        }
    }

    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers[i - mappers_count].WaitForInternalThreadToExit();
            continue;
        }
        mappers[i].WaitForInternalThreadToExit();
    }

    // for (auto &list : fcb.partialLists) {
    //     for (auto &pair : list) {
    //         cout << pair.first << ": ";
    //         for (int &id : pair.second) {
    //             cout << id << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << "------------------------------" << endl;
    // }

    pthread_barrier_destroy(&fcb.barrier);
    pthread_mutex_destroy(&fcb.partialListsMutex);
    // usleep(1000);
    // cout << endl << endl;
    // for (auto &pair : fcb.aggregateList) {
    //     cout << pair.first << ": ";
    //     for (auto &id : pair.second) {
    //         cout << id << " ";
    //     }
    //     cout << endl;
    // }
    return 0;
}