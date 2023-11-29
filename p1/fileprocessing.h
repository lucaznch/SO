#ifndef FILE_PROCESSING
#define FILE_PROCESSING


/// Processes all files with the ".jobs" extension in the given directory.
/// @param directory_path Directory name.
/// @return 0 if the all files with the ".jobs" extension were processed successfully, 1 otherwise.
int file_processing(const char *directory_path);


/// Processes a specific ".jobs" file.
/// @param file_path File path.
/// @param out_fd Field descriptor of the corresponding ".out" file.
/// @return 0 if the specifc ".jobs" file was processed successfully, 1 otherwise.
int process_job_file(const char *file_path, int out_fd);


/// Creates the correspoding ".out" file for the ".jobs" file.
/// @param file_path Path of file.
/// @return File descriptor of created file.
int create_out_file(const char *file_path);

#endif // FILE_PROCESSING