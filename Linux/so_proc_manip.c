#include "so_stdio.h"

SO_FILE *so_popen(const char *command, const char *type)
{
	pid_t pid;
	int r;
	int fds[2];

	/* Initialise so_file data structure */
	SO_FILE *so_file = malloc(sizeof(SO_FILE));

	if (so_file == NULL)
		return NULL;

	so_file->fd = -1;
	so_file->pid = -1;
	strcpy(so_file->mode, type);
	so_file->cursor = 0;
	so_file->buf_cursor = 0;
	so_file->buf_len = 0;
	so_file->cursor = 0;
	so_file->error_flag = 0;
	so_file->prev_op = 0;

	r = pipe(fds);
	if (r < 0) {
		free(so_file);
		return NULL;
	}

	pid = fork();
	switch (pid) {
	case -1:
		/* error forking */
		free(so_file);
		return NULL;
	case 0:
		/* child process */
		if (type[0] == 'r') {
			/* If open in read mode, redirect STDOUT, close pipe's read end */
			dup2(fds[1], STDOUT_FILENO);
			close(fds[0]);
		} else if (type[0] == 'w') {
			/* If open in write mode, redirect STDIN, close pipe's write end */
			dup2(fds[0], STDIN_FILENO);
			close(fds[1]);
		}

		execlp("sh", "sh", "-c", command, NULL);
		/* only if exec failed */
		exit(EXIT_FAILURE);
	default:
		/* parent process */

		/* Store process pid so we know what to wait for in so_pclose */
		so_file->pid = pid;

		if (type[0] == 'r') {
			/* If open in read mode, close pipe's write end of parent's pipe */
			so_file->fd = fds[0];
			close(fds[1]);
		} else if (type[0] == 'w') {
			/* If open in write mode, close pipe's read end of parent's pipe */
			so_file->fd = fds[1];
			close(fds[0]);
		}

		break;
	}

	/* only parent process gets here */
	return so_file;
}

int so_pclose(SO_FILE *stream)
{
	int status, r, close_status, pid;

	if (stream != NULL) {
		pid = stream->pid;

		/* close the last pipe end, free the data structure */
		close_status = so_fclose(stream);

		/* wait for the process to end */
		r = waitpid(pid, &status, 0);

		if (r < 0)
			return -1;
		if (WIFEXITED(status) && close_status == 0)
			return 0;
	}

	return -1;

}
