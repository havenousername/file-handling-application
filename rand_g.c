#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>


int generate_rand(int min, int max) {
//  srand(time(NULL)); //the starting value of random number generation
  int r= rand()%(max - min) + min; //number between min-max
  return r;
}



int main() {
  srand(time(NULL)); //the starting value of random number generation
  for (int i = 0; i < 100; i++) {
          printf("Random from -1 to 4 %d\n", generate_rand(0, 4));
  }
  return 0;
}
