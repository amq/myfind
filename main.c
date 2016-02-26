#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int do_dir(const char *location, char *params[]);
int do_file(const char *location, char *params[]);
int print_usage();

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage();
    return EXIT_FAILURE;
  }

  do_dir(argv[1], argv);

  return EXIT_SUCCESS;
}

int do_dir(const char *location, char *params[]) {
  DIR *dir;
  struct dirent *entry;
  struct stat attr;

  /* open the directory */
  dir = opendir(location);

  if (!dir) {
    printf("opendir(%s): %s\n", location, strerror(errno));
    /* continue even if some entries fail */
  }

  while ((entry = readdir(dir))) {
    /* skip '.' and '..' */
    if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)) {
      continue;
    }

    /* allocate memory for the entry path */
    char *path = malloc(strlen(location) + strlen(entry->d_name) + 2);

    if (!path) {
      printf("malloc(): %s\n", strerror(errno));
      return EXIT_FAILURE;
    }

    /* concat the path with the entry name */
    sprintf(path, "%s/%s", location, entry->d_name);

    /* process the entry */
    do_file(path, params);

    /* if the entry is a directory, call the function recursively */
    if (!lstat(path, &attr)) {
      if (S_ISDIR(attr.st_mode)) {
        do_dir(path, params);
      }
    } else {
      printf("lstat(%s): %s\n", location, strerror(errno));
    }

    free(path);
  }

  if (closedir(dir)) {
    printf("closedir(%s): %s\n", location, strerror(errno));
  }

  return EXIT_SUCCESS;
}

int do_file(const char *location, char *params[]) {
  for (int i = 0; params[i]; i++) {
    if (!strcmp(params[i], "-print")) {
      printf("%s\n", location);
    }
  }

  return EXIT_SUCCESS;
}

int print_usage() {
  printf("myfind <file or directory> [ <aktion> ]\n"
         "-user <name>|<uid>\n"
         "-name <pattern>\n"
         "-type [bcdpfls]\n"
         "-print\n"
         "-ls\n"
         "-nouser\n"
         "-path\n");

  return EXIT_FAILURE;
}