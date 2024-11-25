#include <iostream>
#include <fstream>
#include "structs.h"
#include <unistd.h>
#include "Mapper.h"
#include "Reducer.h"
#include <filesystem>

using namespace std;

void parse_filenames(string in_filename, filesControlBlock *fcb) {
    ifstream in("../checker/" + in_filename);
    if (!in.is_open()) {
        in.close();
        throw runtime_error("Err: Input File failed to open: " + in_filename);
    }

    string line;
    /* Ingore the first line since we don't need the file count */
    getline(in, line);
    int count = 1;
    while (getline(in, line)) {
        file f;
        f.filename = "../checker/" + line;
        filesystem::path p(f.filename);
        f.size = filesystem::file_size(p);
        f.id = count++;
        fcb->filesPq.push(f);
    }
    in.close();
}

int main(int argc, char **argv)
{
    int mappers_count = atoi(argv[1]);
    int reducers_count = atoi(argv[2]);

    vector<Mapper> mappers;
    vector<Reducer> reducers;

    filesControlBlock fcb;
    fcb.idx.store(0);
    fcb.aggregateLists.resize(ALPHABET_SIZE);
    for (int c = 0; c < ALPHABET_SIZE; ++c) {
        fcb.aggregateLists[c].resize(mappers_count);
    }
    try {
        parse_filenames(argv[3], &fcb);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    pthread_mutex_init(&fcb.filesMutex, NULL);
    pthread_barrier_init(&fcb.reduceBarrier, NULL, mappers_count + reducers_count);

    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers.emplace_back(Reducer(&fcb, i - mappers_count, reducers_count));
            continue;
        }
        mappers.emplace_back(Mapper(&fcb, i));
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
    pthread_mutex_destroy(&fcb.filesMutex);
    pthread_barrier_destroy(&fcb.reduceBarrier);

    return 0;
}