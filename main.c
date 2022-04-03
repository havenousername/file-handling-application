#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h> 

// GLOBALS
#define DEBUG 0

#define NAME_SIZE 10
#define SIZE 10
#define REGION_SIZE 10
#define BUFFER_SIZE 4096
#define TIMES_SIZE 20
#define FILENAME "1.csv"
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

#define APPLICANT_OVERRIDE 999
#define APPLICANT_DELETE 1000


// Structures and Enums

enum app_action {
  EXIT,
  CREATE_APPLICANT,
  MODIFY_APPLICANT,
  DELETE_APPLICANT,
  READ
};

enum area {
  BARATFA,
  LOVAS,
  SZULA,
  KIYOS_PATAK,
  MALOM_TELEK,
  PASKOM,
  KAPOSZTAS_KERT
};


const char* const area_str[] = {
  [BARATFA] = "Baratfa",
  [LOVAS] = "Lovas",
  [SZULA] = "Szula",
  [KIYOS_PATAK] = "Kiyos-patak",
  [MALOM_TELEK] = "Malom telek",
  [PASKOM] = "Paskom",
  [KAPOSZTAS_KERT] = "Kaposztas kert"
};

struct applicant {
    char* name;
    char* region;
    unsigned short int participation_times;
};


typedef struct applicant applicant_t;

// util functions
void print_applicant(applicant_t **appl);
applicant_t* create_applicant_from_stream();
int append_applicant(FILE **stream, applicant_t **t);

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

applicant_t *new_applicant_from_stream_t(FILE **stream) {
  return new_applicant_from_stream(&stream);
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
    return -1;
  }
}

// CRUD operations for files

int read_file(char* f_name, int (*parser)(char*, FILE**)) {
  unsigned int current_buffer_size = BUFFER_SIZE;
  printf("====Read the file====\n");
  FILE *stream;
  stream = fopen(f_name, "r");
  if (stream == NULL) {
    perror("File opening error");
    exit(1);
  }

  int res = parser(f_name, &stream);

  if (res == -1) {
    perror("Not corrent file format. Please check your file extension and try again");      
    exit(1);
  }
  printf("\n");
  fclose(stream);
  return res;
}


int override_file(char* f_name, char* name, char* region, int flag) {
  applicant_t **applicants = (applicant_t**)calloc(SIZE, sizeof(applicant_t)); 
  FILE *stream;
  stream = fopen(f_name, "r");
  if (stream == NULL) {
    perror("File opening error");
    exit(1);
  }

  int is_selected_applicant = 0;
  int counter = 0;
  while(1) {
    applicant_t *t = new_applicant_from_stream_t(&stream);
    if (t->participation_times == 0) {
      break;
    }
    is_selected_applicant = strcmp(t->name, name) == 0 && strcmp(t->region, region) == 0;
    const int is_delete_applicant = flag == APPLICANT_DELETE && is_selected_applicant;
    if (is_delete_applicant) {
      continue;
    }

    const int is_override_applicant = flag == APPLICANT_OVERRIDE && is_selected_applicant;
    if (is_override_applicant) {
      printf("Override in proccess. Please enter new data\n");
      printf("1. Enter new name\n");
      *(applicants + counter) = create_applicant_from_stream();
    } else {
      *(applicants + counter) = t;
    }
    counter++;
  }
  fclose(stream);
  FILE *stream_override = fopen(f_name, "w");


  for(int i = 0; i < counter; i++) {
    append_applicant(&stream_override, &applicants[i]);
  }

  fclose(stream_override);
  free(applicants);

  if (flag == APPLICANT_DELETE && is_selected_applicant) {
    printf("Successfully deleted\n");
  } else if(flag == APPLICANT_DELETE && !is_selected_applicant) {
    printf("Could not find and delete such bunnie in the contest. Please use \"Create new bunnie\" functionality for that \n");
  } else if (flag == APPLICANT_OVERRIDE && is_selected_applicant) {
    printf("Successfully updated\n");
  } else {
    printf("Could not find and override such bunnie in the contest. Please use \"Create new bunnie\" functionality for that \n");
  }

  return 0;
}


int append_applicant(FILE **stream, applicant_t **t) {
  #if DEBUG
    print_applicant(t);
  #endif
  fprintf(*stream, "%s,%s,%u\n", (*t)->name, (*t)->region, (*t)->participation_times);
  return 0;
}

void append_applicant_to_file(char* f_name, applicant_t *t,  int (*parser)(FILE**, applicant_t**))  {
  FILE *stream;
  stream = fopen(f_name, "a");
  if (stream == NULL) {
    perror("File appending error");
    exit(1);
  }
  printf("====Appending to the file begins=====\n");
  int res = parser(&stream, &t);
  if (res == 1) {
    perror("Something went wrong while appending to the file new data. Please try again later");
    exit(1);
  }
  fclose(stream);
  printf("==Appended successfully===\n");
  print_applicant(&t);
}


void read_all() {
  read_file(FILENAME, parse_csv);
}


char *get_string(FILE *stream, unsigned int buffer_size) {
  char* buffer = (char*) malloc(sizeof(char) *buffer_size);

  char c = getc(stream);
  unsigned int length = 0;
  while (c != EOF && c != '\n' && c != ',') {
    if (length == buffer_size) {
      buffer_size *= 2;
      buffer = realloc(buffer, buffer_size);
    }
    buffer[length] = c;
    c = getc(stream);
    length++;
  }
  buffer[length] = 0;
  return buffer;
}

char *read_stream_string(FILE* fp, size_t size){
    char *str;
    int ch;
    size_t len = 0;
    str = realloc(NULL, sizeof(*str)*size);//size is start size
    if(!str)return str;
    while(EOF!=(ch=fgetc(fp)) && ch != '\n'){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(*str)*(size+=16));
            if(!str)return str;
        }
    }
    str[len++]='\0';

    return realloc(str, sizeof(*str)*len);
}

// printing functions

void print_allowed_regions() {
  for (unsigned int i = 0; i < ARRAY_SIZE(area_str); i++) {
    printf("%u - %s\n", i, area_str[i]);
  }
}

void print_applicant(applicant_t **appl) {
  printf("{ name: %s, region: %s, participation_times: %d }\n", (*appl)->name, (*appl)->region, (*appl)->participation_times);
}

char* get_location() {
  printf("2. Write location number where bunnie lives.\n");
  printf("Note: Allowed locations are: \n");
  print_allowed_regions();
  unsigned int region; 
  scanf("%d", &region);
  while (region > KAPOSZTAS_KERT || region < BARATFA) {
    perror("Region index is out of range. Please type correct number\n");
    scanf("%d", &region);
  }
  char* region_s = (char*) malloc(sizeof(char) * 20);
  strcpy(region_s, area_str[region]);
  return region_s;
}

// overlay functions

applicant_t* create_applicant_from_stream() {
  char* name = NULL;
  fflush(stdin);
  name = read_stream_string(stdin, NAME_SIZE);
  
  char* region_s = get_location();
  // TODO: Check if name-region is unique in the file
  int count_times;
  printf("3. Write number of participations in the contest\n");
  scanf("%d", &count_times);

  // create struct
  applicant_t *t = new_applicant(name, region_s, count_times);

  free(name);
  return t;
}

void create_applicant() {
  // read the data from stdin
  printf("For creating a new bunnie you have to follow these steps:\n");
  printf("1. Write bunnies name.\n");
  // create struct
  applicant_t *t = create_applicant_from_stream();
  append_applicant_to_file(FILENAME, t, append_applicant);
  free(t);
}


void delete_applicant() {
  printf("For deleting a bunnie you will need to provide its name and region (combination must be unique)\n");
  fflush(stdin);
  
  printf("1. Write bunnies name.\n");
  char* name = read_stream_string(stdin, NAME_SIZE);
  printf("2. Write location number where bunnie lives.\n");
  char* region_s = get_location();

  override_file(FILENAME, name, region_s, APPLICANT_DELETE);
}

void override_applicant() {
  printf("For overriding a bunnie you will need to provide its name and region (combination must be unique)\n");
  fflush(stdin);
  
  printf("1. Write bunnies name.\n");
  char* name = read_stream_string(stdin, NAME_SIZE);
  char* region_s = get_location();
  override_file(FILENAME, name, region_s, APPLICANT_OVERRIDE);
}


// Application loop
void app() {
  enum app_action action = READ;
  printf("=======================Easter bunny king======================\n");
  printf("-------------Welcome to the competitition dear bunny--------------\n");
  while(action != EXIT) {
    printf("==Please choose an action==\n");
    printf("0 - EXIT\n");
    printf("1 - CREATE NEW BUNNIE\n");
    printf("2 - OVERRIDE A BUNNIE\n");
    printf("3 - DELETE A BUNNIE\n");
    printf("4 - READ ALL COMPETITORS\n");
    scanf("%u", &action);
    switch(action) {
      case EXIT: break;
      case CREATE_APPLICANT:
        create_applicant();
        break;
      case MODIFY_APPLICANT: 
        override_applicant();
        break;
      case DELETE_APPLICANT:
        delete_applicant();
        break;
      case READ: read_all();
        break;
      default: break;
    }
  }    
}

int main() {
  app();  
  return 0;
}
