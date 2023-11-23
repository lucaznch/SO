#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_FILENAME_LENGHT 256





int process_job_file(const char *file_path) {
    char c;
    int fd = open(file_path, O_RDONLY);

    if (fd == -1)
        return 1;

    
    ssize_t bytes_read = read(fd, &c, sizeof(c));

    if (bytes_read == -1) {
        close(fd);
        return 1;
    }

    if (bytes_read > 0)
        write(STDOUT_FILENO, &c, sizeof(c));

    printf("\n");

    close(fd);
    return 0;
}




int file_processing(const char *directory_path) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL)
        return 1;
    
    while ((entry = readdir(dir)) != NULL) {
        
        if (strstr((*entry).d_name, ".jobs") != NULL) {
            char path[MAX_FILENAME_LENGHT + 1];
            snprintf(path, sizeof(path), "%s/%s", directory_path, entry->d_name); // concatenate dir + filename_in_dir

            if (process_job_file(path)) {
                fprintf(stderr, "Failed to process file: %s\n", path);
                closedir(dir);
                return 1; 
            }
        }
    }

    closedir(dir);

    return 0; 
}