/*David Severns 3207 Fiore
  Due 4/4/17*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include "sbuff.h"


#define EXIT_USAGE_ERROR 1
#define EXIT_GETADDRINFO_ERROR 2
#define EXIT_BIND_FAILURE 3
#define EXIT_LISTEN_FAILURE 4
#define NUM_WORKERS 3 // number of worker threads to create
#define BACKLOG 10
#define DEFAULT_PORT_STR "1776" //default port to listen on
#define MAX_DATA 1024 // the max that can be read into a "word" from client
#define QUEUE 5


using namespace std;  // to use the c++ cout and c



struct sock_buf connections;

ssize_t readLine(int fd, void *buffer, size_t n);
vector<string> getDefaultD();
int getlistenfd(char*);
void* threads(void* args);

vector<string> dictionary;

int main(int argc, char** argv){
/*open the dictionary and store it into
a data structure*/
  /*if the user doesnt give a dictionary the server
   uses the default dictionary, provided in dictionary*/
  int i;
   //string vector to hold dictionary words
  if(argc<2){

    dictionary = getDefaultD();

  }
  // if the user gives a file use that as dictionary
  else{
    ifstream user_d; // the user entered dictionary

    string word; // whats read on each line

    //open user dictionary
    user_d.open(argv[1]);
    if(!user_d){
      cout << "Error opening your dictionary using default." << endl;
      dictionary = getDefaultD(); // if cant open user dictionry use default
    }
    else{
      // if file opens add each word to the vector
      while(getline(user_d,word)){
        dictionary.push_back(word);
      }
    }
  }

  sbuf_init(&connections, QUEUE);
  int listenFd, socketFd; // to set server side port and read from client port
  struct sockaddr_storage client_addr;
  socklen_t client_addr_size;
  char line[MAX_DATA];
  ssize_t bytes_read;
  char client_name[MAX_DATA];
  char client_port[MAX_DATA];
  char *port = DEFAULT_PORT_STR;
  pthread_t workers[NUM_WORKERS]; // array of pthreads to service clients

  /*run loop to create worker threads*/
  for(unsigned int k;k<NUM_WORKERS;k++){
    pthread_create(&workers[k], NULL, threads,NULL );
  }
  listenFd = getlistenfd(port); // establish a conncect to listen on default port

  while(1){
    client_addr_size=sizeof(client_addr);
    if ((socketFd=accept(listenFd, (struct sockaddr*)&client_addr, &client_addr_size))==-1) {
      fprintf(stderr, "accept error\n");
      continue;
    }
    if (getnameinfo((struct sockaddr*)&client_addr, client_addr_size,
		    client_name, MAX_DATA, client_port, MAX_DATA, 0)!=0) {
      fprintf(stderr, "error getting name information about client\n");
    } else {
      printf("accepted connection from %s:%s\n", client_name, client_port);
    }


    sbuf_insert(&connections, socketFd);// add socket to queue wake workers

  //  cout << connections.sock_d[0] << endl;

  }


  return 0;
// run loop for main thread to listen for connections

  // open socket listen for connection on port
  // get socket descriptor from new connection
  // check to see if there is room in socket que

  /*// if yes then P(mutex) and ad socket d to que and listen for more
  // connections. if no room on queue give up mutex and wait for free space
  */
    // if producer is succesful add sd to queue
    //V(mutex) release lock
}
/* given a port number or service as string, returns a
   descriptor that we can pass to accept() */
/*********************************************
code taken from Dr. Fiore's example
********************************************/
int getlistenfd(char *port) {
  int listenfd, status;
  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_family = AF_INET;	   /* IPv4 */

  if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
    exit(EXIT_GETADDRINFO_ERROR);
  }

  /* try to bind to the first available address/port in the list.
     if we fail, try the next one. */
  for(p = res;p != NULL; p = p->ai_next) {
    if ((listenfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0) {
      continue;
    }

    if (bind(listenfd, p->ai_addr, p->ai_addrlen)==0) {
      break;
    }
  }
  freeaddrinfo(res);
  if (p==NULL) {
    exit(EXIT_BIND_FAILURE);
  }

  if (listen(listenfd, BACKLOG)<0) {
    close(listenfd);
    exit(EXIT_LISTEN_FAILURE);
  }
  return listenfd;
}


/*worker queues will check to see if something is in queue if there is
they will P(mutex) and either wait for the lock to open or if they
can take the lock they will run spell check on sd message and
echo back with "ok" or " wrong"
then close socket and V(mutex)*/

/*creates the dictionary if none is provided using the default file give*/
vector<string> getDefaultD(){
  ifstream default_d; //file stream to open the default dictionary
  vector<string> dictionary; //string vector data struct to hold words
  string word; //temp store each line from file and push to dictionary
  int i;
  // open the dictionary file
  default_d.open("dictionary");

  //check to make sure file opened
  if(!default_d){
    cout << "Error opening file" << endl;
  }
  // if file opens add each word to the vector
  while(getline(default_d,word)){
    dictionary.push_back(word);
  }

  return dictionary;
}

/*read line is also reference to code provided by the echo server*/
ssize_t readLine(int fd, void *buffer, size_t n) {
  ssize_t numRead;                    /* # of bytes fetched by last read() */
  size_t totRead;                     /* Total bytes read so far */
  char *buf;
  char ch;

  if (n <= 0 || buffer == NULL) {
    errno = EINVAL;
    return -1;
  }

  buf = (char*)buffer;                       /* No pointer arithmetic on "void *" */

  totRead = 0;
  for (;;) {
    numRead = read(fd, &ch, 1);

    if (numRead == -1) {
      if (errno == EINTR)         /* Interrupted --> restart read() */
	continue;
      else
	return -1;              /* Some other error */

    } else if (numRead == 0) {      /* EOF */
      if (totRead == 0)           /* No bytes read; return 0 */
	return 0;
      else                        /* Some bytes read; add '\0' */
	break;

    } else {                        /* 'numRead' must be 1 if we get here */
      if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
	totRead++;
	*buf++ = ch;
      }

      if (ch == '\n')
	break;
    }
  }

  *buf = '\0';
  return totRead;
}


  /*while ((bytes_read = readLine(socketFd, line, (MAX_DATA-1)))>0) {
    printf("just read %s", line);
    write(socketFd, line, bytes_read);
  }
  printf("connection closed\n");
  close(socketFd);
}*/

/*thread code for the workers to run*/
void* threads(void* args){
  int socket = sbuf_remove(&connections); // get socket from queue
  int bytes_read;
  char line[MAX_DATA];
  while((bytes_read = readLine(socket, line, (MAX_DATA-1)))>0){
    printf("just read %s", line );
    write(socket,line, bytes_read);
  }
  printf("Connection closed\n" );
  close(socket);
}
