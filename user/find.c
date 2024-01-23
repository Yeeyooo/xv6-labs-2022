#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

// find file name after last slash.
char *fmtname(char *path) {
     char *p;
     for (p = path + strlen(path); p >= path && *p != '/'; p--);
     p++;
     return p;
}

// recursively parse given path to find specific file
void parse(char *path, char *filename) {
     char buf[512], *p;
     int fd;
     struct dirent de;  // structure of directory
     struct stat st;
     if ((fd = open(path, 0)) < 0) {
          fprintf(2, "find: cannot open %s\n", path);
          return;
     }
     if (fstat(fd, &st) < 0) {
          fprintf(2, "find: cannot stat %s\n", path);
          close(fd);
          return;
     }
     
     // check the type of given path
     switch(st.type) {
          case T_DEVICE:
          case T_FILE:
               if (!strcmp(fmtname(path), filename)) {
                    printf("%s\n", path);
               }
               break;
          case T_DIR:
               if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
                    printf("find: path too long\n");
                    break;
               }
               strcpy(buf, path);
               p = buf + strlen(buf);
               *p++ = '/';
               // parse every item in this directory
               while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                    if (de.inum == 0) {
                         continue;
                    }
                    if (!strcmp(".", de.name)|| !strcmp("..", de.name)) {
                         continue;
                    }
                    memmove(p, de.name, DIRSIZ);
                    p[DIRSIZ] = 0;
                    int fd2;
                    if ((fd2 = open(buf, 0)) < 0) {
                         fprintf(2, "find: cannot open %s\n", buf);
                         continue;
                    }
                    if (fstat(fd2, &st) < 0) {
                         fprintf(2, "find: cannot stat %s\n", buf);
                         close(fd2);
                         continue;
                    }

                    if (st.type == T_DEVICE  || st.type == T_FILE) {
                         if (!strcmp(filename, de.name)) {
                              printf("%s\n", buf);
                         }
                    }
                    if (st.type == T_DIR) {
                         parse(buf, filename);
                    }
                    close(fd2);
               }
               break;
     }
     close(fd);
}

// find all the files in a directory tree with a specific name
int main(int argc, char *argv[]) {
     if (argc != 3) {
          fprintf(2, "Usage: find path name\n");
          exit(1);
     }
     parse(argv[1], argv[2]);
     exit(0);
}