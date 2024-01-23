#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/fs.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
     char buf[512];
     int p = 0;    // pointer
     char a;

     char *argv_list[MAXARG];
     for (int i = 1; i < argc; i++) {
          argv_list[i - 1] = (char *)malloc(strlen(argv[i]) + 1);
          strcpy(argv_list[i - 1], argv[i]);
     }

     int end = argc - 1;
     while (read(0, &a, sizeof(a)) != 0) {   // still has input, so read a character at a time
          if (a == ' ') {
               buf[p] = '\0';
               argv_list[end] = (char *)malloc(strlen(buf) + 1);
               strcpy(argv_list[end++], buf);
               p = 0;
          }
          else if (a == '\n') {
               buf[p] = '\0';
               argv_list[end] = (char *)malloc(strlen(buf) + 1);
               strcpy(argv_list[end++], buf);
               p = 0;
               argv_list[end] = 0;

               int pid = fork();
               if (pid == 0) {
                    exec(argv_list[0], argv_list);
               }
               else if (pid > 0) {
                    wait(0);
               }
               for (int i = argc - 1; i < end; i++) {
                    free(argv_list[i]);
               }
               end = argc - 1;

          }
          else {
               buf[p++] = a;
          }
     }
     for (int i = 0; i < argc - 1; i++) {
          free(argv_list[i]);
     }
     exit(0);
}