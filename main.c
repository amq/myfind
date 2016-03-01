#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * why not "const char *path"?
 * because we are always passing the path by value
 *
 * why not "const char *params[]"?
 * the parameters argc and argv and the strings
 * pointed to by the argv array shall be modifiable
 * by the program, and retain their last-stored values
 * between program startup and program termination
 * C11 standard draft N1570, §5.1.2.2.1/2
 */

void print_usage(void);
int do_dir(char *path, char *params[]);
int do_file(char *path, char *params[]);
int do_print(char *path);
/*
int do_ls(char *path);
int do_nouser(char *path);
int do_path(char *path);
int do_user(char *path, char *user);
int do_name(char *path, char *name);
int do_type(char *path, char *type);
*/

/**
 * a global variable containing the program name
 * used for error messages,
 * spares us one argument for each subfunction
 */
char *program;

/**
 * @brief calls do_file on argv[1] and additionally do_dir if a directory
 *
 * @param argc number of arguments
 * @param argv the arguments
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int main(int argc, char *argv[]) {
  struct stat attr;

  /*
   * a minimum of 3 input parameters are required
   * myfind <file or directory> [ <aktion> ]
   */
  if (argc < 3) {
    print_usage();
    return EXIT_FAILURE;
  }

  program = argv[0];

  /*
   * try reading the attributes of the input
   * to verify that it exists and to check if it is a directory
   */
  if (lstat(argv[1], &attr) == 0) {
    /* process the input */
    if (do_file(argv[1], argv) != EXIT_SUCCESS) {
      return EXIT_FAILURE; /* the input didn't pass the parameter check */
    }

    /* if a directory, process its contents */
    if (S_ISDIR(attr.st_mode)) {
      do_dir(argv[1], argv);
    }
  } else {
    fprintf(stderr, "%s: lstat(%s): %s\n", program, argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief prints out the program usage
 *
 * @param void
 *
 * @retval void
 */
void print_usage(void) {

  if (printf("myfind <file or directory> [ <aktion> ]\n"
             "-user <name>|<uid>    entries belonging to a user\n"
             "-name <pattern>       entry names matching a pattern\n"
             "-type [bcdpfls]       entries of a specific type\n"
             "-print                print entries with paths\n"
             "-ls                   print entry details\n"
             "-nouser               entries not belonging to a user\n"
             "-path                 entry paths (incl. names) matching a pattern\n") < 0) {
    fprintf(stderr, "%s: printf() failed\n", program);
  }
}

/**
 * @brief calls do_file on each directory entry recursively
 *
 * @param path the path to be processed
 * @param params the arguments from argv
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_dir(char *path, char *params[]) {
  DIR *dir;
  struct dirent *entry;
  struct stat attr;

  dir = opendir(path);

  if (!dir) {
    fprintf(stderr, "%s: opendir(%s): %s\n", program, path, strerror(errno));
    return EXIT_FAILURE; /* stops a function call, the program continues */
  }

  while ((entry = readdir(dir))) {
    /* skip '.' and '..' */
    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
      continue;
    }

    /* allocate memory for the full entry path */
    char *full_path = malloc(sizeof(char) * (strlen(path) + strlen(entry->d_name) + 2));

    if (!full_path) {
      fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
      return EXIT_FAILURE;
    }

    /* concat the path with the entry name */
    sprintf(full_path, "%s/%s", path, entry->d_name);

    /* process the entry */
    do_file(full_path, params);

    /* if the entry is a directory, call the function recursively */
    if (lstat(full_path, &attr) == 0) {
      if (S_ISDIR(attr.st_mode)) {
        do_dir(full_path, params);
      }
    } else {
      fprintf(stderr, "%s: lstat(%s): %s\n", program, full_path, strerror(errno));
      free(full_path);
      return EXIT_FAILURE;
    }

    free(full_path);
  }

  if (closedir(dir) != 0) {
    fprintf(stderr, "%s: closedir(%s): %s\n", program, path, strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief calls a subfunction on the path based on a parameter match
 *
 * @param path the path to be processed
 * @param params the arguments from argv
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_file(char *path, char *params[]) {
  int i = 0; /* the counter variable is used outside of the loop */

  /*
   * 0 = ok or nothing to check
   * 1 = unknown predicate
   * 2 = missing argument for predicate
   * 3 = unknown argument for predicate
   */
  int status = 0;

  /* parameters start from params[2] */
  for (i = 2; params[i]; i++) {

    /* parameters consisting of a single part */
    if (strcmp(params[i], "-print") == 0) {
      do_print(path);
      continue;
    }
    if (strcmp(params[i], "-ls") == 0) {
      /* do_ls(path); */
      continue;
    }
    if (strcmp(params[i], "-nouser") == 0) {
      /* do_nouser(path); */
      continue;
    }
    if (strcmp(params[i], "-path") == 0) {
      /* do_path(path); */
      continue;
    }

    /* parameters expecting a non-empty second part */
    if (strcmp(params[i], "-user") == 0) {
      if (params[++i]) {
        /* do_user(path, params[i]); */
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }
    if (strcmp(params[i], "-name") == 0) {
      if (params[++i]) {
        /* do_name(path, params[i]); */
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }

    /* a parameter expecting a restricted second part */
    if (strcmp(params[i], "-type") == 0) {
      if (params[++i]) {
        if ((strcmp(params[i], "b") == 0) || (strcmp(params[i], "c") == 0) ||
            (strcmp(params[i], "d") == 0) || (strcmp(params[i], "p") == 0) ||
            (strcmp(params[i], "f") == 0) || (strcmp(params[i], "l") == 0) ||
            (strcmp(params[i], "s") == 0)) {
          /* do_type(path, params[i]); */
          continue;
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
    break;      /* do not increment the counter */
  }

  if (status == 1) {
    fprintf(stderr, "%s: unknown predicate: %s\n", program, params[i]);
    return EXIT_FAILURE;
  }

  if (status == 2) {
    fprintf(stderr, "%s: missing argument to %s\n", program, params[i - 1]);
    return EXIT_FAILURE;
  }

  if (status == 3) {
    fprintf(stderr, "%s: unknown argument to %s: %s\n", program, params[i - 1], params[i]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
 * @brief prints out the path
 *
 * @param path the path to be processed
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_print(char *path) {

  if (printf("%s\n", path) < 0) {
    fprintf(stderr, "%s: printf() failed\n", program);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
