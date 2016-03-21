#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/**
 * a linked list containing the parsed parameters
 */
typedef struct params_s {
  int print;
  int ls;
  int nouser;
  char type;
  char *user;
  unsigned int userid;
  char *path;
  char *name;
  struct params_s *next;
} params_t;

void do_help(void);
int do_parse_params(int argc, char *argv[], params_t *params);
int do_free_params(params_t *params);

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
 * used for error messages
 */
char *program_name;

/**
 * @brief entry point
 *
 * @param argc number of arguments
 * @param argv the arguments
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int main(int argc, char *argv[]) {
  params_t *params;
  struct stat attr;

  program_name = argv[0];

  if (argc < 2) {
    do_help();
    return EXIT_FAILURE;
  }

  params = calloc(1, sizeof(*params));

  if (!params) {
    fprintf(stderr, "%s: calloc(): %s\n", program_name, strerror(errno));
    return EXIT_FAILURE;
  }

  if (do_parse_params(argc, argv, params) != EXIT_SUCCESS) {
    do_free_params(params);
    return EXIT_FAILURE;
  }

  char *path = argv[1];

  if (lstat(path, &attr) == 0) {
    do_file(path, params, attr);

    if (S_ISDIR(attr.st_mode)) {
      do_dir(path, params, attr);
    }
  } else {
    fprintf(stderr, "%s: lstat(%s): %s\n", program_name, path, strerror(errno));
    return EXIT_FAILURE;
  }

  do_free_params(params);

  return EXIT_SUCCESS;
}

/**
 * @brief prints out the program usage
 */
void do_help(void) {

  if (printf("Usage:\n"
             "myfind [ <location> ] [ <aktion> ]\n"
             "-user <name>|<uid>  entries belonging to a user\n"
             "-name <pattern>     entry names matching a pattern\n"
             "-type [bcdpfls]     entries of a specific type\n"
             "-print              print entries with paths\n"
             "-ls                 print entry details\n"
             "-nouser             entries not belonging to a user\n"
             "-path               entry paths (incl. names) matching a pattern\n") < 0) {
    fprintf(stderr, "%s: printf(): %s\n", program_name, strerror(errno));
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

  /* params can start from argv[2] */
  for (i = 2; i < argc; i++, params = params->next) {

    /* allocate memory for the next run */
    params->next = calloc(1, sizeof(*params));

    if (!params->next) {
      fprintf(stderr, "%s: calloc(): %s\n", program_name, strerror(errno));
      return EXIT_FAILURE;
    }

    /* parameters consisting of a single part */
    if (strcmp(argv[i], "-print") == 0) {
      params->print = 1;
      continue;
    }
    if (strcmp(argv[i], "-ls") == 0) {
      params->ls = 1;
      continue;
    }
    if (strcmp(argv[i], "-nouser") == 0) {
      params->nouser = 1;
      continue;
    }

    /* parameters expecting a non-empty second part */
    if (strcmp(argv[i], "-user") == 0) {
      if (argv[++i]) {
        params->user = argv[i];
        /* check if the user exists */
        if ((pwd = getpwnam(params->user))) {
          params->userid = pwd->pw_uid;
          continue;
        }
        /* otherwise, if the input is a number, use that */
        if (sscanf(params->user, "%u", &params->userid)) {
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
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }
    if (strcmp(argv[i], "-path") == 0) {
      if (argv[++i]) {
        params->path = argv[i];
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

    /* there was no match */
    if (argv[i][0] == '-') {
      status = 1;
      break;
    } else {
      status = 5;
      break;
    }
  }

  /* error handling */
  if (status == 1) {
    fprintf(stderr, "%s: unknown predicate: `%s'\n", program_name, argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 2) {
    fprintf(stderr, "%s: missing argument to `%s'\n", program_name, argv[i - 1]);
    return EXIT_FAILURE;
  }
  if (status == 3) {
    fprintf(stderr, "%s: unknown argument to %s: %s\n", program_name, argv[i - 1], argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 4) {
    fprintf(stderr, "%s: `%s' is not the name of a known user\n", program_name, argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 5) {
    fprintf(stderr, "%s: path must precede expression: %s\n", program_name, argv[i]);
    fprintf(stderr, "Usage: %s [ <location> ] [ <aktion> ]\n", program_name);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief frees the params linked list
 *
 * @param params the parsed parameters
 *
 * @returns EXIT_SUCCESS
 */
int do_free_params(params_t *params) {

  while (params) {
    params_t *next = params->next;
    free(params);
    params = next;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief checks the entry using subfunctions based on params, if passed, prints it
 *
 * @param path the path to be processed
 * @param params the parsed parameters
 * @param attr the entry attributes from lstat
 *
 * @returns EXIT_SUCCESS
 */
int do_file(char *path, params_t *params, struct stat attr) {
  int printed = 0;

  do {
    /* filtering */
    if (params->type && do_type(params->type, attr) != EXIT_SUCCESS) {
      return EXIT_SUCCESS; /* the entry didn't pass the check, do not print it */
    }
    if (params->nouser && do_nouser(attr) != EXIT_SUCCESS) {
      return EXIT_SUCCESS;
    }
    if (params->user && do_user(params->userid, attr) != EXIT_SUCCESS) {
      return EXIT_SUCCESS;
    }
    if (params->name && do_name(path, params->name) != EXIT_SUCCESS) {
      return EXIT_SUCCESS;
    }
    if (params->path && do_path(path, params->path) != EXIT_SUCCESS) {
      return EXIT_SUCCESS;
    }
    /* printing */
    if (params->print) {
      do_print(path);
      printed = 1;
    }
    if (params->ls) {
      do_ls(path, attr);
      printed = 1;
    }
    params = params->next;
  } while (params);

  if (printed == 0) {
    do_print(path);
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
  DIR *dir = opendir(path);
  struct dirent *entry;

  if (!dir) {
    fprintf(stderr, "%s: opendir(%s): %s\n", program_name, path, strerror(errno));
    return EXIT_FAILURE;
  }

  while ((entry = readdir(dir))) {
    size_t length = strlen(path);

    /* skip '.' and '..' */
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    /* add a trailing slash if not present */
    char *slash = (path[length - 1] != '/') ? "/" : "";

    /* full entry path */
    length += strlen(entry->d_name) + 2;
    char full_path[length];

    /* concat the path with the entry name */
    if (snprintf(full_path, length, "%s%s%s", path, slash, entry->d_name) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program_name, strerror(errno));
      break;
    }

    if (lstat(full_path, &attr) == 0) {
      do_file(full_path, params, attr);

      if (S_ISDIR(attr.st_mode)) {
        do_dir(full_path, params, attr);
      }
    } else {
      fprintf(stderr, "%s: lstat(%s): %s\n", program_name, full_path, strerror(errno));
      continue;
    }
  }

  if (closedir(dir) != 0) {
    fprintf(stderr, "%s: closedir(%s): %s\n", program_name, path, strerror(errno));
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
    fprintf(stderr, "%s: printf(): %s\n", program_name, strerror(errno));
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
    fprintf(stderr, "%s: printf(): %s\n", program_name, strerror(errno));
    return EXIT_FAILURE;
  }

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

  if (!getpwuid(attr.st_uid)) {
    return EXIT_SUCCESS;
  }

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

  if (fnmatch(pattern, basename(path), 0) == 0) {
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

  if (fnmatch(pattern, path, 0) == 0) {
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

  if (S_ISBLK(attr.st_mode)) {
    return 'b';
  }
  if (S_ISCHR(attr.st_mode)) {
    return 'c';
  }
  if (S_ISDIR(attr.st_mode)) {
    return 'd';
  }
  if (S_ISFIFO(attr.st_mode)) {
    return 'p';
  }
  if (S_ISREG(attr.st_mode)) {
    return 'f';
  }
  if (S_ISLNK(attr.st_mode)) {
    return 'l';
  }
  if (S_ISSOCK(attr.st_mode)) {
    return 's';
  }

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

  perms[0] = type == 'f' ? '-' : type;
  perms[1] = attr.st_mode & S_IRUSR ? 'r' : '-';
  perms[2] = attr.st_mode & S_IWUSR ? 'w' : '-';
  perms[3] = attr.st_mode & S_ISUID ? (attr.st_mode & S_IXUSR ? 's' : 'S')
                                    : (attr.st_mode & S_IXUSR ? 'x' : '-');
  perms[4] = attr.st_mode & S_IRGRP ? 'r' : '-';
  perms[5] = attr.st_mode & S_IWGRP ? 'w' : '-';
  perms[6] = attr.st_mode & S_ISGID ? (attr.st_mode & S_IXGRP ? 's' : 'S')
                                    : (attr.st_mode & S_IXGRP ? 'x' : '-');
  perms[7] = attr.st_mode & S_IROTH ? 'r' : '-';
  perms[8] = attr.st_mode & S_IWOTH ? 'w' : '-';
  perms[9] = attr.st_mode & S_ISVTX ? (attr.st_mode & S_IXOTH ? 't' : 'T')
                                    : (attr.st_mode & S_IXOTH ? 'x' : '-');
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
  struct passwd *pwd = getpwuid(attr.st_uid);

  if (!pwd) {
    static char user[11];
    if (snprintf(user, 11, "%u", attr.st_uid) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program_name, strerror(errno));
      return "";
    }
    return user;
  }

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
  struct group *grp = getgrgid(attr.st_gid);

  if (!grp) {
    static char group[11];
    if (snprintf(group, 11, "%u", attr.st_gid) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program_name, strerror(errno));
      return "";
    }
    return group;
  }

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

  struct tm *local_mtime = localtime(&attr.st_mtime);

  if (!local_mtime) {
    fprintf(stderr, "%s: localtime(): %s\n", program_name, strerror(errno));
    return "";
  }

  if (strftime(mtime, sizeof(mtime), "%b %e %H:%M", local_mtime) == 0) {
    fprintf(stderr, "%s: strftime(): %s\n", program_name, strerror(errno));
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
    static char symlink[PATH_MAX];

    if (readlink(path, symlink, sizeof(symlink) - 1) < 0) {
      fprintf(stderr, "%s: readlink(%s): %s\n", program_name, path, strerror(errno));
      return "";
    }
    return symlink;
  }

  return "";
}
