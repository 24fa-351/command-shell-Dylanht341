cshell: cshell.c cshell.h main.c
			gcc -o cshell cshell.c main.c

clean:
			rm cshell