all:
	clang -Wall -g -c -o mythreads.o mythreads.c
	ar -cvr libmythreads.a mythreads.o
	clang -Wall -g -o ./a.out preemtive_test.c libmythreads.a
clean:
	rm -f libmythreads.a mythreads.o a.out
