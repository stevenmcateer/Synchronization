/*
	Ethan Schutzman && Steven McAteer
	ehschutzman     ::       smcateer
*/
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define NUM_NODES (15)
#define MIN_ZONE_WAIT_TIME (1) //min time it must wait before entering zone
#define MAX_ZONE_WAIT_TIME (10)
#define MIN_DWELL_DURATION (2)
#define MIN_TALK_PROB (0.1)
#define MIN_DWELL_PROB (0.1)
#define MAX_MESSAGES_ALLOWED (1000)
#define GRID_SIZE (50)
#define MIN_NOISE_SLEEP (5)
#define NUM_NOISEMAKERS (5)


//struct definitions
struct bcasts {
    int id;
    int message;
    int isRebroadcast;
    int channel;
};
typedef struct bcasts bcast;



struct positions {
    int x;
    int y;
};
typedef struct positions position;

struct noisemakers { 
     int channel;
     position pos;
     double talk_prob;
     int talk_duration;
     int random_wait_time;
     bcast broadcast;
     pthread_t noisemaker_thread;
};
typedef struct noisemakers noisemaker;

struct wireless_nodes {
    int id; //A unique ID for each node
    unsigned int dwell_duration; //The amount of time that a node stays on a channel before switching
    double dwell_prob; // The % chance that a node will change channels after dwell_duration is 0
    int trans_time; //The amount of time a node transmits a message
    unsigned int talk_window_time; // The amount of time a node has to consider broadcasting
    double talk_prob; // The probability that a node will talk
    position pos; // The node's x and y coordinates
    int channel; // The current channel that a node is broadcasting on
    bcast nodeBroadcast; // The message and id that will be broadcast
    pthread_t wireless_thread; // The thread that is holding a particular node
    bcast messagesRecieved[MAX_MESSAGES_ALLOWED];
    unsigned int messagesSeen;
    char *threadLog; //The string version of the id + .txt used for file io
};
typedef struct wireless_nodes wireless_node;


//define mutex and condition variable
typedef pthread_mutex_t mutex;

//Function prototypes
void broadcast(wireless_node *node);

void noisemakerBroadcast(noisemaker* maker);

int listen(wireless_node *node);

void *threadAction(void *args);

void *noisemakerAction(void *args);

void changeChannel(wireless_node *node);

int checkZone(int x, int y, bcast **chan);

int markZone(bcast broadcast, int x, int y, bcast *chan[GRID_SIZE]);

int unmarkZone(int x, int y, bcast *chan[GRID_SIZE]);

int checkZoneListen(int x, int y, bcast **chan, bcast *data);

char *idToString(int id);

bcast makeBroadcast(int num, int channel);

position makePos(int x, int y);

bcast **findChannel(int channel);

mutex *findChannelLock(int channel);



//global variables
mutex *mutex_lock;
mutex *channel_lock1;
mutex *channel_lock6;
mutex *channel_lock11;
wireless_node *all_nodes;
noisemaker* all_noisemakers;
bcast** chan1;
bcast** chan6;
bcast** chan11;
int nodePositions[GRID_SIZE][GRID_SIZE];



int main(int argc, char **argv) {
    /*   if (argc != 1) {
           printf("Usage: %s\n", argv[0]);
           return 1; // Indicate FAILURE
       } */
     printf("Executing mutex version of the simulation, press ^C to exit\n");
     chan1 = (bcast**)malloc(sizeof(bcast) * GRID_SIZE);
     chan6 = (bcast**)malloc(sizeof(bcast) * GRID_SIZE);
     chan11 = (bcast**)malloc(sizeof(bcast) * GRID_SIZE);
     for(int i =0; i< GRID_SIZE; i++){
	      chan1[i] = (bcast*)malloc(GRID_SIZE * sizeof(bcast));
        chan6[i] = (bcast*)malloc(GRID_SIZE * sizeof(bcast));
        chan11[i] = (bcast*)malloc(GRID_SIZE * sizeof(bcast));
     }

    bcast* emptyBcast = (bcast*)malloc(sizeof(bcast));
    emptyBcast->id = 0;
    emptyBcast->message = 0;
    emptyBcast->channel =0;
    emptyBcast->isRebroadcast = 0;
    for(int i = 0; i< GRID_SIZE; i++){
       for(int j = 0; j < GRID_SIZE; j++){
	  chan1[i][j].id = emptyBcast->id;
	  chan6[i][j].id = emptyBcast->id;
	  chan11[i][j].id = emptyBcast->id;

	  chan1[i][j].message = emptyBcast->message;
	  chan6[i][j].message = emptyBcast->message;
	  chan11[i][j].message = emptyBcast->id;
       }
    }
    srand((unsigned) time(NULL)); //Initialize random number generation
    mutex_lock = (mutex *) malloc(sizeof(mutex)); // Create a mutex_lock and set it to a default value
    channel_lock1 = (mutex *) malloc(sizeof(mutex));//malloc lock for channel 1
    channel_lock6 = (mutex *) malloc(sizeof(mutex));//malloc lock for channel 6
    channel_lock11 = (mutex *) malloc(sizeof(mutex));//malloc lock for channel 11
    pthread_mutex_init(mutex_lock, 0); // Initialize the mutex lock
    pthread_mutex_init(channel_lock1, 0); // Initialize the mutex lock for channel 1
    pthread_mutex_init(channel_lock6, 0); // Initialize the mutex lock for channel 6
    pthread_mutex_init(channel_lock11, 0); // Initialize the mutex lock for channel 11


    all_nodes = (wireless_node *) malloc(sizeof(wireless_node) * NUM_NODES);
    int made_nodes = 0;
    //Write loop to create nodes
    for (; made_nodes < NUM_NODES; made_nodes++) {
        all_nodes[made_nodes].id = made_nodes + 1;
        all_nodes[made_nodes].dwell_duration = (unsigned) rand() % 10 + MIN_DWELL_DURATION;
        all_nodes[made_nodes].dwell_prob = rand() % 10 / 10 + MIN_DWELL_PROB;
        all_nodes[made_nodes].trans_time = 10;
        all_nodes[made_nodes].talk_prob = (unsigned) rand() % 10 / 10 + MIN_TALK_PROB;
        all_nodes[made_nodes].talk_window_time = 1;
        all_nodes[made_nodes].pos = makePos(rand() % GRID_SIZE, rand() % GRID_SIZE);
        all_nodes[made_nodes].channel = 1;
        all_nodes[made_nodes].nodeBroadcast = makeBroadcast(made_nodes, all_nodes[made_nodes].channel);
        all_nodes[made_nodes].threadLog = idToString(all_nodes[made_nodes].id);
        all_nodes[made_nodes].messagesSeen = 0;
    }

    //make a thread for each node
    for (made_nodes = 0; made_nodes < NUM_NODES; made_nodes++) {
        int successfulThread = pthread_create(&all_nodes[made_nodes].wireless_thread, NULL, threadAction,
                                              (void *) &all_nodes[made_nodes]);

        if (successfulThread) {
            printf("Failed to create a pthread, error number: %d\n", successfulThread);
        }
    }

  
  int made_noisemakers = 0;
  
  all_noisemakers = (noisemaker *) malloc(sizeof(noisemaker) * NUM_NOISEMAKERS);
  for(; made_noisemakers < NUM_NOISEMAKERS; made_noisemakers++){
     all_noisemakers[made_noisemakers].channel = 1;
     all_noisemakers[made_noisemakers].pos = makePos(rand() % GRID_SIZE, rand() % GRID_SIZE);
     all_noisemakers[made_noisemakers].talk_prob = (unsigned) rand() % 10 / 10 + MIN_TALK_PROB;
     all_noisemakers[made_noisemakers].talk_duration = rand() % 7;
     all_noisemakers[made_noisemakers].random_wait_time = rand() % 5;
     all_noisemakers[made_noisemakers].broadcast = makeBroadcast(made_noisemakers, all_noisemakers[made_noisemakers].channel);
   
  }
   //make a thread for each noisemaker
    for (made_noisemakers = 0; made_noisemakers < NUM_NOISEMAKERS; made_noisemakers++) {
        int successfulThread = pthread_create(&all_noisemakers[made_noisemakers].noisemaker_thread, NULL, noisemakerAction,
                                              (void *) &all_noisemakers[made_noisemakers]);

        if (successfulThread) {
            printf("Failed to create a pthread, error number: %d\n", successfulThread);
        }
    }



    for (made_nodes = 0; made_nodes < NUM_NODES; made_nodes++) {
        pthread_join(all_nodes[made_nodes].wireless_thread, NULL); // Join all threads to main and set them free!
    }

    for (made_noisemakers = 0; made_noisemakers < NUM_NOISEMAKERS; made_noisemakers++) {
        pthread_join(all_noisemakers[made_noisemakers].noisemaker_thread, NULL); // Join all threads to main and set them free!
    }
  
    return 0;


}

bcast makeBroadcast(int num, int channel) {
    bcast tmp;
    tmp.id = num;
    tmp.message = rand() % 9999;
    tmp.isRebroadcast = 0;
    tmp.channel = channel;
    return tmp;

}

position makePos(int x, int y) {
  if (nodePositions[x][y] != 0){
    makePos(rand() % GRID_SIZE, rand() % GRID_SIZE);
  }
  	nodePositions[x][y] = 1;
    position tmp;
    tmp.x = x;
    tmp.y = y;
    return tmp;

}

/**
 * This function explodes the id into it's digits, puts them into a string and returns that string
 * @param id The id of a node that needs to be turned into a string
 * @return return the string version of the id
 */
char *idToString(int id) {
    char *stringNum = malloc(sizeof(char) * 15);
    sprintf(stringNum, "%d", id);
    strcat(stringNum, "MutLog.txt");
    return stringNum;

}

/**
 * Thread action is the main action handler of all threads, it will handle when a thread wants to listen broadcast or
 * change channels.
 * @return
 */
void *threadAction(void *args) {

    wireless_node *a_node = (wireless_node *) args;
    while (1) {
        // Find a random time to sleep
        int sleep_num = (MIN_ZONE_WAIT_TIME + (rand() % (MAX_ZONE_WAIT_TIME - MIN_ZONE_WAIT_TIME)));
        sleep((unsigned int) sleep_num); // Make the thread sleep for that many seconds now

        double randNum = (rand() % 10) / 10; //Will generate a decimal number between 0.0 and 1.0
        //check if it wants to broadcast
        if (a_node->talk_prob >= randNum &&
            a_node->talk_window_time != 0) { //If we want to talk and the window time isn't 0
            broadcast(a_node);
        }

        //check if it wants to listen
        listen(a_node);

        //check if it wants to change channels
        if (a_node->dwell_prob >= randNum && a_node->dwell_duration == 0) {
            changeChannel(a_node);
        }
        a_node->talk_window_time = a_node->talk_window_time - 1;
        a_node->dwell_duration = a_node->dwell_duration - 1;
        sleep(1);

    }

}

void changeChannel(wireless_node *node) {
    int randNum = rand() % 10;
    node->dwell_duration = (unsigned) MIN_DWELL_DURATION + rand() % 10;
    if (randNum < 4) {
        node->channel = 1;
    } else if (randNum < 7) {
        node->channel = 6;
    } else {
        node->channel = 11;
    }


}

/**
 *  Broadcast handles the broadcasting capabilities of the wireless_nodes
 */
void broadcast(wireless_node *node) {

    //trylock on channel_lock
    if (pthread_mutex_trylock(findChannelLock(node->channel)) == 0) {
        //if we get lock, release after broadcasting
        int probRebroadcast = (rand() %100); //Generate an int between 0 - 99 and rebroadcast a message if prob >= 50
	if(probRebroadcast >= 50 && node->messagesSeen > 0){
	  int broadcastAgain = rand() % node->messagesSeen;
	  if(node->messagesSeen == 1 && broadcastAgain == 1){
	     broadcastAgain = 0;
	  }
	  node->nodeBroadcast = node->messagesRecieved[broadcastAgain];
	  node->nodeBroadcast.isRebroadcast = 1; //Mark the message as a rebroadcast
	  node->nodeBroadcast.channel = node->channel;
	}
	else{
	  node->nodeBroadcast = makeBroadcast(node->id, node->channel);
        }
        int isBroadcasting = 0;
        //broadcast if there is no data
        if ((checkZone(node->pos.x, node->pos.y, findChannel(node->channel))) == 1) {
            isBroadcasting = markZone(node->nodeBroadcast, node->pos.x, node->pos.y, findChannel(node->channel));
	    pthread_mutex_unlock(findChannelLock(node->channel));
        } else { ; //do nothing
        }

        if (isBroadcasting) {
            sleep((unsigned) node->trans_time);
	    pthread_mutex_lock(findChannelLock(node->channel));
            unmarkZone(node->pos.x, node->pos.y, findChannel(node->channel));
            //release the lock
        }
	pthread_mutex_unlock(findChannelLock(node->channel));
    }
    
    return;

}

//function to find the correct channel
/**
 * Find Channel will find the correct broadcasting grid for each specified channel
 * @return Returns the array that the node will broadcast on.
 */
bcast **findChannel(int channel) {
    if (channel == 1) {
        return chan1;

    } else if (channel == 6) {
        return (bcast **)chan6;

    } else {
        return (bcast **)chan11;
    }

}

mutex *findChannelLock(int channel) {
    if (channel == 1) {
        return channel_lock1;
    } else if (channel == 6) {
        return channel_lock6;
    } else {
        return channel_lock11;
    }

}

/**
 * markZone takes the coordinate positions of a node and fills the broadcast array to +- 5 in x and y directions
 * to indicate that it is broadcasting in that range
 * @param x The X position of the node broadcasting
 * @param y The Y position of the node broadcasting
 * @param chan The Channel that the node is broadcasting on
 * @return Returns true when it finished filling the zone
 */
int markZone(bcast broadcast, int x, int y, bcast *chan[100]) {
    int xStart;
    int yStart;
    int xEnd;
    int yEnd;
    if(x >= 5){
       xStart = x -5;
    }else{
       xStart = 0;
    }
    if(y >= 5){
       yStart = y-5;
    }else{
       yStart = 0;
    }
    if(x +5 >= GRID_SIZE){
	xEnd = GRID_SIZE -1;
    }
    else{
       xEnd = x +5;
    }
    if(y +5 >= GRID_SIZE){
	yEnd = GRID_SIZE -1;
    }
    else{
       yEnd = y +5;
    }
	

     for (int i = yStart; i < yEnd; i++) {
        for (int j = xStart; j < xEnd; j++) {
            chan[i][j] = broadcast;

        }
    }
    return 1;
}


/**
 * unmarkZone takes the coordinate position of a node and unfills the broadcast zone of +- 5 to indicate that nothing
 * is broadcasting in that area anymore.
 * @param x  The x Coordinate of the node that needs to be cleaned up
 * @param y  The y Coordinate of the node that needs to be cleaned up
 * @param chan  The Channel that the node was broadcasting on
 * @return Returns 1 when finished
 */
int unmarkZone(int x, int y, bcast *chan[100]) {
    int xStart;
    int yStart;
    int xEnd;
    int yEnd;
    if(x >= 5){
       xStart = x -5;
    }else{
       xStart = 0;
    }
    if(y >= 5){
       yStart = y-5;
    }else{
       yStart = 0;
    }
    if(x +5 >= GRID_SIZE){
	xEnd = GRID_SIZE -1;
    }
    else{
       xEnd = x +5;
    }
    if(y +5 >= GRID_SIZE){
	yEnd = GRID_SIZE -1;
    }
    else{
       yEnd = y +5;
    }
	

     for (int i = yStart; i < yEnd; i++) {
        for (int j = xStart; j < xEnd; j++) {
            chan[i][j].id = 0;
            chan[i][j].message = 0;

        }
    }
    return 1;
}

/**
 * checkZone looks at the zone that a node would broadcast in and sees if there is any conflict
 * If there is another broadcast happening then the func will return false
 * @param x  The x coordinate of the node
 * @param y  The y coordinate of the node
 * @param chan The channel it is broadcasting on
 * @return Returns logical true if there is nothing broadcasting in the zone, false if there is something
 */
int checkZone(int x, int y, bcast **chan) {
    int isClear = 1;
    int xStart;
    int yStart;
    int xEnd;
    int yEnd;
    if(x >= 5){
       xStart = x -5;
    }else{
       xStart = 0;
    }
    if(y >= 5){
       yStart = y-5;
    }else{
       yStart = 0;
    }
    if(x +5 >= GRID_SIZE){
	xEnd = GRID_SIZE -1;
    }
    else{
       xEnd = x +5;
    }
    if(y +5 >= GRID_SIZE){
	yEnd = GRID_SIZE -1;
    }
    else{
       yEnd = y +5;
    }
	

     for (int i = yStart; i < yEnd; i++) {
        for (int j = xStart; j < xEnd; j++) {
            //if there is data
            if (chan[i][j].id != 0 && chan[i][j].message != 0) {
               isClear = 0;
            }
        }
     }
    return isClear;
}
/**
 * Listen looks to see if there is anything to listen to, and if there is then it takes the information and writes
 * it to a log file
 * @return Returns 0 if no error occurred.
 */
int listen(wireless_node *node) {
    //check the zone
    int alreadySeen = 0;
    bcast *message = malloc(sizeof(bcast));
    if (checkZoneListen(node->pos.x, node->pos.y, findChannel(node->channel), message)) {
        for (int i = 0; i < node->messagesSeen; i++) {
            if (node->messagesRecieved[i].id == message->id && node->messagesRecieved[i].message == message->message) {
                alreadySeen = 1;
                break;
            }
        }
        if (alreadySeen != 1) {
            node->messagesRecieved[node->messagesSeen].id = message->id;
            node->messagesRecieved[node->messagesSeen].message = message->message;
	    node->messagesRecieved[node->messagesSeen].isRebroadcast = message->isRebroadcast;

            FILE *fp = fopen(node->threadLog, "a+");
            fprintf(fp, "Broadcasted from node: %d with message: %d sent on channel %d and rebroadcast status: %d\n", node->messagesRecieved[node->messagesSeen].id,
                    node->messagesRecieved[node->messagesSeen].message, message->channel,message->isRebroadcast );
            fclose(fp); 

            node->messagesSeen += 1;
        }
    }
    free(message);
    //check if we've seen the message before
    //write to log file

    return 0;

}

/**
 * checkZoneListen looks to see if there is anything broadcasting in the area it can listen to, if there it it records
 * the data and handles it in the listen() funciton
 * @param x The x coordinate of the listening node
 * @param y The y coordinate of the listening node
 * @param chan The Channel that the node is listening on
 * @param data The broadcast data that will be written to if anything is found
 * @return Returns 0 if nothing is found, 1 if there is data being broadcasted.
 */
int checkZoneListen(int x, int y, bcast **chan, bcast *data) {
    int isClear = 0;
    int xStart;
    int yStart;
    int xEnd;
    int yEnd;
    if(x >= 5){
       xStart = x -5;
    }else{
       xStart = 0;
    }
    if(y >= 5){
       yStart = y-5;
    }else{
       yStart = 0;
    }
    if(x +5 >= GRID_SIZE){
	xEnd = GRID_SIZE -1;
    }
    else{
       xEnd = x +5;
    }
    if(y +5 >= GRID_SIZE){
	yEnd = GRID_SIZE -1;
    }
    else{
       yEnd = y +5;
    }
	

     for (int i = yStart; i < yEnd; i++) {
        for (int j = xStart; j < xEnd; j++) {
            //if there is data
            if (chan[i][j].id != 0 && chan[i][j].message != 0) {
                isClear = 1;
                data->id = chan[i][j].id;
                data->message = chan[i][j].message;
		data->isRebroadcast = chan[i][j].isRebroadcast;
		data->channel = chan[i][j].channel;
            }
        }
    }

    return isClear;

}

void noisemakerBroadcast(noisemaker* maker) {
	int disrupting;
	if (pthread_mutex_trylock(findChannelLock(maker->channel)) == 0) {
	    disrupting = markZone(maker->broadcast, maker->pos.x, maker->pos.y, findChannel(maker->channel));
	    sleep(maker->talk_duration);
	    pthread_mutex_unlock(findChannelLock(maker->channel));
	}
	return;
}


void* noisemakerAction(void* args) {
  noisemaker *a_maker = (noisemaker *) args;
    while (1) {
        // Find a random time to sleep
        int sleep_num = (MIN_NOISE_SLEEP + rand() % 10);
        sleep((unsigned int) sleep_num); // Make the thread sleep for that many seconds now

        double randNum = (rand() % 10) / 10; //Will generate a decimal number between 0.0 and 1.0
        //check if it wants to broadcast
        if (a_maker->talk_prob >= randNum &&
            a_maker->talk_duration != 0) { //If we want to talk and the window time isn't 0
            noisemakerBroadcast(a_maker);
        }

    }
  
  
}

