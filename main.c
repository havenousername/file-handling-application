#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h> 

#include <sys/time.h>
#include <unistd.h>  //fork
#include <sys/wait.h> //waitpid

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
  READ,
  READ_REGION,
  START_COMPETITION
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
char* get_location();
void flush_stdin();

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
      print_applicant(&t);
      free(t);
    }
    return 0;
  } else {
    return -1;
  }
}

int get_applicants_by_region(char* region, FILE **stream) {
  applicant_t **applicants = (applicant_t**)calloc(SIZE, sizeof(applicant_t));
  int counter = 0;  
  while (1) {
    applicant_t* t = new_applicant_from_stream(&stream);
    if (strcmp(t->region, region) == 0) {
        print_applicant(&t);
        counter++;
    }

    if (t->participation_times == 0) {
      break;
    }
    free(t);
  }

  if (counter == 0) {
    printf("Could not find any applicant in this region. Please try again or create some bunnies in it\n");
  }

  return 0;
}

int is_inside_locations(const char** regions, const char* region, int region_size) {
    for (int i = 0; i < region_size; i++) {
      if (strcmp(*(regions + i), region) == 0) {
        return 1;
      }
    }
    return 0;
}
struct applicants_arr {
  applicant_t** applicants;
  int size; 
};


typedef struct applicants_arr applicants_arr_t;

applicants_arr_t  get_applicants_by_regions(const char* f_name, const char** regions) {
   int applicants_size = 1;
   unsigned int current_buffer_size = BUFFER_SIZE;
   printf("===Get all required regions===\n");
   FILE *stream;
   stream = fopen(f_name, "r");
   if (stream == NULL) {
      perror("File opening error. Returning...\n");
      exit(1);
   }
   applicant_t **applicants = (applicant_t**)malloc(applicants_size * sizeof(applicant_t*));
   int counter = 0;

   while (1) {
      applicant_t* t = new_applicant_from_stream_t(&stream);

      if (t->participation_times == 0) {
        break;
      } 

      if (is_inside_locations(regions, t->region, 3)) {
        applicants[counter] = t;
        counter++;
      }
   }

   if (counter == 0) {
    printf("||WARNING|| No participants in the provided areas\n");
   }

   applicants_arr_t t;
   t.applicants = applicants;
   t.size = counter;

   return t;
}

// CRUD operations for files

int read_file(char* f_name, int (*parser)(char*, FILE**), char* filter) {
  unsigned int current_buffer_size = BUFFER_SIZE;
  printf("====Read the file====\n");
  FILE *stream;
  stream = fopen(f_name, "r");
  if (stream == NULL) {
    perror("File opening error");
    exit(1);
  }

  int res = filter != NULL ? parser(filter, &stream) : parser(f_name, &stream);

  if (res == -1) {
    perror("Not corrent file format. Please check your file extension and try again");      
    exit(1);
  }
  printf("\n");
  fclose(stream);
  return res;
}


int override_file(char* f_name, char* name, char* region, int flag) {
  int applicants_size = SIZE;
  applicant_t **applicants = (applicant_t**)calloc(applicants_size, sizeof(applicant_t)); 
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

    if (counter == applicants_size) {
      applicants_size *= 2;
      applicants = (applicant_t**)realloc(applicants, applicants_size * sizeof(applicant_t));
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
  read_file(FILENAME, parse_csv, NULL);
}

void read_region() {
  printf("Please provide a location which you would like to print:\n");
  char* location = get_location();
  read_file(FILENAME, get_applicants_by_region, location);
}


char *get_string(FILE *stream, unsigned int buffer_size) {
  char* buffer = (char*) malloc(sizeof(char) *buffer_size);

  char c = getc(stream);
  unsigned int length = 0;
  while (c != EOF && c != '\n' && c != ',') {
    if (length == buffer_size) {
      buffer_size *= 2;
      buffer = realloc(buffer, sizeof(char) * buffer_size);
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
  flush_stdin();
  
  printf("1. Write bunnies name.\n");
  char* name = read_stream_string(stdin, NAME_SIZE);
  char* region_s = get_location();
  override_file(FILENAME, name, region_s, APPLICANT_OVERRIDE);
}


void flush_stdin() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}

struct competitor {
  char name[50];
  int eggs;
};

typedef struct competitor competitor_t;

int generate_rand(int min, int max) {
//  srand(time(NULL)); //the starting value of random number generation
  int r= rand()%(max - min) + min; //number between min-max
  return r;
}

char* int_to_string(int i) {
  int length = snprintf( NULL, 0, "%d", i );
  char* str = malloc( length + 1 );
  snprintf( str, length + 1, "%d", i );

  return str;
}

char* max_search(competitor_t *competitors, int size) {
  char* max_bunnie = malloc(sizeof(char) * 8);  
  int max_eggs = 0;
  for (int i = 0; i < size; i++) {
    if (max_eggs < competitors[i].eggs) {
      max_eggs = competitors[i].eggs;
      strcpy(max_bunnie, competitors[i].name);
    }
  }

  return max_bunnie;
}

char* stringify_applicants_by_area(const char** child_areas) {
  applicants_arr_t applicant1 = get_applicants_by_regions(FILENAME, child_areas);
  char* child1_names = malloc(applicant1.size * 100); 
  int size = 0;
  for (int i = 0; i < applicant1.size; i++) {
    size += strlen(applicant1.applicants[i]->name) + 1;
    strcat(strcat(child1_names, applicant1.applicants[i]->name), ",");
  }
  return child1_names;
}

struct competitor_t_sized {
  competitor_t* competitors;
  int size;
};

typedef struct competitor_t_sized competitor_t_sized_t;

competitor_t_sized_t generate_child_competitors(char* buffer) {
  competitor_t* competitors = malloc(strlen(buffer) * sizeof(competitor_t));
  FILE *stream;
  stream = fmemopen(buffer, strlen(buffer), "r");

  char ch;
  int index_i = 0;
  int index_j = 0;

  while((ch = fgetc (stream)) != EOF){  
    if (ch == ',') {
      competitors[index_i].eggs = generate_rand(1, 100);
      index_i++;
      index_j = 0;
      continue;
    }
    competitors[index_i].name[index_j] = ch;
    index_j++;
  }

  fclose (stream);
  competitor_t_sized_t t;
  t.competitors = competitors;
  t.size = index_i;
  return t;
}

competitor_t_sized_t generate_parent_competitors(char* buffer) {
  competitor_t* competitors = malloc(strlen(buffer) * sizeof(competitor_t));
  FILE *stream;
  stream = fmemopen(buffer, strlen(buffer), "r");

  char ch;
  int index_i = 0;
  int index_j = 0;
  int first_letter = 1;
  char* eggs_str = malloc(sizeof(char) * 3);

  while((ch = fgetc (stream)) != EOF){  
    if (ch == '\n') {
      competitors[index_i].eggs = atoi(eggs_str);
      first_letter = 1;
      index_i++;
      index_j = 0;
      continue;
    }

    if (ch == ',') {
      first_letter = 0;
      index_j = 0;
      continue;
    }

    if (first_letter) {
      competitors[index_i].name[index_j] = ch;
    } else {
      eggs_str[index_j] = ch;
    }
    index_j++;
  }
  competitor_t_sized_t t;
  t.competitors = competitors;
  t.size = index_i;
  return t;
}

char* collect_eggs(competitor_t_sized_t c) {
  char* buffer = calloc(c.size, sizeof(char) * 2);
  for (int i = 0; i < c.size; i++) {
    printf("Name: %s, Eggs %d \n", c.competitors[i].name, c.competitors[i].eggs);
    char* eggs_str = malloc(sizeof(char) * 8);
    strcat(eggs_str, ",");
    strcat(eggs_str, int_to_string(c.competitors[i].eggs));
    strcat(eggs_str, "\n");
    strcat(buffer, strcat(c.competitors[i].name, eggs_str));
  }
  return buffer;
}

void reset_rand() {
  srand(time(NULL)); //the starting value of random number generation
}

void start_competition() {
  printf("Starting competition now...\n");

  const char* child2_areas[] = {
    area_str[MALOM_TELEK],
    area_str[PASKOM],
    area_str[KAPOSZTAS_KERT]
  };

  int first_child_pipe[2];
  int second_child_pipe[2];

  int first_parent_pipe[2];
  int second_parent_pipe[2];
  
  // applicants_arr_t applicant1 = get_applicants_by_regions(FILENAME, child1_areas);
  // applicants_arr_t applicant2 = get_applicants_by_regions(FILENAME, child2_areas);

  if (pipe(first_child_pipe) == -1) 
  {
    perror("Opening error!");
    exit(EXIT_FAILURE);
  }

  if (pipe(second_child_pipe) == -1) {
    perror("Opening error!");
    exit(EXIT_FAILURE);
  }

  if (pipe(first_parent_pipe) == -1) {
    perror("Opening error!");
    exit(EXIT_FAILURE);
  }

  if (pipe(second_parent_pipe) == -1) {
    perror("Opening error!");
    exit(EXIT_FAILURE);
  }

  int status;
  pid_t superviser1 = fork();
  if (superviser1 < 0) {
   perror("The forking of a new process could not be succeded.\n");
   exit(1);
  }

  if (superviser1 > 0) {
    pid_t superviser2 = fork();
    if (superviser2 < 0) {
      perror("The forking of a new process could not be succeded.\n");
      exit(1);
    }

    if (superviser2 > 0) {
      // close unused pipes
      close(first_child_pipe[0]);
      close(second_child_pipe[0]);
      close(first_parent_pipe[1]);
      close(second_parent_pipe[1]);

      printf("Parent process\n");

      // prepare data for child one
      const char* child1_areas[] = {
        area_str[BARATFA],
        area_str[LOVAS],
        area_str[SZULA],
        area_str[KIYOS_PATAK]
      };

      char* child1_names = stringify_applicants_by_area(child1_areas);
      write(first_child_pipe[1], child1_names, strlen(child1_names));

      // prepare data for child 2
      const char* child2_areas[] = {
        area_str[MALOM_TELEK],
        area_str[PASKOM],
        area_str[KAPOSZTAS_KERT],
      };

      char* child2_names = stringify_applicants_by_area(child2_areas);
      write(second_child_pipe[1], child2_names, strlen(child2_names));
 

      printf("Parent: Start sleeping...\n");
      sleep(2);

      // receive data from child 1 & 2
      char* buffer = calloc(240, 4);
      char* buffer2 = calloc(240, 4);

      read(first_parent_pipe[0], buffer, 240);
      read(second_parent_pipe[0], buffer2, 240);

      printf("Parent: First child receive: %s\n", buffer);
      printf("Parent: Second child receive: %s\n", buffer2);

      strcat(buffer, buffer2);

      // // prepare the buffer for the max-search
      competitor_t_sized_t c = generate_parent_competitors(buffer);
      competitor_t* competitors = c.competitors;
      int index_i = c.size;

      for (int i = 0; i < index_i; i++) {
        printf("Parent: Received in parent Name: %s, Eggs %d \n", competitors[i].name, competitors[i].eggs);
      }

      char* max_bunnie = max_search(competitors, index_i);
      printf("\n");
      printf("||||WINNER OF THE COMPETITION IS %s. CONGRADULATIONS. ||||\n", max_bunnie);
      printf("\n");
      printf("You can start a new bunnie competition now...\n");
      sleep(1);

      // free memory parent
      free(max_bunnie);
      free(child1_names);
      free(child2_names);
      free(buffer);
      free(buffer2);
      free(competitors);

      // close used pipes
      close(second_child_pipe[1]);
      close(first_child_pipe[1]);
      close(first_parent_pipe[0]);
      close(second_parent_pipe[0]);
    } else {
      // close unused pipes
      close(second_child_pipe[1]);
      close(second_parent_pipe[0]);

      printf("Child 2 process\n");
      char* buffer = calloc(60, 4);
      // reset random
      reset_rand();
      sleep(1);
      read(second_child_pipe[0], buffer, 60);
      printf("Child 2: Bunnies data received: %s \n", buffer);
      competitor_t_sized_t competitors = generate_child_competitors(buffer);

      printf("====Child 2: Ready for parent data ====\n");
      printf("===Child 2: Preparing Data to be sent back to parent===\n");

      char* buffer2 = collect_eggs(competitors);
      printf("Child 1: Buffer data: \n%s", buffer2);
      write(second_parent_pipe[1], buffer2, strlen(buffer2));

      // free memory child 2
      free(competitors.competitors);
      free(buffer);
      free(buffer2);
      // close used pipes
      close(second_child_pipe[1]);
      close(second_parent_pipe[0]);
      exit(0);
    }
  } else {
    // close unused pipes
    close(first_child_pipe[1]);
    close(first_parent_pipe[0]);

    printf("Child 1 process\n");
    char* buffer = calloc(60, 4);
    // reset random
    reset_rand();
    sleep(1);
    read(first_child_pipe[0], buffer, 60);

    printf("Child 1: Bunnies data received: %s \n", buffer);

    competitor_t_sized_t competitors = generate_child_competitors(buffer);

    printf("====Child 1: Ready for parent data ====\n");
    printf("===Child 1: Preparing Data to be sent back to parent===\n");

    char* buffer2 = collect_eggs(competitors);
    printf("Child 1: Buffer data: \n%s", buffer2);
    write(first_parent_pipe[1], buffer2, strlen(buffer2));

    // free memory child 1
    free(competitors.competitors);
    free(buffer);
    free(buffer2);

    // close used pipes
    close(first_parent_pipe[1]);
    close(first_child_pipe[0]);
    exit(0);
  }
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
    printf("5 - READ SELECTED REGION\n");
    printf("6 - START COMPETITION\n");
    scanf("%u", &action);
	
    flush_stdin();
    switch(action) {
      case EXIT: 
        printf("Thank you for using our application. Goodbye!\n");
        break;
      case CREATE_APPLICANT:
        create_applicant();
        break;
      case MODIFY_APPLICANT: 
        override_applicant();
        break;
      case DELETE_APPLICANT:
        delete_applicant();
        break;
      case READ: 
        read_all();
        break;
      case READ_REGION:
        read_region(); 
        break;
      case START_COMPETITION:
        start_competition();
      default: break;
    }
  }    
}

int main() {
  app();  
  return 0;
}
