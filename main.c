#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/**
 * a linked list containing the parsed parameters
 * out of argv by do_parse_params
 */
typedef struct params_t {
  char *location;
  int help;
  int print;
  int ls;
  int nouser;
  char type;
  char *user;
  unsigned int userid;
  char *path;
  char *name;
  struct params_t *next;
} params_t;

void do_help(void);
int do_parse_params(int argc, char *argv[], params_t *params);

int do_location(params_t *params);
int do_file(char *path, params_t *params, struct stat attr);
int do_dir(char *path, params_t *params, struct stat attr);

int do_print(char *path);
int do_ls(char *path, struct stat attr);
int do_type(char type, struct stat attr);
int do_nouser(struct stat attr);
int do_user(unsigned int userid, struct stat attr);
int do_name(char *path, char *pattern);
int do_path(char *path, char *pattern);

char do_get_type(struct stat attr);
char *do_get_perms(struct stat attr);
char *do_get_user(struct stat attr);
char *do_get_group(struct stat attr);
char *do_get_mtime(struct stat attr);
char *do_get_symlink(char *path, struct stat attr);

/**
 * a global variable containing the program name
 * used for error messages,
 * spares us one argument for each subfunction
 */
char *program;

/**
 * @brief calls do_parse_params and do_location
 *
 * @param argc number of arguments
 * @param argv the arguments
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int main(int argc, char *argv[]) {
  params_t *params;

  program = argv[0];

  /* honor the system locale */
  if (!setlocale(LC_ALL, "")) {
    fprintf(stderr, "%s: setlocale() failed\n", program);
  }

  params = calloc(1, sizeof(*params));

  if (!params) {
    fprintf(stderr, "%s: calloc(): %s\n", program, strerror(errno));
    return EXIT_FAILURE;
  }

  if (do_parse_params(argc, argv, params) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  if (params->help) {
    do_help();
    return EXIT_SUCCESS;
  }

  if (do_location(params) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  /* params are freed by the program termination */

  return EXIT_SUCCESS;
}

/**
 * @brief prints out the program usage
 */
void do_help(void) {

  if (printf("usage:\n"
             "myfind [ <location> ] [ <aktion> ]\n"
             "-help               show this message\n"
             "-user <name>|<uid>  entries belonging to a user\n"
             "-name <pattern>     entry names matching a pattern\n"
             "-type [bcdpfls]     entries of a specific type\n"
             "-print              print entries with paths\n"
             "-ls                 print entry details\n"
             "-nouser             entries not belonging to a user\n"
             "-path               entry paths (incl. names) matching a pattern\n") < 0) {
    fprintf(stderr, "%s: printf(): %s\n", program, strerror(errno));
  }
}

/**
 * @brief parses argv and populates the params struct
 *
 * @param argc the number of arguments from argv
 * @param argv the arguments from argv
 * @param params the struct to populate
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_parse_params(int argc, char *argv[], params_t *params) {
  struct passwd *pwd;
  int i; /* used outside of the loop */

  /*
   * 0 = ok or nothing to check
   * 1 = unknown predicate
   * 2 = missing argument for predicate
   * 3 = unknown argument for predicate
   * 4 = unknown user
   * 5 = path after expression
   */
  int status = 0;
  int expression = 0;

  /* params can start from argv[1] */
  for (i = 1; i < argc; i++, params = params->next) {

    /* allocate memory for the next run */
    params->next = calloc(1, sizeof(*params));

    if (!params->next) {
      fprintf(stderr, "%s: calloc(): %s\n", program, strerror(errno));
      return EXIT_FAILURE;
    }

    /* parameters consisting of a single part */
    if (strcmp(argv[i], "-help") == 0) {
      params->help = 1;
      expression = 1;
      continue;
    }
    if (strcmp(argv[i], "-print") == 0) {
      params->print = 1;
      expression = 1;
      continue;
    }
    if (strcmp(argv[i], "-ls") == 0) {
      params->ls = 1;
      expression = 1;
      continue;
    }
    if (strcmp(argv[i], "-nouser") == 0) {
      params->nouser = 1;
      expression = 1;
      continue;
    }

    /* parameters expecting a non-empty second part */
    if (strcmp(argv[i], "-user") == 0) {
      if (argv[++i]) {
        params->user = argv[i];
        /* check if the user exists */
        if ((pwd = getpwnam(params->user))) {
          params->userid = pwd->pw_uid;
          expression = 1;
          continue;
        }
        /* otherwise, if the input is a number, use that */
        if (sscanf(params->user, "%u", &params->userid)) {
          expression = 1;
          continue;
        }
        status = 4;
        break; /* the user is not found and not a number */
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }
    if (strcmp(argv[i], "-name") == 0) {
      if (argv[++i]) {
        params->name = argv[i];
        expression = 1;
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }
    if (strcmp(argv[i], "-path") == 0) {
      if (argv[++i]) {
        params->path = argv[i];
        expression = 1;
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }

    /* a parameter expecting a restricted second part */
    if (strcmp(argv[i], "-type") == 0) {
      if (argv[++i]) {
        if ((strcmp(argv[i], "b") == 0) || (strcmp(argv[i], "c") == 0) ||
            (strcmp(argv[i], "d") == 0) || (strcmp(argv[i], "p") == 0) ||
            (strcmp(argv[i], "f") == 0) || (strcmp(argv[i], "l") == 0) ||
            (strcmp(argv[i], "s") == 0)) {
          params->type = argv[i][0];
          expression = 1;
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

    /*
     * there was no match;
     * if the parameter starts with '-', return an error,
     * else if there were no previous matches (expressions), save it as a location
     */
    if (argv[i][0] == '-') {
      status = 1;
      break;
    } else {
      if (expression == 0) {
        params->location = argv[i];
        continue;
      } else {
        status = 5;
        break;
      }
    }
  }

  /* error handling */
  if (status == 1) {
    fprintf(stderr, "%s: unknown predicate: `%s'\n", program, argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 2) {
    fprintf(stderr, "%s: missing argument to `%s'\n", program, argv[i - 1]);
    return EXIT_FAILURE;
  }
  if (status == 3) {
    fprintf(stderr, "%s: unknown argument to %s: %s\n", program, argv[i - 1], argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 4) {
    fprintf(stderr, "%s: `%s' is not the name of a known user\n", program, argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 5) {
    fprintf(stderr, "%s: paths must precede expression: %s\n", program, argv[i]);
    do_help();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief calls do_file and do_directory on locations from the params struct
 *
 * @param params the parsed parameters
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_location(params_t *params) {
  struct stat attr;
  char *location;

  do {
    location = params->location;

    if (!location) {
      location = ".";
    }

    /*
     * try reading the attributes of the location
     * to verify that it exists and to check if it is a directory
     */
    if (lstat(location, &attr) == 0) {
      /*
       * the returns are needed here
       * to forward the status of the location processing to main()
       * for example, ./myfind /nope -ls should return 1
       */
      if (do_file(location, params, attr) != EXIT_SUCCESS) {
        return EXIT_FAILURE; /* typically: printf failed */
      }

      /* if a directory, process its contents */
      if (S_ISDIR(attr.st_mode)) {
        if (do_dir(location, params, attr) != EXIT_SUCCESS) {
          return EXIT_FAILURE; /* typically: permission denied */
        }
      }
    } else {
      fprintf(stderr, "%s: lstat(%s): %s\n", program, location, strerror(errno));
      return EXIT_FAILURE; /* typically: no such file or directory */
    }

    params = params->next;
  } while (params && params->location);

  return EXIT_SUCCESS;
}

/**
 * @brief checks the entry using subfunctions based on params, if passed, prints it
 *
 * @param path the path to be processed
 * @param params the parsed parameters
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_file(char *path, params_t *params, struct stat attr) {
  int printed = 0;

  do {
    /* filtering */
    if (params->type) {
      if (do_type(params->type, attr) != EXIT_SUCCESS) {
        return EXIT_SUCCESS; /* the entry didn't pass the check, do not print it */
      }
    }
    if (params->nouser) {
      if (do_nouser(attr) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
      }
    }
    if (params->user) {
      if (do_user(params->userid, attr) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
      }
    }
    if (params->name) {
      if (do_name(path, params->name) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
      }
    }
    if (params->path) {
      if (do_path(path, params->path) != EXIT_SUCCESS) {
        return EXIT_SUCCESS;
      }
    }
    /* printing */
    if (params->print) {
      if (do_print(path) != EXIT_SUCCESS) {
        return EXIT_FAILURE; /* a fatal error occurred */
      }
      printed = 1;
    }
    if (params->ls) {
      if (do_ls(path, attr) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
      printed = 1;
    }
    params = params->next;
  } while (params);

  if (printed == 0) {
    if (do_print(path) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

/**
 * @brief calls do_file on each directory entry recursively
 *
 * @param path the path to be processed
 * @param params the parsed parameters
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_dir(char *path, params_t *params, struct stat attr) {
  DIR *dir;
  struct dirent *entry;
  size_t length;
  char *slash = "";
  char *full_path;

  dir = opendir(path);

  if (!dir) {
    fprintf(stderr, "%s: opendir(%s): %s\n", program, path, strerror(errno));
    return EXIT_FAILURE;
  }

  while ((entry = readdir(dir))) {
    /* skip '.' and '..' */
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    /* add a trailing slash if not present */
    length = strlen(path);
    if (path[length - 1] != '/') {
      slash = "/";
    }

    /* allocate memory for the full entry path */
    length = strlen(path) + strlen(entry->d_name) + 2;
    full_path = malloc(sizeof(char) * length);

    if (!full_path) {
      fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
      break; /* a return would require a closedir() */
    }

    /* concat the path with the entry name */
    if (snprintf(full_path, length, "%s%s%s", path, slash, entry->d_name) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program, strerror(errno));
      free(full_path);
      break;
    }

    /* process the entry */
    if (lstat(full_path, &attr) == 0) {
      /*
       * there are no returns for do_file and do_dir on purpose here;
       * it is normal for a single entry to fail, then we try the next one
       */
      do_file(full_path, params, attr);

      /* if a directory, call the function recursively */
      if (S_ISDIR(attr.st_mode)) {
        do_dir(full_path, params, attr);
      }
    } else {
      fprintf(stderr, "%s: lstat(%s): %s\n", program, full_path, strerror(errno));
      free(full_path);
      continue;
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
 * @brief prints out the path
 *
 * @param path the path to be processed
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_print(char *path) {

  if (printf("%s\n", path) < 0) {
    fprintf(stderr, "%s: printf(): %s\n", program, strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief prints out the path with details
 *
 * @param path the path to be processed
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_ls(char *path, struct stat attr) {
  unsigned long inode = attr.st_ino;
  long long blocks = S_ISLNK(attr.st_mode) ? 0 : attr.st_blocks / 2;
  char *perms = do_get_perms(attr);
  unsigned long links = attr.st_nlink;
  char *user = do_get_user(attr);
  char *group = do_get_group(attr);
  long long size = attr.st_size;
  char *mtime = do_get_mtime(attr);
  char *symlink = do_get_symlink(path, attr);
  char *arrow = symlink[0] ? " -> " : "";

  if (printf("%6lu %4lld %10s %3lu %-8s %-8s %8lld %12s %s%s%s\n", inode, blocks, perms, links,
             user, group, size, mtime, path, arrow, symlink) < 0) {
    fprintf(stderr, "%s: printf(): %s\n", program, strerror(errno));
    free(symlink);
    return EXIT_FAILURE;
  }

  free(symlink);

  return EXIT_SUCCESS;
}

/**
 * @brief checks if the type matches the entry attributes
 *
 * @param type the type to match against
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_type(char type, struct stat attr) {

  /* comparing two chars */
  if (type == do_get_type(attr)) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

/**
 * @brief checks if the entry doesn't have a user
 *
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_nouser(struct stat attr) {
  static unsigned int cache_uid = UINT_MAX;

  /* skip getgrgid if we have the record in cache */
  if (cache_uid == attr.st_uid) {
    return EXIT_FAILURE;
  }

  if (!getpwuid(attr.st_uid)) {
    return EXIT_SUCCESS;
  }

  /* cache an existing user (more common) */
  cache_uid = attr.st_uid;

  return EXIT_FAILURE;
}

/**
 * @brief checks if the userid matches the entry attribute
 *
 * @param userid the uid to match against
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_user(unsigned int userid, struct stat attr) {

  if (userid == attr.st_uid) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

/**
 * @brief checks if the filename matches the pattern
 *
 * @param path the entry path
 * @param pattern the pattern to match against
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_name(char *path, char *pattern) {
  char *filename = basename(path);
  int flags = 0;

  if (fnmatch(pattern, filename, flags) == 0) {
    return EXIT_SUCCESS;
  }

  /* basename manual: do not pass the returned pointer to free() */

  return EXIT_FAILURE;
}

/**
 * @brief checks if the path matches the pattern
 *
 * @param path the entry path
 * @param pattern the pattern to match against
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int do_path(char *path, char *pattern) {
  int flags = 0;

  if (fnmatch(pattern, path, flags) == 0) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

/**
 * @brief converts the entry attributes to a readable type
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the entry type as a char
 */
char do_get_type(struct stat attr) {

  /* block special file */
  if (S_ISBLK(attr.st_mode)) {
    return 'b';
  }
  /* character special file */
  if (S_ISCHR(attr.st_mode)) {
    return 'c';
  }
  /* directory */
  if (S_ISDIR(attr.st_mode)) {
    return 'd';
  }
  /* fifo (named pipe) */
  if (S_ISFIFO(attr.st_mode)) {
    return 'p';
  }
  /* regular file */
  if (S_ISREG(attr.st_mode)) {
    return 'f';
  }
  /* symbolic link */
  if (S_ISLNK(attr.st_mode)) {
    return 'l';
  }
  /* socket */
  if (S_ISSOCK(attr.st_mode)) {
    return 's';
  }

  /* some other file type */
  return '?';
}

/**
 * @brief converts the entry attributes to readable permissions
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the entry permissions as a string
 */
char *do_get_perms(struct stat attr) {
  static char perms[11];
  char type = do_get_type(attr);

  /*
   * cast is used to avoid the IDE warnings
   * about int possibly not fitting into char
   */
  perms[0] = (char)(type == 'f' ? '-' : type);
  perms[1] = (char)(attr.st_mode & S_IRUSR ? 'r' : '-');
  perms[2] = (char)(attr.st_mode & S_IWUSR ? 'w' : '-');
  perms[3] = (char)(attr.st_mode & S_ISUID ? (attr.st_mode & S_IXUSR ? 's' : 'S')
                                           : (attr.st_mode & S_IXUSR ? 'x' : '-'));
  perms[4] = (char)(attr.st_mode & S_IRGRP ? 'r' : '-');
  perms[5] = (char)(attr.st_mode & S_IWGRP ? 'w' : '-');
  perms[6] = (char)(attr.st_mode & S_ISGID ? (attr.st_mode & S_IXGRP ? 's' : 'S')
                                           : (attr.st_mode & S_IXGRP ? 'x' : '-'));
  perms[7] = (char)(attr.st_mode & S_IROTH ? 'r' : '-');
  perms[8] = (char)(attr.st_mode & S_IWOTH ? 'w' : '-');
  perms[9] = (char)(attr.st_mode & S_ISVTX ? (attr.st_mode & S_IXOTH ? 't' : 'T')
                                           : (attr.st_mode & S_IXOTH ? 'x' : '-'));
  perms[10] = '\0';

  return perms;
}

/**
 * @brief converts the entry attributes to username or, if not found, uid
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the username if getpwuid() worked, otherwise uid, as a string
 */
char *do_get_user(struct stat attr) {
  struct passwd *pwd;

  static unsigned int cache_uid = UINT_MAX;
  static char *cache_pw_name = NULL;

  /* skip getgrgid if we have the record in cache */
  if (cache_uid == attr.st_uid) {
    return cache_pw_name;
  }

  pwd = getpwuid(attr.st_uid);

  if (!pwd) {
    /*
     * the user is not found or getpwuid failed,
     * return the uid as a string then;
     * an unsigned int needs 10 chars
     */
    static char user[11];
    if (snprintf(user, 11, "%u", attr.st_uid) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program, strerror(errno));
      return "";
    }
    return user;
  }

  cache_uid = pwd->pw_uid;
  cache_pw_name = pwd->pw_name;

  /* getpwuid manual: do not pass the returned pointer to free() */

  return pwd->pw_name;
}

/**
 * @brief converts the entry attributes to groupname or, if not found, gid
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the groupname if getgrgid() worked, otherwise gid, as a string
 */
char *do_get_group(struct stat attr) {
  struct group *grp;

  static unsigned int cache_gid = UINT_MAX;
  static char *cache_gr_name = NULL;

  /* skip getgrgid if we have the record in cache */
  if (cache_gid == attr.st_gid) {
    return cache_gr_name;
  }

  grp = getgrgid(attr.st_gid);

  if (!grp) {
    /*
     * the group is not found or getgrgid failed,
     * return the gid as a string then;
     * an unsigned int needs 10 chars
     */
    static char group[11];
    if (snprintf(group, 11, "%u", attr.st_gid) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program, strerror(errno));
      return "";
    }
    return group;
  }

  cache_gid = grp->gr_gid;
  cache_gr_name = grp->gr_name;

  /* getgrgid manual: do not pass the returned pointer to free() */

  return grp->gr_name;
}

/**
 * @brief converts the entry attributes to a readable modification time
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the entry modification time as a string
 */
char *do_get_mtime(struct stat attr) {
  static char mtime[16]; /* 12 length + 3 special + null */
  char *format;

  time_t now = time(NULL);
  time_t six_months = 31556952 / 2; /* 365.2425 * 60 * 60 * 24 */
  struct tm *local_mtime = localtime(&attr.st_mtime);

  if (!local_mtime) {
    fprintf(stderr, "%s: localtime(): %s\n", program, strerror(errno));
    return "";
  }

  if ((now - six_months) < attr.st_mtime) {
    format = "%b %e %H:%M"; /* recent */
  } else {
    format = "%b %e  %Y"; /* older than 6 months */
  }

  if (strftime(mtime, sizeof(mtime), format, local_mtime) == 0) {
    fprintf(stderr, "%s: strftime(): %s\n", program, strerror(errno));
    return "";
  }

  mtime[15] = '\0';

  return mtime;
}

/**
 * @brief extracts the entry symlink
 *
 * @param path the entry path
 * @param attr the entry attributes from lstat
 *
 * @returns the entry symlink as a string
 */
char *do_get_symlink(char *path, struct stat attr) {

  if (S_ISLNK(attr.st_mode)) {
    /*
     * st_size appears to be an unreliable source of the link length
     * PATH_MAX is artificial and not used by the GNU C Library
     */
    ssize_t length;
    size_t buffer = 128;
    char *symlink = malloc(sizeof(char) * buffer);

    if (!symlink) {
      fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
      return strdup("");
    }

    /*
     * if readlink() fills the buffer, double it and run again
     * even if it equals, because we need a character for the termination;
     * a check for > 0 is mandatory, because we are comparing signed and unsigned
     */
    while ((length = readlink(path, symlink, buffer)) > 0 && (size_t)length >= buffer) {
      buffer *= 2;
      char *new_symlink = realloc(symlink, buffer);

      if (!new_symlink) {
        fprintf(stderr, "%s: realloc(): %s\n", program, strerror(errno));
        free(symlink); /* realloc doesn't free the old object if it fails */
        return strdup("");
      }

      symlink = new_symlink;
    }

    if (length < 0) {
      fprintf(stderr, "%s: readlink(%s): %s\n", program, path, strerror(errno));
      free(symlink);
      return strdup("");
    }

    symlink[length] = '\0';
    return symlink;
  }

  /*
   * the entry is not a symlink
   * strdup is needed to keep the output free-able
   */
  return strdup("");
}
