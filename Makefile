all: main.exe

main.exe: main.c
	ccpsx -O2 -Xo$80100000 main.c -omain.cpe,main.sym,mem.map
	cpe2x main.cpe
	del main.cpe

clean:
	del main.exe
	del main.sym
	del main.map
	del main.obj
