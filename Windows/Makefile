OBJ_LIST = so_file_manip.obj
CFLAGS = /nologo /DDLL_EXPORTS /c

build: so_stdio.dll

so_stdio.dll: so_file_manip.obj so_proc_manip.obj
	link /nologo /dll /out:$@ /implib:so_stdio.lib $**

so_file_manip.obj: so_file_manip.c
	cl $(CFLAGS) /Fo$@ $**

so_proc_manip.obj: so_proc_manip.c
	cl $(CFLAGS) /Fo$@ $**

clean : exe_clean obj_clean

obj_clean :
	del *.obj

exe_clean :
	del so_stdio.dll so_file_manip.obj so_proc_so_file_manip.obj