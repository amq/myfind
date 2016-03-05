#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>

typedef struct actions_t {
  int print;
  int ls;
  int nouser;
  char type;
  char *user;
  char *path;
  char *name;
} actions_t;

void do_print_usage(void);
int do_parse_params(char *argv[], actions_t *actions);

int do_dir(char *path, const actions_t *actions, struct stat *attr);
int do_file(char *path, const actions_t *actions, struct stat *attr);

int do_print(char *path);
int do_ls(char *path, const struct stat *attr);
int do_type(char type, const struct stat *attr);
int do_nouser(const struct stat *attr);
int do_user(char *user, const struct stat *attr);
int do_path(char *path, char *pattern);
int do_name(char *path, char *pattern);

char do_get_type(const struct stat *attr);
char *do_get_perms(const struct stat *attr);
char *do_get_mtime(const struct stat *attr);
char *do_get_symlink(char *path, const struct stat *attr);
char *do_get_user(const struct stat *attr);
char *do_get_group(const struct stat *attr);

/**
 * a global variable containing the program name
 * used for error messages,
 * spares us one argument for each subfunction
 */
char *program;

/**
 * @brief calls do_file on argv[1] and additionally do_dir, if a directory
 *
 * @param argc number of arguments
 * @param argv the arguments
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int main(int argc, char *argv[]) {
  actions_t actions;
  struct stat attr;

  /*
   * a minimum of 3 input parameters are required
   * myfind <file or directory> [ <aktion> ]
   */
  if (argc < 3) {
    do_print_usage();
    return EXIT_FAILURE;
  }

  program = argv[0];

  if (do_parse_params(argv, &actions) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  /*
   * try reading the attributes of the input
   * to verify that it exists and to check if it is a directory
   */
  if (lstat(argv[1], &attr) == 0) {
    /* process the input */
    do_file(argv[1], &actions, &attr);

    /* if a directory, process its contents */
    if (S_ISDIR(attr.st_mode)) {
      do_dir(argv[1], &actions, &attr);
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
void do_print_usage(void) {

  if (printf("myfind <file or directory> [ <aktion> ]\n"
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
 * @brief parses params and populates the actions struct
 *
 * @param params the arguments from argv
 * @param params the struct to populate
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_parse_params(char *argv[], actions_t *actions) {
  int i = 0; /* the counter variable is used outside of the loop */

  /* initialize the actions with zeroes */
  memset(actions, 0, sizeof(*actions));

  /*
   * 0 = ok or nothing to check
   * 1 = unknown predicate
   * 2 = missing argument for predicate
   * 3 = unknown argument for predicate
   */
  int status = 0;

  /* parameters start from argv[2] */
  for (i = 2; argv[i]; i++) {

    /* parameters consisting of a single part */
    if (strcmp(argv[i], "-print") == 0) {
      actions->print = 1;
      continue;
    }
    if (strcmp(argv[i], "-ls") == 0) {
      actions->ls = 1;
      continue;
    }
    if (strcmp(argv[i], "-nouser") == 0) {
      actions->nouser = 1;
      continue;
    }

    /* parameters expecting a non-empty second part */
    if (strcmp(argv[i], "-user") == 0) {
      if (argv[++i]) {
        actions->user = argv[i];
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }
    if (strcmp(argv[i], "-name") == 0) {
      if (argv[++i]) {
        actions->name = argv[i];
        continue;
      } else {
        status = 2;
        break; /* the second part is missing */
      }
    }
    if (strcmp(argv[i], "-path") == 0) {
      if (argv[++i]) {
        actions->path = argv[i];
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
          actions->type = argv[i][0];
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
    break;      /* do not increment the counter,
                   so that we can access the current state in error handling */
  }

  /* error handling */
  if (status == 1) {
    fprintf(stderr, "%s: unknown predicate: %s\n", program, argv[i]);
    return EXIT_FAILURE;
  }
  if (status == 2) {
    fprintf(stderr, "%s: missing argument to %s\n", program, argv[i - 1]);
    return EXIT_FAILURE;
  }
  if (status == 3) {
    fprintf(stderr, "%s: unknown argument to %s: %s\n", program, argv[i - 1], argv[i]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief calls do_file on each directory entry recursively
 *
 * @param path the path to be processed
 * @param actions the parsed parameters
 * @param attr the entry attributes from lstat
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_dir(char *path, const actions_t *actions, struct stat *attr) {
  DIR *dir;
  struct dirent *entry;
  size_t length;

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

    /* avoid '//' in path if the input has a trailing slash */
    length = strlen(path);
    if (path[length - 1] == '/') {
      path[length - 1] = '\0';
    }

    /* allocate memory for the full entry path */
    length = strlen(path) + strlen(entry->d_name) + 2;
    char *full_path = malloc(sizeof(char) * length);

    if (!full_path) {
      fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
      return EXIT_FAILURE;
    }

    /* concat the path with the entry name */
    if (snprintf(full_path, length, "%s/%s", path, entry->d_name) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program, strerror(errno));
      return EXIT_FAILURE;
    }

    /* process the entry */
    if (lstat(full_path, attr) == 0) {
      do_file(full_path, actions, attr);

      /* if a directory, call the function recursively */
      if (S_ISDIR(attr->st_mode)) {
        do_dir(full_path, actions, attr);
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
 * @brief calls subfunctions based on the actions struct
 *
 * @param path the path to be processed
 * @param actions the parsed parameters
 * @param attr the entry attributes from lstat
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_file(char *path, const actions_t *actions, struct stat *attr) {

  /* filtering */
  if (actions->nouser && do_nouser(attr) != EXIT_SUCCESS) {
    return EXIT_SUCCESS;
  }
  if (actions->user && do_user(actions->user, attr) != EXIT_SUCCESS) {
    return EXIT_SUCCESS;
  }
  if (actions->name && do_name(path, actions->name) != EXIT_SUCCESS) {
    return EXIT_SUCCESS;
  }
  if (actions->path && do_path(path, actions->path) != EXIT_SUCCESS) {
    return EXIT_SUCCESS;
  }
  if (actions->type && do_type(actions->type, attr) != EXIT_SUCCESS) {
    return EXIT_SUCCESS;
  }

  /* printing */
  if ((!actions->ls || actions->print) && do_print(path) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (actions->ls && do_ls(path, attr) != EXIT_SUCCESS) {
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
    fprintf(stderr, "%s: printf(): %s\n", program, strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
 * @brief prints out the path with details
 *
 * @param path the path to be processed
 * @param attr the entry attributes from lstat
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_ls(char *path, const struct stat *attr) {
  long inode = attr->st_ino;
  long long blocks = attr->st_blocks / 2;
  char *perms = do_get_perms(attr);
  long links = attr->st_nlink;
  char *user = do_get_user(attr);
  char *group = do_get_group(attr);
  long long size = attr->st_size;
  char *mtime = do_get_mtime(attr);
  char *symlink = do_get_symlink(path, attr);
  int symlink_present = 0;
  char *arrow = "";

  if (strlen(symlink) > 0) {
    symlink_present = 1;
    arrow = " -> ";
  }

  if (printf("%-6ld %4lld %10s %3ld %-8s %-8s %8lld %12s %s%s%s\n", inode, blocks, perms, links,
             user, group, size, mtime, path, arrow, symlink) < 0) {
    fprintf(stderr, "%s: printf(): %s\n", program, strerror(errno));
    free(user);
    free(group);
    if (symlink_present) {
      free(symlink);
    }
    return EXIT_FAILURE;
  }

  free(user);
  free(group);
  if (symlink_present) {
    free(symlink);
  }

  return EXIT_SUCCESS;
}

/*
 * @brief returns EXIT_SUCCESS when the type matches the entry attribute
 *
 * @param type the type to match against
 * @param attr the entry attributes from lstat
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_type(char type, const struct stat *attr) {

  /* comparing two chars */
  if (type == do_get_type(attr)) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

/*
 * @brief returns EXIT_SUCCESS if the entry doesn't have a user
 *
 * @param attr the entry attributes from lstat
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_nouser(const struct stat *attr) {

  /* amanf */

  return EXIT_SUCCESS;
}

/*
 * @brief returns EXIT_SUCCESS if the user(-name/-id) matches the entry attribute
 *
 * @param user the username or uid to match against
 * @param attr the entry attributes from lstat
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_user(char *user, const struct stat *attr) {

  /* amanf */
  /* see also do_get_username */

  return EXIT_SUCCESS;
}

/*
 * @brief returns EXIT_SUCCESS if the path matches the pattern
 *
 * @param path the entry path
 * @param pattern the pattern to match against
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_path(char *path, char *pattern) {

  /* khalikov */

  return EXIT_SUCCESS;
}

/*
 * @brief returns EXIT_SUCCESS if the filename matches the pattern
 *
 * @param path the entry path
 * @param pattern the pattern to match against
 *
 * @retval EXIT_SUCCESS
 * @retval EXIT_FAILURE
 */
int do_name(char *path, char *pattern) {
  char *filename = basename(path);

  /* khalikov */

  /*
   * we are not calling free() on the filename; from the basename manual:
   * both dirname() and basename() return pointers to null-terminated strings,
   * do not pass these pointers to free()
   */

  return EXIT_SUCCESS;
}

/*
 * @brief returns the entry permissions
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the entry permissions as a string
 */
char *do_get_perms(const struct stat *attr) {
  static char perms[11];
  char type = do_get_type(attr);

  /*
   * cast is used to avoid the IDE warnings
   * about int possibly not fitting into char
   */
  perms[0] = (char)(type == 'f' ? '-' : type);
  perms[1] = (char)(attr->st_mode & S_IRUSR ? 'r' : '-');
  perms[2] = (char)(attr->st_mode & S_IWUSR ? 'w' : '-');
  perms[3] = (char)(attr->st_mode & S_ISUID ? (attr->st_mode & S_IXUSR ? 's' : 'S')
                                           : (attr->st_mode & S_IXUSR ? 'x' : '-'));
  perms[4] = (char)(attr->st_mode & S_IRGRP ? 'r' : '-');
  perms[5] = (char)(attr->st_mode & S_IWGRP ? 'w' : '-');
  perms[6] = (char)(attr->st_mode & S_ISGID ? (attr->st_mode & S_IXGRP ? 's' : 'S')
                                           : (attr->st_mode & S_IXGRP ? 'x' : '-'));
  perms[7] = (char)(attr->st_mode & S_IROTH ? 'r' : '-');
  perms[8] = (char)(attr->st_mode & S_IWOTH ? 'w' : '-');
  perms[9] = (char)(attr->st_mode & S_ISVTX ? (attr->st_mode & S_IXOTH ? 't' : 'T')
                                           : (attr->st_mode & S_IXOTH ? 'x' : '-'));
  perms[10] = '\0';

  return perms;
}

/*
 * @brief returns the entry type
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the entry type as a char
 * @retval b block special file
 * @retval c character special file
 * @retval d directory
 * @retval p fifo (named pipe)
 * @retval f regular file
 * @retval l symbolic link
 * @retval s socket
 * @retval ? some other file type
 */
char do_get_type(const struct stat *attr) {

  if (S_ISBLK(attr->st_mode)) {
    return 'b';
  }
  if (S_ISCHR(attr->st_mode)) {
    return 'c';
  }
  if (S_ISDIR(attr->st_mode)) {
    return 'd';
  }
  if (S_ISFIFO(attr->st_mode)) {
    return 'p';
  }
  if (S_ISREG(attr->st_mode)) {
    return 'f';
  }
  if (S_ISLNK(attr->st_mode)) {
    return 'l';
  }
  if (S_ISSOCK(attr->st_mode)) {
    return 's';
  }

  return '?';
}

/*
 * @brief returns the entry modification time
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the entry modification time as a string
 */
char *do_get_mtime(const struct stat *attr) {
  static char mtime[13];
  char *format;
  time_t now = time(NULL);
  time_t six_months = 31556952 / 2;

  if ((now - six_months) < attr->st_mtime) {
    format = "%b %e %H:%M"; /* recent */
  } else {
    format = "%b %e  %Y"; /* older than 6 months */
  }

  if (strftime(mtime, sizeof(mtime), format, localtime(&attr->st_mtime)) == 0) {
    fprintf(stderr, "%s: strftime() failed\n", program);
  }

  mtime[12] = '\0';

  return mtime;
}

/*
 * @brief returns the username or uid, if user not present on the system
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the username if getpwuid() worked, otherwise uid, as a string
 */
char *do_get_user(const struct stat *attr) {
  char *user;
  char *buffer;
  struct passwd pwd;
  struct passwd *result;
  long sysconf_length;
  size_t length;

  sysconf_length = sysconf(_SC_GETPW_R_SIZE_MAX);

  if (sysconf_length == -1) {
    sysconf_length = 16384;
  }

  length = (size_t)sysconf_length;

  buffer = malloc(length);

  if (!buffer) {
    fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
    return "";
  }

  if (getpwuid_r(attr->st_uid, &pwd, buffer, length, &result) != 0) {
    free(buffer);
    fprintf(stderr, "%s: getpwuid_r(): %s\n", program, strerror(errno));
    return "";
  }

  if (result) {
    user = pwd.pw_name;
  } else {
    user = buffer;
    if (snprintf(user, length, "%ld", (long)attr->st_uid) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program, strerror(errno));
      return "";
    }
  }

  return user;
}

/*
 * @brief returns the groupname or gid, if group not present on the system
 *
 * @param attr the entry attributes from lstat
 *
 * @returns the groupname if getgrgid() worked, otherwise gid, as a string
 */
char *do_get_group(const struct stat *attr) {
  char *group;
  char *buffer;
  struct group grp;
  struct group *result;
  long sysconf_length;
  size_t length;

  sysconf_length = sysconf(_SC_GETPW_R_SIZE_MAX);

  if (sysconf_length == -1) {
    sysconf_length = 16384;
  }

  length = (size_t)sysconf_length;

  buffer = malloc(length);

  if (!buffer) {
    fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
    return "";
  }

  if (getgrgid_r(attr->st_gid, &grp, buffer, length, &result) != 0) {
    free(buffer);
    fprintf(stderr, "%s: getgrgid_r(): %s\n", program, strerror(errno));
    return "";
  }

  if (result) {
    group = grp.gr_name;
  } else {
    group = buffer;
    if (snprintf(group, length, "%ld", (long)attr->st_uid) < 0) {
      fprintf(stderr, "%s: snprintf(): %s\n", program, strerror(errno));
      return "";
    }
  }

  return group;
}

/*
 * @brief returns the entry symlink, if entry of type s
 *
 * @param path the entry path
 * @param attr the entry attributes from lstat
 *
 * @returns the entry symlink as a string
 */
char *do_get_symlink(char *path, const struct stat *attr) {

  if (S_ISLNK(attr->st_mode)) {
    char *symlink = malloc(sizeof(char) * (attr->st_size + 2));

    if (!symlink) {
      fprintf(stderr, "%s: malloc(): %s\n", program, strerror(errno));
      return "";
    }

    if (readlink(path, symlink, sizeof(symlink) - 1) == -1) {
      fprintf(stderr, "%s: readlink(%s): %s\n", program, path, strerror(errno));
      free(symlink);
      return "";
    } else {
      symlink[attr->st_size] = '\0';
    }

    return symlink;
  }

  return "";
}
