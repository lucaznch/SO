#ifndef FILE_PROCESSING
#define FILE_PROCESSING


typedef struct {
    const char *file_path;      // File path of file to process
    int out_fd;                 // File descriptor of corresponding ".out" file
    int thread_id;              // 
    int max_threads;            //
    unsigned int delay;         //
} ThreadArgs;

/// Creates the correspoding ".out" file for the ".jobs" file. General function for exercise 1, 2 and 3.
/// @param file_path Path of file.
/// @return File descriptor of created file.
int create_out_file(const char *file_path);

/// Processes a specific ".jobs" file. General function for exercise 1 and 2.
/// @param file_path File path.
/// @param out_fd Field descriptor of the corresponding ".out" file.
/// @return 0 if the specifc ".jobs" file was processed successfully, 1 otherwise.
int process_job_file(const char *file_path, int out_fd);

/// Processes all files with the ".jobs" extension in the given directory. Specific for exercise 1
/// @param directory_path Directory path.
/// @param delay Delay.
/// @return 0 if the all files with the ".jobs" extension were processed successfully, 1 otherwise.
int file_processing(const char *directory_path, unsigned int delay);

/// Processes all files with the ".jobs" extension in the given directory, with multiprocessing.
/// Each file is processed in one process. Specific for exercise 2
/// @param directory_path Directory path.
/// @param max_proc Maximum number of child processes.
/// @param delay Delay.
/// @return 0 if the all files with the ".jobs" extension were processed successfully, 1 otherwise.
int file_processing_with_processes(const char *directory_path, int max_proc, unsigned int delay);

/// Reads the remainder of the line.
/// Useful when get_next() reads the start of the line but we don't call the parser for the given command, and we just read the remainder of the line.
/// @param fd File descriptor of the file to read.
void read_to_next_line(int fd);

/// Processes the ".jobs" file using a thread. This function is used by all threads of process to process the file.
/// @param arg struct containing all the thread arguments (thread_id, max_threads, out_fd, ...)
/// @return 
void *process_job_file_with_thread(void *arg);

/// Creates threads to process a ".jobs" file.
/// @param file_path File path.
/// @param out_fd File descriptor of the corresponding ".out" file.
/// @param max_threads Maximum threads to process the ".jobs" file.
/// @param delay Delay.
/// @return 0 if all threads were created successfully, 1 otherwise.
int file_processing_with_threads(const char *file_path, int out_fd, int max_threads, unsigned int delay);

/// Processes all files with the ".jobs" extension in the given directory, with multithreaded and multiprocessing. 
/// Each file is processed in a process using multiple threads. Specific for exercise 3
/// @param file_path Directory path.
/// @param max_proc Maximum number of child processes.
/// @param max_threads Maximum number of threads to process each ".jobs" file
/// @param delay Delay.
/// @return 0 if the all files with the ".jobs" extension were processed successfully, 1 otherwise.
int file_processing_with_processes_and_threads(const char *directory_path, int max_proc, int max_threads, unsigned int delay);

#endif // FILE_PROCESSING