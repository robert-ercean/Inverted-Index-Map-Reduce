#include <iostream>
#include <fstream>
#include "structs.h"
#include <unistd.h>
#include "Mapper.h"
#include "Reducer.h"
#include <chrono>
#include <filesystem>

using namespace std;

void parse_filenames(string in_filename, filesControlBlock *fcb) {
    ifstream in("../checker/" + in_filename);
    if (!in.is_open()) {
        in.close();
        throw runtime_error("Err: Input File failed to open: " + in_filename);
    }

    string line;
    getline(in, line);
    int count = 1;
    while (getline(in, line)) {
        file f;
        f.filename = "../checker/" + line;
        filesystem::path p(f.filename);
        f.size = filesystem::file_size(p);
        f.id = count++;
        fcb->allFiles.push_back(f);
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

    /* Some memory pre-allocation to save time */
    fcb.chFreq.resize(ALPHABET_SIZE, 0);
    fcb.mergedHeaps.resize(ALPHABET_SIZE);
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        fcb.mergedHeaps[i] = priority_queue<entry, vector<entry>, decltype(&pqComparator)>(pqComparator);
    }
    fcb.partialEntries.resize(ALPHABET_SIZE);
    for (int ch = 0; ch < ALPHABET_SIZE; ++ch) {
        fcb.partialEntries[ch].resize(mappers_count);
    }

    try {
        parse_filenames(argv[3], &fcb);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    /* Sort the files in descending order by size*/
    sort(fcb.allFiles.begin(), fcb.allFiles.end(), [](const file& a, const file& b) {
        return a.size > b.size;
    });

    vector<vector<file>> mapperFiles(mappers_count);
    vector<intmax_t> mapperWorkloads(mappers_count, 0);

    /* Manage the work of each Mapper using a greedy load balancing logic */
    for (const file& f : fcb.allFiles) {
        /* Iterator pointing to the Mapper with the lowest current workload */
        auto minIter = min_element(mapperWorkloads.begin(), mapperWorkloads.end());
        /* Index of the Mapper with the lowest workload */
        int mapperIndex = distance(mapperWorkloads.begin(), minIter);

        /* Assign the curr file to that Mapper and updates the workloads counters */
        mapperFiles[mapperIndex].push_back(f);
        mapperWorkloads[mapperIndex] += f.size;
    }

    /* Construct the workers and the reduce phase barrier */
    pthread_barrier_init(&fcb.reduceBarrier, NULL, mappers_count + reducers_count);
    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers.emplace_back(Reducer(&fcb, i - mappers_count, reducers_count));
            continue;
        }
        mappers.emplace_back(Mapper(&fcb, i, reducers_count, move(mapperFiles[i])));
    }

    /* Start the Workers */
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

    /* Wait for the workers to stop */
    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers[i - mappers_count].WaitForInternalThreadToExit();
            continue;
        }
        mappers[i].WaitForInternalThreadToExit();
    }

    pthread_barrier_destroy(&fcb.reduceBarrier);

    return 0;
}