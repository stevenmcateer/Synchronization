PROJECT 3B - Solving synchronization problems


Coded by: Ethan Schutzman && Steven McAteer



GOAL OF THIS ASSIGNMENT: 
We solved the wireless node synchronization problem with mutexes and with 
semaphores.



For mutex.c: 
Used mutexes (ex/ pthread_mutex_lock, pthread_mutex_trylock, pthread_mutex_unlock)



For semaphore.c: 
Used semaphores (ex/ sem_wait, sem_trywait and sem_post)





Modifying the program: 


NUM_NODES (15) -- The number of nodes that the program will create

MAX_MESSAGES_ALLOWED (1000) -- The maximum number of messages that a node can see, This value 
should be scaled proportionally to the number of nodes, based on a size 15 we can
assume that the time it would take to see 1000 messages is ample enough to prove that the system works

GRID_SIZE (50) -- This is the size of the network, in an GRID_SIZE x GRID_SIZE array

NUM_NOISEMAKERS (5) -- This is the number of noisemakers the program will make




TO RUN:


1. Run "make clean", and then "make".

2. Run "./mutex" to run the mutex implementation ("./semaphore" to run the semaphore implementation)

3. The program will run and output broadcasts to different log files for each node. The log files will 
be named using the following scheme: node # + Mut/Sem + Log.txt  Mut/Sem will be determined by the 
version of the program being run.

4. To stop the program, press ^C.
