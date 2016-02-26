#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int do_dir(const char *location, char *params[]);
int do_file(const char *location, char *params[]);
void print_usage();

int main(int argc, char *argv[]) {
  struct stat attr;

  /* minimum of 3 input parameters are required */
  /* myfind <file or directory> [ <aktion> ] */
  if (argc < 3) {
    print_usage();
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
    return EXIT_FAILURE;
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
      return EXIT_FAILURE;
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

  for (int i = 0; params[i]; i++) {

    if (strcmp(params[i], "-print") == 0) {
      printf("%s\n", location);
    }
    if (strcmp(params[i], "-ls") == 0) {
      /* TODO */
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