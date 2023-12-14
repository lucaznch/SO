#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

#include "parser.h"
#include "constants.h"
#include "operations.h"
#include "fileprocessing.h"



int create_out_file(const char *file_path) { // file_path is sure to have a file with the ".jobs" extension

    const char *lastDot = strrchr(file_path, '.'); // returns a pointer on the last occurrence of '.'
    size_t prefixLength = (size_t)(lastDot - file_path); // len of the prefix (excluding the ".jobs" extension)
    char *out_filename = malloc(prefixLength + strlen(".out") + 1); // allocate memory for the modified file name with ".out" extension

    strncpy(out_filename, file_path, prefixLength); // copy the prefix (excluding the ".jobs" extension)
    strcpy(out_filename + prefixLength, ".out"); // add the new ".out" extension

    int fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);   // open file to write only. if file does not exist it creates. if files exists it truncates.
                                                                                    // file has read and write permissions

    free(out_filename);

    return fd;
}



int process_job_file(const char *file_path, int out_fd) {
    int fd = open(file_path, O_RDONLY);

    if (fd == -1) {
        fprintf(stderr, "Failed to open .jobs file: %s\n", file_path);
        return 1;
    }

    while (1) { // this loop goes line by line, using get_next(), of the ".jobs" file
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
                if (ems_list_events(out_fd)) {
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
                close(fd);
                return 0;
        }
    }

    close(fd);
    return 0;
}



int file_processing(const char *directory_path, unsigned int delay) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", directory_path);
        return 1;
    }

    if (ems_init(delay)) { // initialize the EMS state that will be used by all ".jobs" files together (files are not related)
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }
    
    while ((entry = readdir(dir)) != NULL) { // go through all directory entries
        
        if (strstr((*entry).d_name, ".jobs") != NULL) { // if entry is a file with the ".jobs" extension
            char path[MAX_FILENAME_LENGHT + 1];
            int out_fd; // file descriptor of the corresponding ".out" file of the ".jobs" file

            snprintf(path, sizeof(path), "%s/%s", directory_path, (*entry).d_name); // concatenate directory_path with file_name obtaining file_path
            out_fd = create_out_file(path); // open the corresponding ".out"

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

    ems_terminate(); // terminate the EMS state that was used by all ".jobs" files

    closedir(dir);

    return 0; 
}



int file_processing_with_processes(const char *directory_path, int max_proc, unsigned int delay) {
    int processes_counter = 0; // counter to keep track of child processes
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", directory_path);
        return 1;
    }

    if (ems_init(delay)) { // initialize the EMS state that will be used by all ".jobs" files together (files are not related)
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }
    
    while ((entry = readdir(dir)) != NULL) { // go through all directory entries
        
        if (strstr((*entry).d_name, ".jobs") != NULL) { // if entry is a file with the ".jobs" extension
            char path[MAX_FILENAME_LENGHT + 1];
            int out_fd; // file descriptor of the corresponding ".out" file of the ".jobs" file
            pid_t pid;

            snprintf(path, sizeof(path), "%s/%s", directory_path, (*entry).d_name); // concatenate directory_path with file_name obtaining file_path
            out_fd = create_out_file(path); // open the corresponding ".out"

            if (out_fd == -1) {
                fprintf(stderr, "Failed to create out file: %s\n", path);
                close(out_fd);
                closedir(dir);
                return 1; 
            }

            /*
            fork a child process for each file (respecting the maximum number of processes) while the parent process continues to iterate over the remaing files
            both processes start executing after the fork() call
            the child processes will get a copy of the heap with the data of EMS, and will process a file and store new data on the heap
            */
            pid = fork();

            if (pid == -1) {
                fprintf(stderr, "Failed to fork\n");
                close(out_fd);
                closedir(dir);
                return 1;
            }
            else if (pid == 0) { // CHILD PROCESS CODE
                if (process_job_file(path, out_fd)) {
                    fprintf(stderr, "Failed to process file: %s\n", path);
                    close(out_fd);
                    closedir(dir);
                    ems_terminate();
                    exit(1); 
                }
                close(out_fd);
                ems_terminate();
                exit(0);
            }
            else { // PARENT PROCESS CODE
                processes_counter++; // the parent process created a new child process
                
                if (processes_counter >= max_proc) { // if reached the maximum number of child processes, the parent will wait for any child to finish
                    wait(NULL);
                    processes_counter--;
                }
            }
        }
    }

    // before terminating the parent process, we make sure to wait for any remaining child processes to finish
    while (processes_counter > 0) {
        wait(NULL);
        processes_counter--;
    }

    ems_terminate(); // terminate the EMS state that was used by all ".jobs" files

    closedir(dir);

    return 0;
}




pthread_mutex_t mutex;

pthread_cond_t condition;

int current_thread_id = 0;



void read_to_next_line(int fd) {
    char buf;
    ssize_t bytesRead;

    do {
        bytesRead = read(fd, &buf, 1);
    } while (bytesRead > 0 && buf != '\n');
}



void *process_job_file_with_thread(void *arg) {
    ThreadArgs *thread_args = (ThreadArgs *)arg;

    const char *file_path = (*thread_args).file_path;
    int out_fd = (*thread_args).out_fd;
    int thread_id = (*thread_args).thread_id;
    int max_threads = (*thread_args).max_threads;
    
    int fd = open(file_path, O_RDONLY);

    int line_number = 0;
    int command_type;

    if (fd == -1) {
        fprintf(stderr, "Failed to open .jobs file: %s\n", file_path);
        pthread_exit(NULL);
    }

    while (1) { // this loop goes line by line, using get_next(), of the ".jobs" file
        unsigned int event_id, delay, wait_thread_id;
        size_t num_rows, num_columns, num_coords;
        size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

        pthread_mutex_lock(&mutex);

        while (current_thread_id != thread_id) { // wait for this thread turn to execute
            printf("Thread %d waiting. Current thread_id: %d, Expected thread_id: %d\n", thread_id, current_thread_id, thread_id);
            pthread_cond_wait(&condition, &mutex);
        }

        command_type = get_next(fd);    // all threads read the command type of the current line
                                        // only one thread executes the command of the line, with one exception!
                                        // if the command type is EOC, then all threads read it and execute it

        printf("(thread %d), line %d, has: %d\n",thread_id, line_number, command_type);

        if (command_type == EOC) {
            current_thread_id = (current_thread_id + 1) % max_threads;
            pthread_cond_broadcast(&condition);
            pthread_mutex_unlock(&mutex);
            close(fd);
            printf("(1EOC)thread %d off\n", thread_id);
            pthread_exit(NULL);
        }

        /* 
        each thread only executes specific lines
        line is executed if (line_number % max_threads) == thread_id 
        example:
            10 lines (l0, l1, ..., l8 and l9) and 3 threads (t0, t1 and t2):
                thread0 executes line0
                thread1 executes line1
                thread2 executes line2
                thread0 executes line3
                thread1 executes line4
                ...
        */
        if (line_number % max_threads == thread_id) {
            printf("thread %d will execute line %d, with: %d\n", thread_id, line_number, command_type);

            if (command_type == CMD_CREATE) {
                if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
                    fprintf(stderr, "Create: Invalid command. See HELP for usage\n");
                    continue;
                }
                if (ems_create(event_id, num_rows, num_columns)) {
                    fprintf(stderr, "Failed to create event\n");
                }
            } 
            else if (command_type == CMD_RESERVE) {
                num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

                if (num_coords == 0) {
                    fprintf(stderr, "Reserve: Invalid command. See HELP for usage\n");
                    continue;
                }
                if (ems_reserve(event_id, num_coords, xs, ys)) {
                    fprintf(stderr, "Failed to reserve seats\n");
                }
            } 
            else if (command_type == CMD_SHOW) {
                if (parse_show(fd, &event_id) != 0) {
                    fprintf(stderr, "Show: Invalid command. See HELP for usage\n");
                    continue;
                }
                if (ems_show(out_fd, event_id)) {
                    fprintf(stderr, "Failed to show event\n");
                }
            } 
            else if (command_type == CMD_LIST_EVENTS) {
                if (ems_list_events(out_fd)) {
                    fprintf(stderr, "Failed to list events\n");
                }
            } 
            else if (command_type == CMD_WAIT) {
                int wait_result = parse_wait(fd, &delay, &wait_thread_id);

                if (wait_result == -1) {
                    fprintf(stderr, "Wait: Invalid command. See HELP for usage\n");
                    continue;
                }
                if (delay > 0) {
                    if (wait_result == 1 && (int)wait_thread_id == thread_id) { // WAIT arguments: delay and thread_id
                        printf("Waiting...\n");
                        ems_wait(delay);
                    }
                    else if (wait_result == 1 && (int)wait_thread_id != thread_id) {
                        //idk
                    }
                    else { // WAIT arguments: delay
                        printf("Waiting...\n");
                        ems_wait(delay);
                    }
                }
            }
            else if (command_type == CMD_INVALID) {
                fprintf(stderr, "Invalid: Invalid command. See HELP for usage\n");
            } 
            else if (command_type == CMD_HELP) {
                printf(
                    "Available commands:\n"
                    "  CREATE <event_id> <num_rows> <num_columns>\n"
                    "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                    "  SHOW <event_id>\n"
                    "  LIST\n"
                    "  WAIT <delay_ms> [thread_id]\n"
                    "  BARRIER\n"
                    "  HELP\n");
            } 
            else if (command_type == CMD_BARRIER) {
                // we don't need to implement this command, because there is order in executing each line
                // the program already enforces a form of barrier due to the sequential nature of command execution by individual threads
                // whenever a line gets executed, no line in front has been executed
            }
            else if (command_type == CMD_EMPTY) {
                // do nothing
            }

            current_thread_id = (current_thread_id + 1) % max_threads;
            printf("current thread id: %d\n", current_thread_id);
            pthread_cond_broadcast(&condition);
        
        }
        else {
            // if thread does not execute the command of the line, then we read the remainder of line because get_next() only reads the command and not the whole line
            // with the exception of LIST and BARRIER commands, that have no args so the line was already read
            if (command_type != CMD_LIST_EVENTS && command_type != CMD_BARRIER && command_type != CMD_EMPTY)
                read_to_next_line(fd);
        }

        line_number++;

        pthread_mutex_unlock(&mutex);
        
    }
}



int file_processing_with_threads(const char *file_path, int out_fd, int max_threads, unsigned int delay) {
    int i;
    pthread_t threads[max_threads];
    ThreadArgs thread_args[max_threads];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);

    for (i = 0; i < max_threads; i++) {
        thread_args[i].file_path = file_path;
        thread_args[i].out_fd = out_fd;
        thread_args[i].max_threads = max_threads;
        thread_args[i].delay = delay;
        thread_args[i].thread_id = i;

        printf("created thread %d for %s file\n", i, file_path);
        pthread_create(&threads[i], NULL, process_job_file_with_thread, (void *)&thread_args[i]);
    }

    for (i = 0; i < max_threads; i++)
        pthread_join(threads[i], NULL);

    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);

    return 0;
}



int file_processing_with_processes_and_threads(const char *directory_path, int max_proc, int max_threads, unsigned int delay) {
    int processes_counter = 0; // counter to keep track of child processes
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", directory_path);
        return 1;
    }

    if (ems_init(delay)) { // initialize the EMS state that will be used by all ".jobs" files together (files are not related)
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }

    //pthread_mutex_init(&mutex, NULL);
    //pthread_cond_init(&condition, NULL);
    
    while ((entry = readdir(dir)) != NULL) { // go through all directory entries
        
        if (strstr((*entry).d_name, ".jobs") != NULL) { // if entry is a file with the ".jobs" extension
            char path[MAX_FILENAME_LENGHT + 1];
            int out_fd; // file descriptor of the corresponding ".out" file of the ".jobs" file
            pid_t pid;

            snprintf(path, sizeof(path), "%s/%s", directory_path, (*entry).d_name); // concatenate directory_path with file_name obtaining file_path
            out_fd = create_out_file(path); // open the corresponding ".out"

            if (out_fd == -1) {
                fprintf(stderr, "Failed to create out file: %s\n", path);
                close(out_fd);
                closedir(dir);
                return 1; 
            }

            /*
            fork a child process for each file (respecting the maximum number of processes) while the parent process continues to iterate over the remaing files
            both processes start executing after the fork() call
            the child processes will get a copy of the heap with the data of EMS, and will process a file and store new data on the heap
            */
            pid = fork();

            if (pid == -1) {
                fprintf(stderr, "Failed to fork\n");
                close(out_fd);
                closedir(dir);
                return 1;
            }
            else if (pid == 0) { // CHILD PROCESS CODE
                if (file_processing_with_threads(path, out_fd, max_threads, delay)) {
                    fprintf(stderr, "Failed to process file: %s\n", path);
                    close(out_fd);
                    closedir(dir);
                    ems_terminate();
                    exit(1); 
                }
                close(out_fd);
                ems_terminate();
                exit(0);
            }
            else { // PARENT PROCESS CODE
                processes_counter++; // the parent process created a new child process
                
                if (processes_counter >= max_proc) { // if reached the maximum number of child processes, the parent will wait for any child to finish
                    wait(NULL);
                    processes_counter--;
                }
            }
        }
    }

    // before terminating the parent process, we make sure to wait for any remaining child processes to finish
    while (processes_counter > 0) {
        wait(NULL);
        processes_counter--;
    }

    ems_terminate(); // terminate the EMS state that was used by all ".jobs" files

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition);

    closedir(dir);

    return 0;
}