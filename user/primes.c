#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void cull(int p) {
     int n;
     for (;;) {
          int num = read(0, &n, sizeof(n));
          if (num == 0) break;
          if (n % p != 0) {
               write(1, &n, sizeof(n));
          }
     }
}

void redirect(int k, int fd[2]) {
     close(k);
     dup(fd[k]);
     close(fd[0]);
     close(fd[1]);
}

void sink() {
     int fd[2];
     int p;       // p is a prime
     for (;;) {
          int num = read(0, &p, sizeof(p));
          if (num == 0) {
               break;
          }
          printf("prime %d\n", p);
          pipe(fd);
          if (fork()) {     // In parent process
               redirect(1, fd);
               cull(p);
               close(1);
               wait(0);    // wait for current child process to terminate
          } 
          else {            // In child process
               redirect(0, fd);
               continue;
          }
     }
}

int main(int argc, char *argv[]) {
     int fd[2];   // pipe descriptors
     pipe(fd);
     if (fork()) {
          redirect(1, fd);
          int n;
          for (n = 2; n <= 35; n++) {         // feed the numbers 2 through 35 into the pipeline
               write(1, &n, sizeof(n));
          }
          close(1);
          wait(0);
     }
     else {     // child process
          redirect(0, fd);
          sink();
          close(0);
     }
     exit(0);
}