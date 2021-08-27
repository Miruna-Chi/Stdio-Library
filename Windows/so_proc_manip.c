#include "so_stdio.h"

SO_FILE *so_popen(const char *command, const char *type)
{
	int r;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HANDLE hReadPipe, hWritePipe;
	SECURITY_ATTRIBUTES sa;
	BOOL ret;
	char *full_cmd;

	/* Initialise so_file data structure */
	SO_FILE *so_file = malloc(sizeof(SO_FILE));

	if (so_file == NULL)
		return NULL;

	so_file->handle = INVALID_HANDLE_VALUE;
	strcpy(so_file->mode, type);
	so_file->cursor = 0;
	so_file->buf_cursor = 0;
	so_file->buf_len = 0;
	so_file->cursor = 0;
	so_file->error_flag = 0;
	so_file->prev_op = 0;

	/* Complete the command */
	full_cmd = malloc(strlen(command) + 1 + strlen("cmd /C "));
	if (full_cmd == NULL) {
		free(so_file);
		return NULL;
	}
	strcpy(full_cmd, "cmd /C ");
	strcat(full_cmd, command);

	/* Initialize pipe */
	ZeroMemory(&sa, sizeof(sa));

	/* Setup flags to allow handle inheritance (redirection) */
	sa.bInheritHandle = TRUE;

	ret = CreatePipe(
		&hReadPipe,
		&hWritePipe,
		&sa,
		0
	);

	if (ret == FALSE) {
		free(full_cmd);
		free(so_file);
		return NULL;
	}

	/*
	 * Set up members of the STARTUPINFO structure.
	 * This structure specifies the STDIN and STDOUT
	 * handles for redirection.
	 */

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	ZeroMemory(&pi, sizeof(pi));

	if (type[0] == 'r') {
		/*
		 * the child is going to write to STDOUT -> don't inherit
		 * the read head
		 */
		ret = SetHandleInformation(
			hReadPipe,
			HANDLE_FLAG_INHERIT,
			0
		);
		/* redirect the write pipe to STDOUT */
		si.hStdOutput = hWritePipe;
		/* parent process is going to use STDIN */
		so_file->handle = hReadPipe;
	} else
		if (type[0] == 'w') {
			/*
			 * the child is going to read from STDIN
			 * -> don't inherit the write head
			 */
			ret = SetHandleInformation(
				hWritePipe,
				HANDLE_FLAG_INHERIT,
				0
			);
			/* redirect the read pipe to STDIN */
			si.hStdInput = hReadPipe;
			/* parent process is going to use STDOUT */
			so_file->handle = hWritePipe;
	} else {
		free(full_cmd);
		free(so_file);
		return NULL;
	}

	if (ret == FALSE) {
		free(full_cmd);
		free(so_file);
		return NULL;
	}

	ret =  CreateProcess(
		NULL,           /* No module name (use command line) */
		full_cmd,       /* Command line */
		NULL,           /* Process handle is not inheritable */
		NULL,           /* Thread handle not inheritable */
		TRUE,           /* Set handle inheritance to TRUE */
		0,              /* No creation flags */
		NULL,           /* Use parent's environment block */
		NULL,           /* Use parent's starting directory */
		&si,            /* Pointer to STARTUPINFO structure */
		&pi             /* Pointer to PROCESS_INFORMATION structure */
	);

	if (ret == FALSE) {
		free(full_cmd);
		free(so_file);
		return NULL;
	}

	so_file->processHandle = pi.hProcess;

	/* only parent process gets here and closes its own pipe heads */
	if (type[0] == 'r')
		CloseHandle(hWritePipe);
	else
		if (type[0] == 'w')
			CloseHandle(hReadPipe);

	free(full_cmd);
	return so_file;
}

int so_pclose(SO_FILE *stream)
{
	int close_status;
	HANDLE pHandle;
	DWORD dwRes;
	BOOL bRes;

	if (stream != NULL) {
		pHandle = stream->processHandle;

		/* close the last pipe end, free the data structure */
		close_status = so_fclose(stream);

		/* Wait for the child to finish */
		dwRes = WaitForSingleObject(pHandle, INFINITE);
		if (dwRes == WAIT_FAILED)
			return -1;

		/* See how the child finished */
		bRes = GetExitCodeProcess(pHandle, &dwRes);
		if (bRes == FALSE)
			return -1;

		if (close_status == 0)
			return 0;
	}

	return -1;

}
