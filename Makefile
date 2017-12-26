all: mutex semaphore

semaphore: semaphore.o
	gcc -g semaphore.o -o semaphore -lpthread
semaphore.o: semaphore.c
	gcc -g -c semaphore.c -lpthread
mutex: mutex.o
	gcc -g mutex.o -o mutex -lpthread

mutex.o: mutex.c
	gcc -g -c mutex.c -lpthread

clean:
	rm -f *.o
	rm -f mutex
	rm -f semaphore
	rm -f *Log.txt
