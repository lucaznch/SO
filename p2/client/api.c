#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "api.h"
#include "common/constants.h"
// #include "common/constants.h"

int register_fifo, request_fifo, response_fifo;
const char *request_pipe_path, *response_pipe_path;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  printf("ems_setup()\n");

  request_pipe_path = req_pipe_path;
  response_pipe_path = resp_pipe_path;

  if (mkfifo(req_pipe_path, 0666)) {
    fprintf(stderr, "Failed to create request pipe\n");
    return 1;
  }

  if (mkfifo(resp_pipe_path, 0666)) {
    fprintf(stderr, "Failed to create respond pipe\n");
    return 1;
  }


  register_fifo = open(server_pipe_path, O_WRONLY);

  if (register_fifo == -1) {
    fprintf(stderr, "Failed to open server pipe\n");
    return 1;
  }

  char buf[84]; // 1 + 1 + 40 + 1 + 40 +1

  snprintf(buf, sizeof(buf), "%d %s %s", 1, req_pipe_path, resp_pipe_path);

  write(register_fifo, buf, strlen(buf) + 1);


  request_fifo = open(req_pipe_path, O_WRONLY);
  
  if (request_fifo == -1) {
    fprintf(stderr, "Failed to open request fifo\n");
    return 1;
  }

  response_fifo = open(resp_pipe_path, O_RDONLY);

  if (response_fifo == -1) {
    fprintf(stderr, "Failed to open response fifo\n");
    return 1;
  }


  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  printf("ems_quit()\n");

  char buf[sizeof(char)];

  size_t offset = 0;

  memcpy(buf + offset, &(char){2}, sizeof(char));
  offset += sizeof(char);

  write(request_fifo, buf, offset);

  close(register_fifo);
  close(request_fifo);
  close(response_fifo);
  
  unlink(request_pipe_path);
  unlink(response_pipe_path);
  
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  printf("ems_create(), event_id:%d, num_rows:%lu, num_cols:%lu\n", event_id, num_rows, num_cols);

  char buf[sizeof(char) + sizeof(unsigned int) + sizeof(size_t) * 2];

  size_t offset = 0;

  memcpy(buf + offset, &(char){3}, sizeof(char));
  offset += sizeof(char);

  memcpy(buf + offset, &event_id, sizeof(unsigned int));
  offset += sizeof(unsigned int);

  memcpy(buf + offset, &num_rows, sizeof(size_t));
  offset += sizeof(size_t);

  memcpy(buf + offset, &num_cols, sizeof(size_t));
  offset += sizeof(size_t);


  write(request_fifo, buf, offset);


  int response;

  read(response_fifo, &response, sizeof(int));

  if (response == 0) {
    printf("exiting ems_Create() all good\n");
    return 0;
  }
  else {
    printf("not good Create\n");
    ems_quit();
    return 1;
  }
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  printf("ems_reserve(), event_id:%d, num_seats:%lu, xs:%lu, ys:%lu\n", event_id, num_seats, (*xs), (*ys));

  char buf[sizeof(char) + sizeof(unsigned int) + sizeof(size_t) + MAX_RESERVATION_SIZE * sizeof(size_t) * 2];

  size_t offset = 0;

  memcpy(buf + offset, &(char){4}, sizeof(char));
  offset += sizeof(char);

  memcpy(buf + offset, &event_id, sizeof(unsigned int));
  offset += sizeof(unsigned int);

  memcpy(buf + offset, &num_seats, sizeof(size_t));
  offset += sizeof(size_t);

  memcpy(buf + offset, xs, MAX_RESERVATION_SIZE * sizeof(size_t));
  offset += MAX_RESERVATION_SIZE * sizeof(size_t);

  memcpy(buf + offset, ys, MAX_RESERVATION_SIZE * sizeof(size_t));
  offset += MAX_RESERVATION_SIZE * sizeof(size_t);

  write(request_fifo, buf, offset);


  int response;

  read(response_fifo, &response, sizeof(int));

  if (response == 0) {
    printf("exiting ems_Reserve() all good\n");
    return 0;
  }
  else {
    printf("not good Reserve\n");
    ems_quit();
    return 1;
  }
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  printf("ems_show(), out_fd:%d, event_id:%d\n", out_fd, event_id);

  char buf[sizeof(char) + sizeof(unsigned int)];

  size_t offset = 0;

  memcpy(buf + offset, &(char){5}, sizeof(char));
  offset += sizeof(char);

  memcpy(buf + offset, &event_id, sizeof(unsigned int));
  offset += sizeof(unsigned int);

  write(request_fifo, buf, offset);

  return 0;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  printf("ems_list_event(), out_fd:%d\n", out_fd);

  char buf[sizeof(char)];

  size_t offset = 0;

  memcpy(buf + offset, &(char){6}, sizeof(char));
  offset += sizeof(char);


  write(request_fifo, buf, offset);

  return 0;
}
