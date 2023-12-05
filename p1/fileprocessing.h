#ifndef FILE_PROCESSING
#define FILE_PROCESSING

/// Creates the correspoding ".out" file for the ".jobs" file.
/// @param file_path Path of file.
/// @return File descriptor of created file.
int create_out_file(const char *file_path);

/// Processes a specific ".jobs" file.
/// @param file_path File path.
/// @param out_fd Field descriptor of the corresponding ".out" file.
/// @return 0 if the specifc ".jobs" file was processed successfully, 1 otherwise.
int process_job_file(const char *file_path, int out_fd);

/// Processes all files with the ".jobs" extension in the given directory.
/// @param directory_path Directory name.
/// @param delay Delay.
/// @return 0 if the all files with the ".jobs" extension were processed successfully, 1 otherwise.
int file_processing(const char *directory_path, unsigned int delay);

/// Processes all files with the ".jobs" extension in the given directory, using multiple processes.
/// @param directory_path Directory name.
/// @param delay Delay.
/// @param max_proc Maximum number of child processes. Use 0 to process files without processes.
/// @return 0 if the all files with the ".jobs" extension were processed successfully, 1 otherwise.
int file_processing_with_processes(const char *directory_path, unsigned int delay, int max_proc);

#endif // FILE_PROCESSING