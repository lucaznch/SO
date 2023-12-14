#define MAX_RESERVATION_SIZE 256
#define STATE_ACCESS_DELAY_MS 10
#define MAX_FILENAME_LENGHT 256
#define MAX_SEAT 4 // maximum digits of a seat in the event
#define MAX_ID 5 // maximum digits of event ID
#define HELP "Available commands:\nCREATE <event_id> <num_rows> <num_columns>\nRESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\nSHOW <event_id>\nLIST\nWAIT <delay_ms> [thread_id]\nBARRIER\nHELP\n"