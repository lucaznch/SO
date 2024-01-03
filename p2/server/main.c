#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/stat.h> // mkfifo
#include <fcntl.h> // open
#include <unistd.h> // close read write

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

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

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }





  //TODO: Intialize server, create worker threads

  int register_fifo, request_fifo, response_fifo, op_code;
  char req_pipe_path[40], resp_pipe_path[40], buf[84];

  // unlink(argv[1]);

  if (mkfifo(argv[1], 0666) == -1) { // create Register FIFO: Used for client session requests
    fprintf(stderr, "Failed to create register pipe: %s\n", argv[1]);
    return 1;
  }

  register_fifo = open(argv[1], O_RDONLY);

  if (register_fifo == -1) {
    fprintf(stderr, "Failed to open register pipe for reading\n");
    return 1;
  }


  while (1) {
    //TODO: Read from pipe
    //TODO: Write new client to the producer-consumer buffer

    if (read(register_fifo, buf, sizeof(buf)) > 0) {
      printf("read from client: %s\n", buf);
      break;
    }
  }

  sscanf(buf, "%d %s %s", &op_code, req_pipe_path, resp_pipe_path);

  request_fifo = open(req_pipe_path, O_RDONLY);

  if (request_fifo == -1) {
    fprintf(stderr, "Failed to open request fifo: %s\n", req_pipe_path);
    return 1;
  }

  response_fifo = open(resp_pipe_path, O_WRONLY);

  if (response_fifo == -1) {
    fprintf(stderr, "Failed to open response fifo: %s\n", resp_pipe_path);
    return 1;
  }


  // read from request fifo!
  while (1) {
    //char request[64];
    char op;

    if (read(request_fifo, &op, 1) > 0) {
      if (op == 2) {
        printf("ems_quit(), OP_CODE=2\n");
        break;
      }
      else if (op == 3) {
        printf("ems_create(), OP_CODE=3\n");
        
        unsigned int event_id;
        size_t num_rows, num_cols;

        read(request_fifo, &event_id, sizeof(unsigned int));
        read(request_fifo, &num_rows, sizeof(size_t));
        read(request_fifo, &num_cols, sizeof(size_t));

        printf("%d %lu %lu\n", event_id, num_rows, num_cols);

        int response;

        if (ems_create(event_id, num_rows, num_cols)) {
          fprintf(stderr, "Failed to create event\n");
          response = 1;
          break;
        }
        else
          response = 0;

        write(response_fifo, &response, sizeof(int));
      }
      else if (op == 4) {
        printf("ems_reserve(), OP_CODE=4\n");

        unsigned int event_id;
        size_t num_seats, xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

        read(request_fifo, &event_id, sizeof(unsigned int));
        read(request_fifo, &num_seats, sizeof(size_t));
        read(request_fifo, xs, MAX_RESERVATION_SIZE * sizeof(size_t));
        read(request_fifo, ys, MAX_RESERVATION_SIZE * sizeof(size_t));

        printf("%d %lu %lu %lu\n", event_id, num_seats, *xs, *ys);

        int response;

        if (ems_reserve(event_id, num_seats, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
          response = 1;
          break;
        }
        else
          response = 0;
        
        write(response_fifo, &response, sizeof(int));
      }
      else if (op == 5) {
        printf("ems_show(), OP_CODE=5\n");

        unsigned int event_id;

        read(request_fifo, &event_id, sizeof(unsigned int));

        printf("%d\n", event_id);

        // ems_show(ffdd, event_id);
      }
      else if (op == 6) {
        printf("ems_list_events(), OP_CODE=6\n");

        // ems_list_events(response_fifo);
      }
    }
  }


  //TODO: Close Server
  close(register_fifo);

  close(request_fifo);
  close(response_fifo);

  unlink(argv[1]);

  ems_terminate();
}