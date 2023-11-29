#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "parser.h"
#include "constants.h"
#include "operations.h"
#include <sys/stat.h>


int create_out_file(const char *file_path) { // file_path is sure to have a file with the ".jobs" extension

    const char *lastDot = strrchr(file_path, '.'); // returns a pointer on the last occurrence of '.'
    size_t prefixLength = (size_t)(lastDot - file_path); // len of the prefix (excluding the ".jobs" extension)
    char *out_filename = malloc(prefixLength + strlen(".out") + 1); // allocate memory for the modified file name with ".out" extension

    strncpy(out_filename, file_path, prefixLength); // copy the prefix (excluding the ".jobs" extension)
    strcpy(out_filename + prefixLength, ".out"); // add the new ".out" extension

    int fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    free(out_filename);

    return fd;
}



int process_job_file(const char *file_path, int out_fd) {
    int fd;
    
    fd = open(file_path, O_RDONLY);

    if (fd == -1) {
        fprintf(stderr, "Failed to open .jobs file: %s\n", file_path);
        return 1;
    }

    if (ems_init(STATE_ACCESS_DELAY_MS)) {
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }

    while (1) {
        unsigned int event_id, delay;
        size_t num_rows, num_columns, num_coords;
        size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

        switch (get_next(fd)) {
            case CMD_CREATE:
                if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    continue;
                }
                if (ems_create(event_id, num_rows, num_columns)) {
                    fprintf(stderr, "Failed to create event\n");
                }
                break;

            case CMD_RESERVE:
                num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

                if (num_coords == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    continue;
                }
                if (ems_reserve(event_id, num_coords, xs, ys)) {
                    fprintf(stderr, "Failed to reserve seats\n");
                }
                break;

            case CMD_SHOW:
                if (parse_show(fd, &event_id) != 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    continue;
                }
                if (ems_show(out_fd, event_id)) {
                    fprintf(stderr, "Failed to show event\n");
                }
                break;

            case CMD_LIST_EVENTS:
                if (ems_list_events()) {
                    fprintf(stderr, "Failed to list events\n");
                }
                break;
            
            case CMD_WAIT:
                if (parse_wait(fd, &delay, NULL) == -1) {  // thread_id is not implemented
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    continue;
                }
                if (delay > 0) {
                    printf("Waiting...\n");
                    ems_wait(delay);
                }
                break;

            case CMD_INVALID:
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                break;

            case CMD_HELP:
                printf(
                    "Available commands:\n"
                    "  CREATE <event_id> <num_rows> <num_columns>\n"
                    "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                    "  SHOW <event_id>\n"
                    "  LIST\n"
                    "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
                    "  BARRIER\n"                      // Not implemented
                    "  HELP\n");
                break;

            case CMD_BARRIER:  // Not implemented
            
            case CMD_EMPTY:
                break;

            case EOC:
                ems_terminate();
                return 0;
        }
    }

    close(fd);
    return 0;
}



int file_processing(const char *directory_path) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", directory_path);
        return 1;
    }
    
    while ((entry = readdir(dir)) != NULL) { // go through all directory entries
        
        if (strstr((*entry).d_name, ".jobs") != NULL) { // if entry is a file with the ".jobs" extension
            char path[MAX_FILENAME_LENGHT + 1];
            int out_fd;

            snprintf(path, sizeof(path), "%s/%s", directory_path, (*entry).d_name); // concatenate directory_path with file_name obtaining file_path
            out_fd = create_out_file(path); // open the corresponding ".out" file to write on

            if (out_fd == -1) {
                fprintf(stderr, "Failed to create out file: %s\n", path);
                close(out_fd);
                closedir(dir);
                return 1; 
            }

            if (process_job_file(path, out_fd)) {
                fprintf(stderr, "Failed to process file: %s\n", path);
                close(out_fd);
                closedir(dir);
                return 1; 
            }

            close(out_fd);
        }
    }

    closedir(dir);

    return 0; 
}