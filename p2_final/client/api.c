#include "api.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#define PIPE_PATH_MAX 40

setup_data ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  setup_data setup_info = {0, 0, 0, 0};

  int server_fd = open(server_pipe_path, O_WRONLY);
  
  if (server_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  unlink(resp_pipe_path);
  unlink(req_pipe_path);

  mkfifo(req_pipe_path, 0660); // creates the request pipe
  mkfifo(resp_pipe_path, 0660); // creates the response pipe

  char op_code = '1'; // 1 for connect

  ssize_t bytes_written = write(server_fd, &op_code, sizeof(char)); // writes the operation code

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  char pipe_path[PIPE_PATH_MAX];

  memset(pipe_path, 0, PIPE_PATH_MAX); // clears the pipe path
  strcpy(pipe_path, req_pipe_path); // copies the request pipe path to the pipe path

  bytes_written = write(server_fd, pipe_path, sizeof(char) * PIPE_PATH_MAX); // writes the request pipe path

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  memset(pipe_path, 0, PIPE_PATH_MAX); // clears the buffer
  strcpy(pipe_path, resp_pipe_path); // copies the response pipe path to the buffer

  bytes_written = write(server_fd, pipe_path, sizeof(char) * PIPE_PATH_MAX); // writes the response pipe path

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  bytes_written = write(server_fd, "\0", 1);

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  int req_fd = open(req_pipe_path, O_WRONLY);
  int resp_fd = open(resp_pipe_path, O_RDONLY);

  if (req_fd == -1 || resp_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  int session_id; // session id from the server
  read(resp_fd, &session_id, sizeof(int)); // reads the session id from the server

  if (session_id == -1) {
    fprintf(stderr, "Failed to connect to server\n");
    setup_info.return_code = 1;
    return setup_info;
  }

  close(server_fd);

  setup_info.session_id = session_id;
  setup_info.req_fd = req_fd;
  setup_info.resp_fd = resp_fd;
  setup_info.return_code = 0;
  return setup_info;
}

int ems_quit(int req_fd, int resp_fd, char const* req_pipe_path, char const* resp_pipe_path) { 
  char op_code = '2'; // 2 for quit

  ssize_t bytes_written = write(req_fd, &op_code, sizeof(char)); // writes the operation code

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  
  close(req_fd); // closes the request pipe
  close(resp_fd); // closes the response pipe

  unlink(req_pipe_path); // unlinks the request pipe
  unlink(resp_pipe_path); // unlinks the response pipe

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols, int req_fd, int resp_fd) {
  char op_code = '3'; // 3 for create

  ssize_t bytes_written = write(req_fd, &op_code, sizeof(char)); // writes the operation code

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, &event_id, sizeof(unsigned int)); // writes the event id

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, &num_rows, sizeof(size_t)); // writes the number of rows

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, &num_cols, sizeof(size_t)); // writes the number of columns

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  int return_code; // return code from the server

  bytes_written = read(resp_fd, &return_code, sizeof(int)); // reads the return code from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  
  return return_code;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys, int req_fd, int resp_fd) {
  char op_code = '4'; // 4 for reserve

  ssize_t bytes_written = write(req_fd, &op_code, sizeof(char));

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, &event_id, sizeof(unsigned int));

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, &num_seats, sizeof(size_t));

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, xs, sizeof(size_t) * num_seats);

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, ys, sizeof(size_t) * num_seats);

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  int return_code; // return code from the server
  bytes_written = read(resp_fd, &return_code, sizeof(int)); // reads the return code from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  return return_code;
}

int ems_show(int out_fd, unsigned int event_id, int req_fd, int resp_fd) {
  char op_code = '5'; // 5 for show

  ssize_t bytes_written = write(req_fd, &op_code, sizeof(char));

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  bytes_written = write(req_fd, &event_id, sizeof(unsigned int));

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  int return_code; // return code from the server
  size_t num_rows; // number of rows
  size_t num_cols; // number of columns
  unsigned int *seats; // array of seats

  bytes_written = read(resp_fd, &return_code, sizeof(int)); // reads the return code from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  if (return_code != 0) {
    fprintf(stderr, "Failed to show event\n");
    return 1;
  }

  bytes_written = read(resp_fd, &num_rows, sizeof(size_t)); // reads the number of rows from the server

  if (bytes_written != sizeof(size_t)) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  bytes_written = read(resp_fd, &num_cols, sizeof(size_t)); // reads the number of columns from the server

  if (bytes_written != sizeof(size_t)) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  seats = malloc(sizeof(unsigned int) * num_rows * num_cols); // allocates memory for the seats

  if (seats == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
    return 1;
  }

  bytes_written = read(resp_fd, seats, sizeof(unsigned int) * num_rows * num_cols); // reads the seats from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  size_t i, j; // iterators

  for (i = 0; i < num_rows; i++) {
    for (j = 0; j < num_cols; j++) {
      char id[16];
      sprintf(id, "%u", seats[i * num_cols + j]); // converts the seat id to a string
      write(out_fd, id, strlen(id)); // writes the seat id to the output file

      if (j < num_cols - 1) { // writes a space if the seat is not the last seat in the row
				write(out_fd, " ", 1); 
			}
    }

    write(out_fd, "\n", 1);
  }

  free(seats);

  return 0;
}

int ems_list_events(int out_fd, int req_fd, int resp_fd) {
  char op_code = '6'; // 6 for list events
  ssize_t bytes_written = write(req_fd, &op_code, sizeof(char));

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  int return_code; // return code from the server
  size_t num_events; // number of events
  unsigned int *events; // array of events

  bytes_written = read(resp_fd, &return_code, sizeof(int)); // reads the return code from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  if (return_code != 0) {
    fprintf(stderr, "Failed to list events\n");
    return 1;
  }

  bytes_written = read(resp_fd, &num_events, sizeof(size_t)); // reads the number of events from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  events = malloc(sizeof(unsigned int) * num_events); // allocates memory for the events

  if (events == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
    return 1;
  }

  bytes_written = read(resp_fd, events, sizeof(unsigned int) * num_events); // reads the events from the server

  if (bytes_written == -1) {
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  if (num_events == 0) {
    fprintf(stderr, "No events\n");
  } else {
    size_t i;
    for (i = 0; i < num_events; i++) {
      write(out_fd, "Event: ", 7);
      char id[16];
      sprintf(id, "%u", events[i]);
      write(out_fd, id, strlen(id));
      write(out_fd, "\n", 1);
    }
  }

  free(events);

  return 0;
}
