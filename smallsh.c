#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h> 
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>


#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);

int* bg_pid;
int bg_pid_count = 0;
int last_bg = -1;

int last_status = 0;

void sigint_handler(int sig) {
  fprintf(stderr, "\n");
}

struct sigaction sigint_oldact;
struct sigaction sigtstp_oldact;
struct sigaction newact = { 0 };

int main(int argc, char *argv[]) {
  FILE *input = stdin;
  char *input_fn = "(stdin)";
  int interactive = 1; 
  int input_fd; 
  if (argc == 2) {
    input_fn = argv[1];
    input_fd = open(input_fn, O_RDONLY | O_CLOEXEC);
    if (input_fd < 0) {
      err(1, "%s", input_fn);
    }
    input = fdopen(input_fd, "r");
    interactive = 0; 
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  if (interactive) {
    /* In interactive mode */

    /* Save the original actions */
    sigaction(SIGINT, NULL, &sigint_oldact);
    sigaction(SIGTSTP, NULL, &sigtstp_oldact);

    /* Set a new SIGINT action */
    newact.sa_handler = SIG_IGN; 
    sigaction(SIGINT, &newact, NULL);

    /* Set a new SIGTSTP action */
    newact.sa_handler = SIG_IGN;  
    sigaction(SIGTSTP, &newact, NULL);
  }

  char *line = NULL;
  size_t n = 0;

  int size = 0;
  bg_pid = (int*) malloc(size * sizeof(int));

  for (;;) {
    /* 1. INPUT */
    /* Manage background processes */
    for (int i = 0; i < bg_pid_count; ++i) {
      if (bg_pid[i] != -1) {
        int status;
        pid_t result = waitpid(bg_pid[i], &status, WNOHANG | WUNTRACED);

        if (result == 0) {
          /* Child process is still running */
        } else if (result < 0) {
          /* Error occurred */
          perror("waitpid");
          last_status = 1;
        } else {
          /* Child process has terminated */
          if (WIFEXITED(status)) {
            fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t)bg_pid[i], WEXITSTATUS(status));
            bg_pid[i] = -1;
          } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t)bg_pid[i], WTERMSIG(status));
            bg_pid[i] = -1;
          } else if (WIFSTOPPED(status)) {
            /* Child process was stopped. */
            kill(bg_pid[i], SIGCONT); /* send SIGCONT to the pid to continue the process. */
            fprintf(stderr, "Child process %d stopped. Continuing.\n", bg_pid[i]);
          }
        }
      }
    }

    /* Remove any terminated background processes */
    int new_count = 0;
    for (int i = 0; i < bg_pid_count; ++i) {
      if (bg_pid[i] != -1) {
        bg_pid[new_count] = bg_pid[i];
        ++new_count;
      }
    }
    bg_pid_count = new_count;    

    /* Prompt */
    if (input == stdin) {
      char *ps1 = getenv("PS1");
      if (ps1 == NULL) {
          ps1 = "";
      }
      fprintf(stderr, "%s", ps1);
    }
    
    /* Read a line of input */
    newact.sa_handler = sigint_handler;
    sigaction(SIGINT, &newact, NULL); 

    ssize_t line_len = getline(&line, &n, input);

    newact.sa_handler = SIG_IGN;
    sigaction(SIGINT, &newact, NULL); 

    if (line_len < 0) {
      if (errno == EINTR) {
        clearerr(input);
        errno = 0;
        continue;
      } else if (feof(input)) {
        /* End of file (EOF) reached */
        break;
      } else {
        err(1, "%s", input_fn);
      }
    }


    /* 2. WORD SPLITTING */
    size_t nwords = wordsplit(line);
    if (nwords == 0) {
      continue;
    }
    for (size_t i = 0; i < nwords; ++i) {
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
    }
    words[nwords] = NULL; /* ensure words is null terminated*/


    /* 3. EXPANSION */
    /* Handled at bottom of the code */


    /* 4. PARSING */
    int background = 0; /* 0 = run in foreground, 1 = run in background */
    int redirection_indices[nwords];
    int num_of_redirections = 0;

    if (strcmp(words[nwords-1], "&") == 0 && nwords > 1) {
      background = 1;
      words[nwords-1] = NULL; /* remove the background operator */
    } else {
      /* Look for redirection operators and store their indices */
      for (int i = 0; i < nwords; ++i) {
        if (strcmp(words[i], ">") == 0 || strcmp(words[i], "<") == 0 || strcmp(words[i], ">>") == 0) {
            redirection_indices[num_of_redirections] = i;
            ++num_of_redirections;
        }
      }
    }


    /* 5. EXECUTION*/
    if (nwords > 0) {

      if (strcmp(words[0], "exit") == 0) {
        /* Handle exit */
        /* Check if a second argument is provided */
        if (nwords > 2) {
          fprintf(stderr, "exit: Too many arguments\n");
          last_status = 1;
          continue;
        } else if (nwords > 1) {
          char *num;
          long int temp_status = strtol(words[1], &num, 10);
          if (*num == '\0') {
            last_status = temp_status;
          } else {
            fprintf(stderr, "exit: Invalid argument\n");
            last_status = 1;
            continue;
          }
        }
        exit(last_status);

      } else if (strcmp(words[0], "cd") == 0) {
        /* Handle cd */
        /* Check if a second argument is provided */
        if (nwords > 2) {
          fprintf(stderr, "cd: Too many arguments\n");
          last_status = 1;
        } else {
          char *dir = nwords == 1 ? getenv("HOME") : words[1]; /* use HOME environment variable if no argument is provided */
          if (chdir(dir) != 0) {
            perror("cd failed");
            last_status = 1;
          } 
        }
        continue;

      } else {
        /* Handle non-built-in commands */
        pid_t pid = fork();
        if (pid < 0) {
          /* Fork failed */
          perror("fork");
          last_status = 1;
        } else if (pid == 0) {
          /* In child process */ 

          /* Restore the original signal handlers */
          sigaction(SIGINT, &sigint_oldact, NULL);
          sigaction(SIGTSTP, &sigtstp_oldact, NULL);

          for (int i = 0; i < num_of_redirections; ++i) {
              int redirection_index = redirection_indices[i];
              char *operator = words[redirection_index];

              /* Check if there is a following word */
              if (redirection_index + 1 >= nwords || words[redirection_index + 1] == NULL) {
                  fprintf(stderr, "Redirection error: No file specified\n");
                  exit(EXIT_FAILURE);
              }

              char *filename = words[redirection_index + 1];
              int fd;

              /* Handle redirection */
              if (strcmp(operator, ">") == 0) {
                  /* Write to file */
                  fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
              } else if (strcmp(operator, "<") == 0) {
                  /* Read from file */
                  fd = open(filename, O_RDONLY);
              } else if (strcmp(operator, ">>") == 0) {
                  /* Append to file */
                  fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
              }

              if (fd < 0) {
                  perror("Cannot open file for redirection");
                  exit(EXIT_FAILURE);
              }

              /* Redirect the appropriate file descriptor */
              if (strcmp(operator, ">") == 0) {
                  dup2(fd, STDOUT_FILENO);
              } else if (strcmp(operator, "<") == 0) {
                  dup2(fd, STDIN_FILENO);
              } else if (strcmp(operator, ">>") == 0) {
                  dup2(fd, STDOUT_FILENO);
              }

              close(fd);

              /* Remove the operator and its operand */
              words[redirection_index] = NULL;
              words[redirection_index + 1] = NULL;
          }
          execvp(words[0], words);
          perror("execvp");  /* execvp returns on error */
          exit(EXIT_FAILURE); 
        } else {
          if (background) {
            /* Don't wait for child process, run it in the background */
            bg_pid = realloc(bg_pid, sizeof(int) * (bg_pid_count + 1));
            bg_pid[bg_pid_count] = pid;
            ++bg_pid_count;
            bg_pid[bg_pid_count] = -1; /* mark the next element as -1 */
            last_bg = pid;
            
          } else {
            /* 6. WAITING */
            /* Wait for child to finish */
            int status;
            if (waitpid(pid, &status, WUNTRACED) < 0) {
              perror("waitpid");
              last_status = 1;
            } else if (WIFEXITED(status)) {
              last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
              last_status = 128 + WTERMSIG(status); 
            } else if (WIFSTOPPED(status)) {
              /* Child process stopped, send SIGCONT and run in background */
              fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) pid);
              kill(pid, SIGCONT);
              /* Run it in the background */
              bg_pid = realloc(bg_pid, sizeof(int) * (bg_pid_count + 1));
              bg_pid[bg_pid_count] = pid;
              ++bg_pid_count;
              bg_pid[bg_pid_count] = -1; /* mark the next element as -1 */
              last_bg = pid;          
            } else {
              last_status = 1;
            }
          }
        }
      }
    }

    /* Reset words array */
    for (size_t i = 0; i < nwords; ++i) {
      free(words[i]);
      words[i] = NULL;
    }

  }

  free(bg_pid);
  exit(last_status);
}


char *words[MAX_WORDS] = {0};

/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char const **start, char const **end)
{
  static char const *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = 0;
  *end = 0;
  for (char const *s = word; *s && !ret; ++s) {
    s = strchr(s, '$');
    if (!s) break;
    switch (s[1]) {
    case '$':
    case '!':
    case '?':
      ret = s[1];
      *start = s;
      *end = s + 2;
      break;
    case '{':;
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = s[1];
        *start = s;
        *end = e + 1;
      }
      break;
    }
  }
  prev = *end;
  return ret;
}


/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}


/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char const *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);

  while (c) {
    if (c == '!') {
      if (last_bg != -1) {
        char bgpid[10];
        sprintf(bgpid, "%d", last_bg);
        build_str(bgpid, NULL);
      } else {
        build_str("", NULL);
      }
    } 
    else if (c == '$') {
      char pid[10];
      sprintf(pid, "%d", getpid());
      build_str(pid, NULL);
    }
    else if (c == '?') {
      char status[10];
      sprintf(status, "%d", last_status); 
      build_str(status, NULL);
    }
    else if (c == '{') {
      char *param_name = strndup(start + 2, end - start - 3);
      char *param_value = getenv(param_name);
      if (param_value != NULL) {
        build_str(param_value, NULL);
      } else {
        build_str("", NULL);
      }
      free(param_name);
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}
