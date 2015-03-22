//Killian Mills 11368701
//Mark McCluskey 12514857
 
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
 
typedef struct {
  int firstNum; //first sigHoldber to be added
  int secondNum; //second sigHoldber to be added
  char *fileName; //a string containing the filename
  pthread_t mainRef; //our reference to the main thread
 
}shared;
 
 
//reader thread reads from file and prints what is has read
static void * reader(void *shared_in){
 
  //bring struct into use
  shared *structShared = (shared *)shared_in;
 
  //set up signal blocker
  sigset_t readBlock;
  sigemptyset(&readBlock);
  sigaddset(&readBlock, SIGUSR1);
  pthread_sigmask(SIG_BLOCK, &readBlock, NULL);
 
  int sigHold; //hold the value of the current signal
   
  FILE *fr;
  fr = fopen(structShared->fileName, "r");
 
  fscanf(fr, "%d", &structShared->firstNum);
  fscanf(fr, "%d", &structShared->secondNum);
  sigwait(&readBlock,&sigHold);
 
  while(!feof(fr)){ //while not end of file
 
    printf("Read in first number %d\n", structShared->firstNum);
    printf("Read in second number %d\n", structShared->secondNum);
    int randomSleep = rand() % 10000;
    usleep( randomSleep );
 
    pthread_kill(structShared->mainRef, SIGUSR1); //tell main to go to work
 
    sigwait(&readBlock,&sigHold); //wait for main to send back to work
    fscanf(fr, "%d", &structShared->firstNum);
    fscanf(fr, "%d", &structShared->secondNum);
 
  }
   
  fclose(fr); //close the file
 
  //signal to main that reader has finished
  pthread_kill(structShared->mainRef, SIGINT);
  sigwait(&readBlock,&sigHold);
}
 
 
//calculator thread adds two numbers together and prints result
static void * calculator(void *shared_in){
 
  //bring struct into use
  shared *structShared = (shared *)shared_in;
 
  //set up signal blocker
  sigset_t calcBlock;
  sigemptyset(&calcBlock);
  sigaddset(&calcBlock, SIGUSR2);
  pthread_sigmask(SIG_BLOCK, &calcBlock, NULL);
 
  int sigHold; //hold the value of the current signal
 
  while(1){
 
    sigwait(&calcBlock,&sigHold); //wait for main to send back to work
 
    int result = (structShared->firstNum + structShared->secondNum);
    printf("Result: %d", structShared->firstNum);
    printf(" + %d ",structShared->secondNum);
    printf("= %d\n\n", result);
 
    pthread_kill(structShared->mainRef,SIGUSR2); //tell main to go to work
 
  }
 
}
 
 
int  main(int argc, char *argv[]){
  pthread_t readThread, calcThread; //make reader and calculator threads
 
  //check that a file was given
  if (argc != 2) {
    printf("Usage: %s file\n", argv[0]);
    exit(EXIT_FAILURE);
  }
 
  shared structShared; //declares the shared struct
  structShared.mainRef = pthread_self(); //makes t the reference to the main
  structShared.fileName = argv[1]; //makes filename the reference to input
 
  //adds the three signals we are using to the main thread's signal blocker
  sigset_t mainBlock;
  sigemptyset(&mainBlock);
  sigaddset(&mainBlock, SIGUSR1);
  sigaddset(&mainBlock, SIGUSR2);
  sigaddset(&mainBlock, SIGINT);
  pthread_sigmask(SIG_BLOCK, &mainBlock, NULL);
 
  //Create our threads
  pthread_create(&readThread, NULL, reader, (void *)&structShared);
  pthread_create(&calcThread, NULL, calculator, (void *)&structShared);
 
  int sigHold; //hold the value of the current signal
 
  while(1){ //loop until the break occurs
 
    pthread_kill(readThread, SIGUSR1); //send reader to work
    sigwait(&mainBlock,&sigHold); //wait for reader to be done
 
    //if a sigint is sent, close the threads
    if (sigHold == SIGINT){
      pthread_cancel(readThread);
      pthread_cancel(calcThread);
      printf("Goodbye From Reader Thread\n");
      printf("Goodbye From Calculator Thread\n");
      break;
    }
 
    pthread_kill(calcThread, SIGUSR2); //send calculator to work
    sigwait(&mainBlock,&sigHold); //wait for calculator to be done
 
  }
 
  //Wait for our threads
  pthread_join(readThread, NULL);
  pthread_join(calcThread, NULL);
 
  return (0);
 
}