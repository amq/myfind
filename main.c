#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int do_dir(const char *location, char *params[]);
int do_file(const char *location, char *params[]);
int check_params(char *params[]);
void print_usage();

int main(int argc, char *argv[]) {
  struct stat attr;

  /* a minimum of 3 input parameters are required */
  /* myfind <file or directory> [ <aktion> ] */
  if (argc < 3) {
    print_usage();
    return EXIT_FAILURE;
  }

  if (check_params(argv)) {
    return EXIT_FAILURE;
  }

  /* both files and directories are accepted as an input */
  if (lstat(argv[1], &attr) == 0) {
    if (S_ISDIR(attr.st_mode)) {
      do_dir(argv[1], argv);
    } else if (S_ISREG(attr.st_mode)) {
      do_file(argv[1], argv);
    }
  } else {
    printf("%s: lstat(%s): %s\n", argv[0], argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int do_dir(const char *location, char *params[]) {
  DIR *dir;
  struct dirent *entry;
  struct stat attr;

  dir = opendir(location);

  if (!dir) {
    printf("%s: opendir(%s): %s\n", params[0], location, strerror(errno));
    return EXIT_FAILURE; /* stops a function call, the program continues */
  }

  while ((entry = readdir(dir))) {

    /* skip '.' and '..' */
    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
      continue;
    }

    /* allocate memory for the entry path */
    char *path = malloc(strlen(location) + strlen(entry->d_name) + 2);

    if (!path) {
      printf("%s: malloc(): %s\n", params[0], strerror(errno));
      return EXIT_FAILURE; /* stops a function call, the program continues */
    }

    /* concat the path with the entry name */
    sprintf(path, "%s/%s", location, entry->d_name);

    /* process the entry */
    do_file(path, params);

    /* if the entry is a directory, call the function recursively */
    if (lstat(path, &attr) == 0) {
      if (S_ISDIR(attr.st_mode)) {
        do_dir(path, params);
      }
    } else {
      printf("%s: lstat(%s): %s\n", params[0], location, strerror(errno));
    }

    free(path);
  }

  if (closedir(dir) != 0) {
    printf("%s: closedir(%s): %s\n", params[0], location, strerror(errno));
  }

  return EXIT_SUCCESS;
}

int do_file(const char *location, char *params[]) {

  /* parameters start from params[2] */
  for (int i = 2; params[i]; i++) {

    if (strcmp(params[i], "-print") == 0) {
      printf("%s\n", location);
    }
    if (strcmp(params[i], "-ls") == 0) {
      /* TODO */
    }
  }

  return EXIT_SUCCESS;
}

int check_params(char *params[]) {
  int status;

  /* parameters start from params[2] */
  for (int i = 2; params[i]; i++) {
    /* a parameter is considered bad unless it passes a check */
    status = 1;

    /* parameters consisting of a single part */
    if ((strcmp(params[i], "-print") == 0) || (strcmp(params[i], "-ls") == 0) ||
        (strcmp(params[i], "-nouser") == 0) ||
        (strcmp(params[i], "-path") == 0)) {
      status = 0;
    }

    /* parameters expecting an unrestricted second part */
    if ((strcmp(params[i], "-user") == 0) ||
        (strcmp(params[i], "-name") == 0)) {
      status = 2;

      /* the second part must not be empty */
      if (params[++i]) {
        status = 0;
        /* increment to the next parameter */
        i++;
      }
    }

    /* a parameter expecting a restricted second part */
    /* after an increment a check for params[i] is required */
    if (params[i] && strcmp(params[i], "-type") == 0) {
      status = 3;

      /* before doing a comparison make sure the second part is not empty */
      if (params[++i]) {
        if ((strcmp(params[i], "b") == 0) || (strcmp(params[i], "c") == 0) ||
            (strcmp(params[i], "d") == 0) || (strcmp(params[i], "p") == 0) ||
            (strcmp(params[i], "f") == 0) || (strcmp(params[i], "l") == 0) ||
            (strcmp(params[i], "s") == 0)) {
          status = 0;
        }
      } else {
        status = 2;
      }
    }

    if (status == 1) {
      printf("%s: unknown predicate: %s\n", params[0], params[i]);
      return EXIT_FAILURE;
    }

    if (status == 2) {
      printf("%s: missing argument to %s\n", params[0], params[--i]);
      return EXIT_FAILURE;
    }

    if (status == 3) {
      printf("%s: unknown argument to %s: %s\n", params[0], params[--i],
             params[i]);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

void print_usage() {

  printf("myfind <file or directory> [ <aktion> ]\n"
         "-user <name>|<uid>\n"
         "-name <pattern>\n"
         "-type [bcdpfls]\n"
         "-print\n"
         "-ls\n"
         "-nouser\n"
         "-path\n");
}