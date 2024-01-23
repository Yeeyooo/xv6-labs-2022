#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
     if (argc != 2) {    // print error message if no argument
          fprintf(2, "Usage: sleep time\n"); 
          exit(1);
     }
     int duration = atoi(argv[1]);
     sleep(duration);
     exit(0);     // main should call exit(0) when it's done
}