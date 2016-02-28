#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int do_dir(const char *location, char *params[]);
int do_file(const char *location, char *params[]);
int check_params(char *params[]);
void print_usage(void);

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
    char *path =
        malloc(sizeof(char) * (strlen(location) + strlen(entry->d_name) + 2));

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
  int i;

  /* 0 = ok or nothing to check */
  /* 1 = unknown predicate */
  /* 2 = missing argument for predicate */
  /* 3 = unknown argument for predicate */
  int status = 0;

  /* parameters start from params[2] */
  for (i = 2; params[i]; i++) {

    /* parameters consisting of a single part */
    if ((strcmp(params[i], "-print") == 0) || (strcmp(params[i], "-ls") == 0) ||
        (strcmp(params[i], "-nouser") == 0) ||
        (strcmp(params[i], "-path") == 0)) {
      continue; /* found a match */
    }

    /* parameters expecting a non-empty second part */
    if ((strcmp(params[i], "-user") == 0) ||
        (strcmp(params[i], "-name") == 0)) {

      if (params[i+1]) {
        continue; /* found a match */
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }

    /* a parameter expecting a restricted second part */
    if (strcmp(params[i], "-type") == 0) {
      if (params[i+1]) {
        if ((strcmp(params[i], "b") == 0) || (strcmp(params[i], "c") == 0) ||
            (strcmp(params[i], "d") == 0) || (strcmp(params[i], "p") == 0) ||
            (strcmp(params[i], "f") == 0) || (strcmp(params[i], "l") == 0) ||
            (strcmp(params[i], "s") == 0)) {
          continue; /* found a match */
        } else {
          status = 3;
          break; /* the second part is unknown */
        }
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }

    status = 1; /* no match found */
    break;      /* don't increment the counter so that we can access the state
                   outside of the loop */
  }

  if (status == 1) {
    printf("%s: unknown predicate: %s\n", params[0], params[i]);
    return EXIT_FAILURE;
  }

  if (status == 2) {
    printf("%s: missing argument to %s\n", params[0], params[i-1]);
    return EXIT_FAILURE;
  }

  if (status == 3) {
    printf("%s: unknown argument to %s: %s\n", params[0], params[i-1],
           params[i]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void print_usage(void) {

  printf(
      "myfind <file or directory> [ <aktion> ]\n"
      "-user <name>|<uid>    entries beloning to a user\n"
      "-name <pattern>       entry names matching a pattern\n"
      "-type [bcdpfls]       entries of a specific type\n"
      "-print                print entries with paths\n"
      "-ls                   print entry details\n"
      "-nouser               entries not beloning to a user\n"
      "-path                 entry paths (incl. names) matching a pattern\n");
}
