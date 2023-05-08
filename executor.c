/* Name: Austin Sankanung */
/* UID: 117872163 */
/* Directory ID: acsan */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include "command.h"
#include "executor.h"

#define FILE_PER 0664

static int execute_helper(struct tree *t, int in_fd, int out_fd);
/* static void print_tree(struct tree *t); */

int execute(struct tree *t) {

   if(t != NULL){
      return execute_helper(t, STDIN_FILENO, STDOUT_FILENO);
   }
   return -1;

}

static int execute_helper(struct tree *t, int in_fd, int out_fd){

   pid_t fd;
   int status;
   int AND_status;
   pid_t pipe1;
   int pipe_fd[2];
   pid_t subshell_fd;

   if(t->conjunction == NONE){
      if(strcmp(t->argv[0], "cd") == 0){
         if(strcmp(t->argv[1], "") == 0 || t->argv[1] == NULL){
            if(chdir(getenv("HOME")) == -1){
               perror("HOME");
            }
         } else {
            if(chdir(t->argv[1]) == -1){
               perror(t->argv[1]);
            }
         }
      } else if(strcmp(t->argv[0], "exit") == 0){
         exit(0);
      } else {
         fd = fork();
         if(fd == -1){
            perror("fork");
         } else if(fd != 0){
            wait(&status);
            return status;
         } else {
            if(t->input != NULL){
               in_fd = open(t->input, O_RDONLY);
               if(in_fd == -1){
                  perror("open");
               }
               if(dup2(in_fd, STDIN_FILENO) == -1){
                  perror("dup2");
               }
               close(in_fd);
            }

            if(t->output != NULL){
               out_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, FILE_PER);
               if(out_fd == -1){
                  perror("open");
               }
               if(dup2(out_fd, STDOUT_FILENO) == -1){
                  perror("dup2");
               }
               close(out_fd);
            }
            execvp(t->argv[0], t->argv);
            fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
            exit(-1);
         }
      }

   } else if(t->conjunction == AND){

      if(t->input != NULL){
         in_fd = open(t->input, O_RDONLY);
         if(in_fd == -1){
            perror("open");
         }
         if(dup2(in_fd, STDIN_FILENO) == -1){
            perror("dup2");
         }
         close(in_fd);
      }

      if(t->output != NULL){
         out_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, FILE_PER);
         if(out_fd == -1){
            perror("open");
         }
         if(dup2(out_fd, STDOUT_FILENO) == -1){
            perror("dup2");
         }
         close(out_fd);
      }

      AND_status = execute_helper(t->left, in_fd, out_fd);
      if(AND_status == 0){
         execute_helper(t->right, in_fd, out_fd);
      } else {
         /* EXIT WITH LEFT COMMAND'S STATUS */
         return -1;
      }

   } else if(t->conjunction == SUBSHELL){

      if(t->input != NULL){
         in_fd = open(t->input, O_RDONLY);
         if(in_fd == -1){
            perror("open");
         }
      }
      if(t->output != NULL){
         out_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, FILE_PER);
         if(out_fd == -1){
            perror("open");
         }
      }
      
      subshell_fd = fork();
      if(subshell_fd == -1){
         perror("fork");
      } else if(subshell_fd != 0){
         wait(NULL);
      } else {
         execute_helper(t->left, in_fd, out_fd);
      }
      exit(0);

   } else if(t->conjunction == PIPE){

      if(t->left->output != 0){
         printf("Ambiguous output redirect.\n");
         return -1;
      }
      if(t->left->output != 0){
         printf("Ambiguous input redirect.\n");
         return -1;
      }

      if(t->input != NULL){
         in_fd = open(t->input, O_RDONLY);
         if(in_fd == -1){
            perror("open");
         }
      }
      if(t->output != NULL){
         out_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, FILE_PER);
         if(out_fd == -1){
            perror("open");
         }
      }

      if(pipe(pipe_fd) == -1){
         perror("pipe");
      }
      
      pipe1 = fork();
      if(pipe1 == -1){
         perror("fork");
      }

      if(pipe1 != 0){   /* Parent code for input */
         wait(NULL);
         close(pipe_fd[1]);
         if(dup2(pipe_fd[0], STDIN_FILENO) == -1){
            perror("dup2");
         }
         execute_helper(t->right, pipe_fd[0], out_fd);
         close(pipe_fd[0]);
         

      } else {          /* Child code for output */
         close(pipe_fd[0]);
         if(dup2(pipe_fd[1], STDOUT_FILENO) == -1){
            perror("dup2");
         }
         execute_helper(t->left, in_fd, pipe_fd[1]);
         close(pipe_fd[1]);
         
      }
      exit(0);
      
   }

   return 0;
}

/*
static void print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
}
*/
