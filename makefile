qdiff: main.c commit_stack.h
	gcc main.c -pedantic -o qdiff -lgit2 -lncurses

clean:
	rm -f qdiff
