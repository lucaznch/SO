#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "fileprocessing.h"


int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 1) {
    char *endptr;

    // converts argv[1] to an unsigned long int corresponding to the possible delay value
    // strtoul() returns the first char in the input string that is not valid of conversion or returns '\0' if the whole input string was converted successfully
    unsigned long int delay = strtoul(argv[1], &endptr, 10); 

    if (*endptr != '\0') { // fail conversion, so argv[1] is the "jobs" directory
      
      if (argc == 5 || argc == 4) {
        if (argc == 5) {
          delay = strtoul(argv[4], &endptr, 10);

          if (delay > UINT_MAX) {
            fprintf(stderr, "Invalid delay value or value too large\n");
            return 1; 
          }

          state_access_delay_ms = (unsigned int)delay;
        }
        
        if (file_processing_with_processes_and_threads(argv[1], atoi(argv[2]), atoi(argv[3]), state_access_delay_ms)) {
          fprintf(stderr, "Failed to process files\n");
          return 1;
        }
        return 0;
      }

      else if (argc == 3) {
        if (file_processing_with_processes(argv[1], atoi(argv[2]), state_access_delay_ms)) {
          fprintf(stderr, "Failed to process file\n");
          return 1;
        }
        return 0;
      }

      else if (argc == 2) {
        if (file_processing(argv[1], state_access_delay_ms)) {
          fprintf(stderr, "Failed to process file\n");
          return 1;
        }
        return 0;
      }
    }
  }

  fprintf(stderr, "Usage: %s <directory> <MAX_PROC> <MAX_THREADS> [delay]\n", argv[0]);
  fprintf(stderr, "<directory>: Path to the 'jobs' directory\n<MAX_PROC>:  Maximum number of child processes\n<MAX_THREADS>: Maximum number of threads\n[delay]: delay (optional)\n");
  return 1;
}