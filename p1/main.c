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

      if (argc == 3) { // ./ems jobs/ MAX_PROC
        // implement ex2 logic here
      }
      else if (argc == 2) { // ./ems jobs/
        if (file_processing(argv[1], state_access_delay_ms)) {
          fprintf(stderr, "Failed to process file\n");
          return 1;
        }
      }
    }
    else if (delay > UINT_MAX) { // successful conversion but returns invalid delay
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1; 
    }
    else { // successful conversion
      state_access_delay_ms = (unsigned int)delay;
    }
  }

  return 0;
}