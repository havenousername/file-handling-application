#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h> 

// GLOBALS
#define NAME_SIZE 10
#define REGION_SIZE 10
#define BUFFER_SIZE 4096
#define TIMES_SIZE 20

// Structures and Enums

enum app_action {
  EXIT,
  CREATE_LIST,
  MODIFY_APPLICANT 
};

struct applicant {
    char* name;
    char* region;
    unsigned short int participation_times;
};


typedef struct applicant applicant_t;


// Initializers
applicant_t *new_applicant(char *name, char *region, unsigned short int times) {
  applicant_t *new = malloc(sizeof(applicant_t));
  new->name = malloc(strlen(name));
  new->region = malloc(strlen(region));
  new->participation_times = times;
  strcpy(new->name, name);
  strcpy(new->region, region);
  return new;
}

char *get_char(FILE ****stream, unsigned int buffer_size) {
  char* buffer = (char*) malloc(sizeof(char) *buffer_size);

  char c = getc(***stream);
  unsigned int length = 0;
  while (c != EOF && c != '\n' && c != ',') {
    if (length == buffer_size) {
      buffer_size *= 2;
      buffer = realloc(buffer, buffer_size);
    }
    buffer[length] = c;
    c = getc(***stream);
    length++;
  }
  buffer[length] = 0;
  return buffer;
}

applicant_t *new_applicant_from_stream(FILE ***stream) {
  unsigned int name_size = NAME_SIZE,
               region_size = REGION_SIZE, 
               times_size = TIMES_SIZE; 
  char* name = get_char(&stream, name_size);
  char* region = get_char(&stream, region_size);
  int times =  atoi(get_char(&stream, times_size));

  applicant_t* appl = new_applicant(name, region, times);

  free(region);
  free(name);
  return appl;
}


// Parsers
int parse_csv(char *filename, FILE **stream) {
  char* format_check = strstr(filename, ".csv");
  if (format_check) {
    while(1) {
      applicant_t*  t = new_applicant_from_stream(&stream);
      if (t->participation_times == 0) {
        break;
      }
      printf("Name: %s", t->name);
      printf(", Region: %s", t->region);
      printf(", Times: %d\n", t->participation_times);
      free(t);
    }

    return 0;
  } else {
    return 1;
  }
}

// CRUD operations for files

void read_file(char* f_name, int (*parser)(char*, FILE**)) {
  unsigned int current_buffer_size = BUFFER_SIZE;
  
  printf("====Read the file====\n");
  FILE *stream;
  stream = fopen(f_name, "r");
  if (stream == NULL) {
    perror("File opening error");
    exit(1);
  }

  int res = parser(f_name, &stream);

  if (res == 1) {
    perror("Not corrent file format. Please check your file extension and try again");      
    exit(1);
  }
  printf("\n");
  fclose(stream);
}


// Application loop
void app() {
  char action;
    
}

int main() {
    read_file("1.csv", parse_csv);    	
    return 0;
}
