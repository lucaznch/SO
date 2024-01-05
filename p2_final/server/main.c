#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "utils.h"

#define PIPE_PATH_MAX 40
#define NUMBER_OF_SESSIONS 2

typedef struct {
    session_info** sessions;
    int* prod_ptr;
    int* cons_ptr;
    int* count;
    pthread_mutex_t* mutex;
    pthread_cond_t* canProd;
    pthread_cond_t* canCons;
} common_arg;

typedef struct {
  int session_id;
  common_arg *common_args;
} individual_session_args;



volatile sig_atomic_t sigusr1_received = 0; // global variable used in the main thread, to indicate the signal status. It's the SIGUSR1 flag of the main thread
pthread_mutex_t sigusr1_mutex = PTHREAD_MUTEX_INITIALIZER;


void sigusr1_handler(int s) { // handler routine of the SIGUSR1 signal
  (void)s; // unused parameter
  sigusr1_received = 1; // change the status to confirm there was a signal, and let the main thread deal with it outside this routine
  signal(SIGUSR1, sigusr1_handler); // re-associate the SIGUSR1 signal to this routine
}




void* individual_session(void* args) {
  int session_id = ((individual_session_args*)args)->session_id;
  common_arg* common_args = ((individual_session_args*)args)->common_args;
  session_info* current_session = NULL;

  sigset_t mask; // represents a set of signals to block in this thread
  sigemptyset(&mask); // initialize the set to an empty set
  sigaddset(&mask, SIGUSR1); // add the SIGUSR1 signal to the set
  pthread_sigmask(SIG_BLOCK, &mask, NULL); // block all the signals of the set, in this thread. Only SIGUSR1 will be blocked, since it's the only signal in the set



  while (1) {
    pthread_mutex_lock(common_args->mutex); // locks the mutex
    while (*(common_args->count) == 0) { // if the buffer is empty
      pthread_cond_wait(common_args->canCons, common_args->mutex); // waits until a slot is available for consumption
    }
    current_session = common_args->sessions[*(common_args->cons_ptr)]; // gets the session from the buffer
    *(common_args->cons_ptr) = *(common_args->cons_ptr) + 1;
    if (*(common_args->cons_ptr) == NUMBER_OF_SESSIONS) { // if the index of the next slot to be consumed is NUMBER_OF_SESSIONS, then it wraps around
      *(common_args->cons_ptr) = 0;
    }
    (*(common_args->count)) = (*(common_args->count)) - 1;
    pthread_cond_signal(common_args->canProd); // signals that a slot is available for production
    pthread_mutex_unlock(common_args->mutex); // unlocks the mutex

    if (current_session == NULL) { // if the session is NULL, then the buffer is empty
      printf("Buffer is empty\n"); // just for debugging purposes
      continue;
    }

    int req_fd = open(current_session->req_pipe_path, O_RDONLY); // opens the request pipe
    int resp_fd = open(current_session->resp_pipe_path, O_WRONLY); // opens the response pipe

    if (req_fd == -1 || resp_fd == -1) { // if the request pipe or the response pipe could not be opened, then they do not exist
      fprintf(stderr, "Failed to open pipe\n");
      return NULL;
    }

    write(resp_fd, &session_id, sizeof(int)); // writes the session id to the response pipe

    int to_continue = 1;
    while (to_continue) {
      printf("Session %d waiting for operation\n", session_id); // just for debugging purposes
      char op_code; // to store the type of operation
      ssize_t bytes_read; // to store the number of bytes read
      
      bytes_read = read(req_fd, &op_code, sizeof(op_code)); // reads the type of operation

      if (bytes_read != 1) { // if the number of bytes read is not 1, then the pipe is empty
        fprintf(stderr, "Failed to read from pipe\n");
        int return_code = 1;
        write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
        return NULL;
      }

      switch (op_code) {
        case '2': { // to quit
          printf("Session %d quitting\n", session_id); // just for debugging purposes
          to_continue = 0;
          printf("Session %d ended\n", session_id); // just for debugging purposes
          break;
        }

        case '3': { // to create
          printf("Session %d creating event\n", session_id); // just for debugging purposes
          unsigned int event_id;
          size_t num_rows;
          size_t num_cols;
          
          bytes_read = read(req_fd, &event_id, sizeof(event_id)); // reads the event id

          if (bytes_read != sizeof(event_id)) { // if the number of bytes read is not sizeof(event_id), then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          bytes_read = read(req_fd, &num_rows, sizeof(num_rows)); // reads the number of rows

          if (bytes_read != sizeof(num_rows)) { // if the number of bytes read is not sizeof(num_rows), then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          bytes_read = read(req_fd, &num_cols, sizeof(num_cols)); // reads the number of columns

          if (bytes_read != sizeof(num_cols)) { // if the number of bytes read is not sizeof(num_cols), then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          if (ems_create(event_id, num_rows, num_cols) != 0) { // creates the event
            fprintf(stderr, "Failed to create event\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          int return_code = 0;
          write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          break;
        }

        case '4': { // to reserve
          printf("Session %d reserving seats\n", session_id); // just for debugging purposes
          unsigned int event_id;
          size_t num_seats;
          size_t* xs;
          size_t* ys;

          bytes_read = read(req_fd, &event_id, sizeof(event_id)); // reads the event id

          if (bytes_read != sizeof(event_id)) { // if the number of bytes read is not sizeof(event_id), then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          bytes_read = read(req_fd, &num_seats, sizeof(num_seats)); // reads the number of seats

          if (bytes_read != sizeof(num_seats)) { // if the number of bytes read is not sizeof(num_seats), then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          xs = malloc(sizeof(size_t) * num_seats); // allocates memory for the x coordinates
          ys = malloc(sizeof(size_t) * num_seats); // allocates memory for the y coordinates

          bytes_read = read(req_fd, xs, sizeof(size_t) * num_seats); // reads the x coordinates

          if (bytes_read != (ssize_t)(sizeof(size_t) * num_seats)) { // if the number of bytes read is not sizeof(size_t) * num_seats, then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          bytes_read = read(req_fd, ys, sizeof(size_t) * num_seats); // reads the y coordinates

          if (bytes_read != (ssize_t)(sizeof(size_t) * num_seats)) { // if the number of bytes read is not sizeof(size_t) * num_seats, then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          if (ems_reserve(event_id, num_seats, xs, ys) != 0) { // reserves the seats
            fprintf(stderr, "Failed to reserve seats\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          free(xs); // frees the memory allocated for the x coordinates
          free(ys); // frees the memory allocated for the y coordinates

          int return_code = 0;
          write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          break;
        }

        case '5': { // to show
          printf("Session %d showing event\n", session_id); // just for debugging purposes
          unsigned int event_id;

          bytes_read = read(req_fd, &event_id, sizeof(event_id)); // reads the event id

          if (bytes_read != sizeof(event_id)) { // if the number of bytes read is not sizeof(event_id), then the pipe is empty
            fprintf(stderr, "Failed to read from pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          show_data data = ems_show(event_id); // shows the event

          if (data.return_code != 0) { // shows the event
            fprintf(stderr, "Failed to show event\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
            return NULL;
          }

          int return_code = 0;
          write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          write(resp_fd, &data.num_rows, sizeof(data.num_rows)); // writes the number of rows to the response pipe
          write(resp_fd, &data.num_cols, sizeof(data.num_cols)); // writes the number of columns to the response pipe
          write(resp_fd, data.seats, sizeof(unsigned int) * data.num_rows * data.num_cols); // writes a vector with the seats to the response pipe

          break;
        }

        case '6': {
          printf("Session %d listing events\n", session_id); // just for debugging purposes
          list_data data = ems_list_events(); // lists the events

          if (data.return_code != 0) { // if the events could not be listed
            fprintf(stderr, "Failed to list events\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          }

          write(resp_fd, &data.return_code, sizeof(int)); // writes the response to the response pipe

          if (data.num_events == 0) { // if there are no events
            write(resp_fd, &data.num_events, sizeof(data.num_events)); // writes the number of events to the response pipe
            break;
          }

          ssize_t bytes_written = write(resp_fd, &data.num_events, sizeof(data.num_events)); // writes the number of events to the response pipe

          if (bytes_written != sizeof(data.num_events)) { // if the number of bytes written is not sizeof(data.num_events), then the pipe is full
            fprintf(stderr, "Failed to write to pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          }

          bytes_written = write(resp_fd, data.events, sizeof(unsigned int) * data.num_events); // writes a vector with the events to the response pipe

          if (bytes_written != (ssize_t)(sizeof(unsigned int) * data.num_events)) { // if the number of bytes written is not sizeof(unsigned int) * data.num_events, then the pipe is full
            fprintf(stderr, "Failed to write to pipe\n");
            int return_code = 1;
            write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          }

          break;
        }

        default: {
          printf("Session %d invalid operation\n", session_id); // just for debugging purposes
          int return_code = 1;
          write(resp_fd, &return_code, sizeof(int)); // writes the response to the response pipe
          return NULL;
        }
      }
    }
    close(req_fd); // closes the request pipe
    close(resp_fd); // closes the response pipe

    unlink(current_session->req_pipe_path); // unlinks the request pipe
    unlink(current_session->resp_pipe_path); // unlinks the response pipe
  }
  free(args); // frees the memory allocated for the arguments of the thread
  return NULL;
}















int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if(ems_init(state_access_delay_us) != 0) { // initializes the EMS
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  unlink(argv[1]);

  mkfifo(argv[1], 0666); // creates the FIFO (named pipe)

  int server_fd = open(argv[1], O_RDONLY); // opens the FIFO

  if (server_fd == -1) { // if the FIFO could not be opened, then it does not exist
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }
  
  pthread_t threads[NUMBER_OF_SESSIONS]; // to store the threads for each session
  session_info* buffer[NUMBER_OF_SESSIONS]; // to store the information of each session
  memset(buffer, 0, sizeof(buffer)); // initializes the buffer to 0

  int prod_ptr = 0; // to store the index of the next slot to be produced
  int cons_ptr = 0; // to store the index of the next slot to be consumed
  int count = 0; // to store the number of slots occupied
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // to control access to the buffer
  pthread_cond_t canProd = PTHREAD_COND_INITIALIZER; // to signal that a slot is available for production
  pthread_cond_t canCons = PTHREAD_COND_INITIALIZER; // to signal that a slot is available for consumption

  common_arg* common_args = malloc(sizeof(common_arg)); // allocates memory for the arguments of the thread
  common_args->sessions = buffer;
  common_args->prod_ptr = &prod_ptr;
  common_args->cons_ptr = &cons_ptr;
  common_args->count = &count;
  common_args->mutex = &mutex;
  common_args->canProd = &canProd;
  common_args->canCons = &canCons;

  for (int i = 1; i <= NUMBER_OF_SESSIONS; i++) { // initializes the sessions
    individual_session_args* args = malloc(sizeof(individual_session_args)); // allocates memory for the arguments of the thread

    if (args == NULL) { // if the memory could not be allocated
      fprintf(stderr, "Failed to allocate memory\n");
      return 1;
    }

    args->session_id = i;
    args->common_args = common_args;
    pthread_create(&threads[i - 1], NULL, individual_session, args); // creates the thread
  }

  signal(SIGUSR1, sigusr1_handler); // associate the SIGUSR1 signal to be handled by sigusr1_handler() routine

  char op_code; // to store the type of operation
  ssize_t bytes_read; // to store the number of bytes read
  while (1) {

    pthread_mutex_lock(&sigusr1_mutex);
    if (sigusr1_received) { // if there was a SIGUSR1 signal received
      ems_list_events_signal(STDOUT_FILENO); // print in stdout a list of events IDs along with its seat status
      sigusr1_received = 0; // reset signal flag
    }
    pthread_mutex_unlock(&sigusr1_mutex);  

    bytes_read = read(server_fd, &op_code, sizeof(op_code)); // reads the type of operation

    if (bytes_read != 1 || op_code == 0) { // the pipe is empty
      continue;
    }

    session_info* session = malloc(sizeof(session_info)); // allocates memory for the session

    bytes_read = read(server_fd, session->req_pipe_path, sizeof(char) * PIPE_PATH_MAX); // reads the request pipe path

    if (bytes_read != PIPE_PATH_MAX) { // if the number of bytes read is not PIPE_PATH_MAX, then the pipe is empty
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }

    bytes_read = read(server_fd, session->resp_pipe_path, sizeof(char) * PIPE_PATH_MAX); // reads the response pipe path

    if (bytes_read != PIPE_PATH_MAX) { // if the number of bytes read is not PIPE_PATH_MAX, then the pipe is empty
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }

    pthread_mutex_lock(&mutex); // locks the mutex
    while (count == NUMBER_OF_SESSIONS) { // if the buffer is full
      pthread_cond_wait(&canProd, &mutex); // waits until a slot is available for production
    }
    buffer[prod_ptr] = session; // stores the session in the buffer
    prod_ptr++;
    if (prod_ptr == NUMBER_OF_SESSIONS) { // if the index of the next slot to be produced is NUMBER_OF_SESSIONS, then it wraps around
      prod_ptr = 0;
    }
    count++;
    pthread_cond_signal(&canCons); // signals that a slot is available for consumption
    pthread_mutex_unlock(&mutex); // unlocks the mutex
  }
  
  close(server_fd); // closes the FIFO
  unlink(argv[1]); // removes the FIFO
  ems_terminate(); // terminates the EMS
  return 0;
}