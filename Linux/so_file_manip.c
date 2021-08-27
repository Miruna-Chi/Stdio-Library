#include "so_stdio.h"

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	SO_FILE *so_file = malloc(sizeof(SO_FILE));
	int fd;

	if (so_file == NULL)
		exit(EXIT_FAILURE);

	strcpy(so_file->mode, mode);
	so_file->cursor = 0;


	if (!strcmp(mode, "r"))
		fd = open(pathname, O_RDONLY);
	else
		if (!strcmp(mode, "r+"))
			fd = open(pathname, O_RDWR);
	else
		if (!strcmp(mode, "w"))
			fd = open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0666);
	else
		if (!strcmp(mode, "w+"))
			fd = open(pathname, O_RDWR | O_TRUNC | O_CREAT, 0666);
	else
		if (!strcmp(mode, "a"))
			fd = open(pathname, O_WRONLY | O_APPEND | O_CREAT, 0666);
	else
		if (!strcmp(mode, "a+"))
			fd = open(pathname, O_RDWR | O_APPEND | O_CREAT, 0666);
	else
		fd = -1;

	if (fd < 0) {
		free(so_file);
		return NULL;
	}
	so_file->fd = fd;
	so_file->buf_cursor = 0;
	so_file->buf_len = 0;
	so_file->cursor = 0;
	so_file->error_flag = 0;
	so_file->prev_op = 0;

	return so_file;

}

int so_fclose(SO_FILE *stream)
{
	int r;

	if (stream != NULL) {
		if (stream->buf_len > 0) {
			/* if there is anything to write, do so before closing */
			r = so_fflush(stream);
			if (r < 0) {
				free(stream);
				return SO_EOF;
			}
		}
		r = close(stream->fd);
		free(stream);
		if (r != 0)
			return SO_EOF;
		return 0;
	}
	return SO_EOF;
}

int so_fflush(SO_FILE *stream)
{
	if (stream != NULL) {
		if (stream->prev_op == 'w') {
			/* flush only makes sense if the last operation was a write */
			if (stream->mode[0] == 'a') {
				/*
				 * If the file is open in append mode, the cursor has to be
				 * moved at the end of the file. The previous cursor is still
				 * kept for another possible read in stream->cursor and
				 * won't be affected. At another read, the actual cursor will
				 * move back to where it was.
				 */
				int r;

				stream->prev_op = 0;
				r = so_fseek(stream, 0, SEEK_END);
				if (r < 0) {
					stream->error_flag = SO_EOF;
					return SO_EOF;
				}
			}
			ssize_t w;

			stream->buf_cursor = 0;
			while (stream->buf_len > 0) {
				w = write(stream->fd, stream->buffer + stream->buf_cursor,
					stream->buf_len);
				if (w >= 0) {
					stream->buf_len -= w;
					stream->buf_cursor += w;
					stream->cursor += w;
				} else {
					stream->error_flag = SO_EOF;
					return SO_EOF;
				}
			}
			stream->buf_cursor = 0;
		}
		return 0;
	}

	stream->error_flag = SO_EOF;
	return SO_EOF;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int r;

	if (stream != NULL) {
		if (stream->prev_op == 'r') {
			/* If the last operation was read, invalidate the buffer */
			stream->buf_cursor = 0;
			stream->buf_len = 0;
		} else
			if (stream->prev_op == 'w') {
				/* If the last operation was write, flush it to the file */
				if (stream->buf_len > 0) {

					r = so_fflush(stream);
					if (r < 0) {
						stream->error_flag = SO_EOF;
						return SO_EOF;
					}
				}
		}
		/* Now we can put the cursor where we want to and then store it. */
		r = lseek(stream->fd, offset, whence);
		if (r < 0) {
			stream->error_flag = SO_EOF;
			return SO_EOF;
		}
		stream->cursor = r;
	} else
		return SO_EOF;

	return 0;
}
long so_ftell(SO_FILE *stream)
{
	int r;

	if (stream != NULL) {
		if (stream->prev_op != 'r') {
			/* If the last operation was not read, do what fseek would do. */
			r = so_fseek(stream, 0, SEEK_CUR);
			if (r < 0)
				return -1;
		}
		/* Otherwise, we have the current cursor stored and at the ready. */
		return stream->cursor;
	}
	return -1;

}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t i, j;
	char ch;
	int r;

	if (stream != NULL) {
		if (stream->prev_op == 'w') {
			/*
			 * If the last operation was a write and we have something in the
			 * buffer, we need to flush it to the file.
			 */
			if (stream->buf_len > 0) {
				r = so_fflush(stream);
				if (r < 0) {
					stream->error_flag = SO_EOF;
					return SO_EOF;
				}
				/* update the cursor to the current position after write */
				stream->cursor = so_fseek(stream, 0, SEEK_CUR);
			}
		}
	} else
		return 0;

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {
			/* read a byte */
			ch = so_fgetc(stream);
			if (stream->error_flag != SO_EOF)
				((char *) ptr)[i * size + j] =  ch;
			else {
				/*
				 * If we got to EOF and a full sized element hasn't been read
				 * -> error
				 */
				if (j != size - 1)
					return 0;
				break;
			}
		}
		if (j < size)
			break;
	}

	return i;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t i, j;
	char crt_ch, ch;
	int r;

	if (stream != NULL) {
		if (stream->prev_op == 'r') {
			/*
			 * If the last operation was a read, the buffer needs to be
			 * invalidated and the buffer positioned at the last read character
			 */
			r = so_fseek(stream, stream->cursor, SEEK_SET);
			if (r < 0) {
				stream->error_flag = SO_EOF;
				return SO_EOF;
			}
		}
	} else
		return 0;

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {
			/* write a byte */
			crt_ch = ((char *) ptr)[i * size + j];
			ch = so_fputc(crt_ch, stream);
			if (stream->error_flag == SO_EOF) {
				if (j != size - 1 || ch != crt_ch)
					return 0;
				break;
			}
		}
		if (j < size)
			break;
	}

	return i;
}

int so_fgetc(SO_FILE *stream)
{
	ssize_t bytes_read;

	if (stream != NULL) {
		if (stream->error_flag == SO_EOF)
			return SO_EOF;

		if (stream->buf_cursor == 0
			|| stream->buf_cursor == BUF_SIZE
			|| (stream->buf_cursor == stream->buf_len
			&& stream->buf_len < BUF_SIZE && stream->error_flag != SO_EOF)) {
			/*
			 * Initialize buffer and its cursor.
			 * Cases where this branch is accessed:
			 * -> first read
			 * -> first buffer filled and read
			 * -> read after write where buffer was invalidated
			 */
			bytes_read = read(stream->fd, stream->buffer, BUF_SIZE);
			stream->prev_op = 'r';

			if (bytes_read <= 0) {
				stream->error_flag = SO_EOF;
				return SO_EOF;
			}

			stream->buf_len = bytes_read;
			stream->buf_cursor = 1;
			stream->cursor++;
			return stream->buffer[0];
		}

		if (stream->buf_cursor < stream->buf_len) {
			/*
			 * The other read cases where we don't need to initialize
			 * the buffer
			 */
			stream->buf_cursor++;
			stream->cursor++;
			return stream->buffer[stream->buf_cursor - 1];
		}
	}

	return SO_EOF;
}

int so_fputc(int c, SO_FILE *stream)
{
	int r;

	if (stream != NULL) {
		/* Put char in buffer. */
		stream->buffer[stream->buf_len] = c;
		stream->buf_len++;
		stream->prev_op = 'w';
		if (stream->buf_len == BUF_SIZE) {
			/* Buffer has been filled, time to write to file. */
			r = so_fflush(stream);
			if (r != 0) {
				stream->error_flag = SO_EOF;
				return SO_EOF;
			}
		}

		return c;
	}

	stream->error_flag = SO_EOF;
	return SO_EOF;
}

int so_feof(SO_FILE *stream)
{
	if (stream != NULL) {
		if (stream->error_flag == SO_EOF)
			return -1;
	} else
		return -1;

	return 0;
}

int so_ferror(SO_FILE *stream)
{
	if (stream != NULL) {
		if (stream->error_flag != 0)
			return -1;
		return 0;
	}
	return -1;
}

int so_fileno(SO_FILE *stream)
{
	if (stream != NULL)
		return stream->fd;
	return -1;
}
