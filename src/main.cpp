#include <iostream>
#include "structs.hpp"

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

void *mapper_func(void *arg) {
    mappers_arg *mapp_arg = (mappers_arg *)arg;
    InFile *in_file = mapp_arg->in_file;
    atomic<int> *curr_file_idx = &in_file->file_idx;
    int id = mapp_arg->id;
    int idx = id;
    indexed_file file_to_process;

    /* Process the statically assigned file id first */
    if (idx >= in_file->file_count) {
        cerr << "Static IDX exceeds maximum file number";
        return NULL;
    }
    file_to_process = in_file->files[idx];
    cout << "Mapper " << mapp_arg->id << " is [STATICALLY] processing file: " << file_to_process.filename << endl;

    ifstream input("../checker/" + file_to_process.filename);
    if (!input.is_open()) {
        cerr << "[STATIC]Mapper failed to open the file: " << file_to_process.filename << endl;
        return NULL;
    }
    string word;
    while (input >> word) {
        strip_word(word);
        vector<int> &arr = mapp_arg->res[word];
        int id = file_to_process.id;
        if (find(arr.begin(), arr.end(), id) == arr.end()) {
            arr.push_back(id);
        }
    }
    input.close();

    /* If there are anymore files left to 
     * process, fetch their atomic id and start parsing */
    while (true) {
        idx = curr_file_idx->fetch_add(1);
        if (idx >= in_file->file_count)
            break;
        file_to_process = in_file->files[idx];
        cout << "Mapper " << mapp_arg->id << " is [DINAMICALLY] processing file: " << file_to_process.filename << endl;
        
        ifstream input("../checker/" + file_to_process.filename);
        if (!input.is_open()) {
            cerr << "[DINAMIC]Mapper failed to open the file: " << file_to_process.filename << endl;
            continue;
        }
        string word;
        while (input >> word) {
            strip_word(word);
            vector<int> &arr = mapp_arg->res[word];
            int id = file_to_process.id;
            if (find(arr.begin(), arr.end(), id) == arr.end()) {
                arr.push_back(id);
            }
        }
        input.close();
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int mappers_count = atoi(argv[1]);

    InFile in_file(argv[3]);
    try {
        in_file.parse_filenames();
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    pthread_t mappers[mappers_count];

    mappers_arg mapp_args[mappers_count];

    for (int i = 0; i < mappers_count; ++i) {
        mapp_args[i].id = i;
        mapp_args[i].in_file = &in_file;
        pthread_create(&mappers[i], NULL, mapper_func, (void *)&mapp_args[i]);
    }

    for (int i = 0; i < mappers_count; ++i) {
        pthread_join(mappers[i], NULL);
    }

    for (int i = 0; i < mappers_count; ++i) {
        cout << "Mapper " << i << ":" << endl;
        for (auto pair : mapp_args[i].res) {
            cout << pair.first << " : ";
            for (int id : pair.second) {
                cout << id << " ";
            }
            cout << endl;
        }
        cout << endl << "-------------------------------" << endl;
    }

    return 0;
}