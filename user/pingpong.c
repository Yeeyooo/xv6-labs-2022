#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// pipe ping: parent write, child read
// pipe pong: parent read, child write
int main(int argc, char *argv[]) {
     int ping[2], pong[2];
     if (pipe(ping) == -1 || pipe(pong) == -1) {    // make pipes
          fprintf(2, "Something is wrong with ping pong\n");
          exit(1);
     }
     int pid = fork();
     if (pid == 0) {
          char buf[1];
          close(ping[1]);
          close(pong[0]);
          if (read(ping[0], buf, 1) != 1) {
               fprintf(2, "child read failed\n");
               exit(1);
          }
          printf("%d: received ping\n", getpid());
          if (write(pong[1], buf, 1) != 1) {
               fprintf(2, "child write failed\n");
               exit(1);
          }
          close(ping[0]);
          close(pong[1]);
          exit(0);
     }
     else if (pid > 0) {
          close(ping[0]);
          close(pong[1]);
          char buf[1];
          buf[0] = 'a';
          if (write(ping[1], buf, 1) != 1) {
               fprintf(2, "parent write failed\n");
               exit(1);
          }
          wait(0);
          if (read(pong[0], buf, 1) != 1) {
               fprintf(2, "parent read failed\n");
               exit(1);
          }
          printf("%d: received pong\n", getpid());
          close(ping[1]);
          close(pong[0]);
          exit(0);
     }
     else {
          exit(1);
     }
}

     // if (pid == 0) {    // child process
     //      close(ping[1]);
     //      close(pong[0]);

     //      close(0);
     //      dup(ping[0]);
     //      close(ping[0]);
     //      close(1);
     //      dup(pong[1]);
     //      close(pong[1]);
     //      printf("pipes operations done in child\n");
     //      char buf[1];
     //      // while (!read(0, &byte, sizeof(byte))){}
     //      if (read(0, buf, 1) != 1) {
     //           printf("Child read failed\n");
     //           exit(1);
     //      }
     //      printf("%d: received ping\n", getpid());
     //      // while (!write(1, &byte, sizeof(byte))) {}
     //      if (write(1, buf, 1) != 1) {
     //           printf("Child write failed\n");
     //           exit(1);
     //      }
     //      close(0);
     //      close(1);
     //      // write(1, &byte, 1);
     //      printf("Byte has been sent in child\n");
     //      exit(0);
     // }
     // else if (pid > 0) {
     //      close(ping[0]);
     //      close(pong[1]);

     //      close(0);
     //      dup(pong[0]);
     //      close(pong[0]);
     //      close(1);
     //      dup(ping[1]);
     //      close(ping[1]);

     //      printf("pipes operations done in parent\n");
     //      char buf[1];
     //      buf[0] = 'a';
     //      if (write(1, buf, 1) != 1) {
     //           printf("parent write failed\n");
     //           exit(1);
     //      }
     //      close(1);
     //      printf("Byte had been sent in parent\n");
     //      // wait(0);
     //      if (read(0, buf, 1) != 1) {
     //           printf("parent read failed\n");
     //           exit(1);
     //      }
     //      printf("%d: received pong\n", getpid());
     //      close(0);
     //      exit(0);
     // }
     // else {
     //      exit(1);
     // }