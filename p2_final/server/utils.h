#define PIPE_PATH_MAX 40

typedef struct {
    char req_pipe_path[PIPE_PATH_MAX];
    char resp_pipe_path[PIPE_PATH_MAX];
} session_info;