CC = gcc

build: libso_stdio.so

libso_stdio.so: so_file_manip.o so_proc_manip.o
	$(CC) -shared $^ -o $@

so_file_manip.o: so_file_manip.c
	$(CC) -Wall -fPIC -c $^ -o $@

so_proc_manip.o: so_proc_manip.c
	$(CC) -Wall -fPIC -c $^ -o $@

.PHONY: clean

clean:
	rm -f so_file_manip.o so_proc_manip.o libso_stdio.so