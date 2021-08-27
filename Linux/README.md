#### Name: Chiricu Miruna

# stdio library implementation #
  
### Synopsis: 
- This is a minimal implementation of the stdio.h library for Linux file
manipulation based on a personalised file structure: SO_FILE

```
struct _so_file {  
   int fd;  
   char mode[3];  
   long cursor;  
   char buffer[4096];  
   int buf_cursor;  
   int buf_len;  
   char prev_op; /* r or w or 0 */  
   int error_flag;  
   int pid;  
};
```

- **fd** = file descriptor for the open file
- **mode** = 'r', 'w', 'a', 'r+', 'w+', 'a+'
- **cursor** = bytes from the beginning of the file
- **buffer**: when reading from/writing to file, fill this buffer to minimize
system calls.
- **buf_cursor** = bytes from the beginning of the buffer
- **buf_len** = string length (how many bytes have been read and put into the buffer
or how many bytes will be flushed to the file when need be)
- **prev_op** = previous operation: needs to keep count of the last operation so
that buffering is done right
- **error_flag** = will be -1 if EOF is reached
- **pid** = in case of redirecting STDIN/STDOUT, this will be the pid of the process
that executes a command and through which read/write operations will be
performed

### Ideas behind the functions:
* **read:** max 4096 bytes, put them into the buffer. Next read operations
      will read from the buffer, not from the file, until:
	* all the buffer has been read
	* a write operation has invalidated the buffer (in this case there will be a
	read syscall and a reinitialization of the buffer)
	* the length of the string from the buffer is less than the buffer's size
	(which means EOF has been reached before reading 4096 bytes)
* **write**: max 4096 bytes will be put into the buffer before flushing them
      to a file. Next write operations will write to the buffer, until:
	-  the buffer has been filled and it has to be flushed to the file
	* a read operation has to overwrite the buffer so the current state
	of the buffer has to be flushed to the file
	
	*More about each function can be found in the comments.*

### Is this implementation efficient?
Considering it minimizes syscalls through buffering, yes, I'd say so. Could it
be better? It could always be better.

### Is it useful?
For educational purposes, yes. The best way to learn how system calls
affect program performance, how file manipulation is done behind the
usual API and how processes and pipes work is to actually implement
them yourself.
	
##### Note: The whole assignment has been implemented.

### How to compile & run?
This is a shared library, you can include it in another program:

If you include it from the same directory as your code:
```
make
gcc -Wall your_code.c -o your_exec -lso_stdio -L.
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
./your_exec <params>
```
Otherwise, `.` becomes the path of the library

### Bibliography
-   [Laborator 1](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-01 "so:laboratoare:laborator-01")
-   [Laborator 2](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-02 "so:laboratoare:laborator-02")
-   [Laborator 3](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-03 "so:laboratoare:laborator-03")

### Git (where you can see this beautiful .md README too)
https://gitlab.cs.pub.ro/miruna.chiricu/l3-so-assignments/tree/master/2-stdio
