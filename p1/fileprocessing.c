#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>


// int process_job_file(const char *filename);




int file_processing(const char *directory_path) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL)
        return 1;
    

    while ((entry = readdir(dir)) != NULL) {
        // siefenfisf
    }

    closedir(dir);
    return 0;  // Return 0 on success
}