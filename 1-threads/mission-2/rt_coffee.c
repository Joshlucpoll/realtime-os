#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

int customerCount;
volatile int customersServed = 0;
pthread_mutex_t customersServedMutex;
sem_t coffeeSemaphore;
pthread_mutex_t printMutex;

void *coffeeMachine(void *arg)
{
  srand(time(NULL));

  while (1)
  {
    // Simulate time taken to make coffee
    sleep(2);

    // Producing 1 to 3 coffees
    int coffeesMade = rand() % 3 + 1;

    pthread_mutex_lock(&printMutex);
    printf("\nCoffee machine made %d coffee(s).\n", coffeesMade);
    pthread_mutex_unlock(&printMutex);

    for (int i = 0; i < coffeesMade; i++)
    {
      sem_post(&coffeeSemaphore); // Make a coffee available
    }

    pthread_mutex_lock(&customersServedMutex);
    if (customersServed >= customerCount)
    {
      pthread_mutex_unlock(&customersServedMutex);
      break; // All customers served, stop the coffee machine
    }
    pthread_mutex_unlock(&customersServedMutex);
  }
  return NULL;
}

void *customer(void *arg)
{
  int customerId = *(int *)arg;

  // Wait for coffee to be available
  sem_wait(&coffeeSemaphore);

  pthread_mutex_lock(&printMutex);
  pthread_mutex_lock(&customersServedMutex);

  printf("Customer %d received a coffee. %d customer(s) have been severed\n", customerId, customersServed++);

  pthread_mutex_unlock(&customersServedMutex);
  pthread_mutex_unlock(&printMutex);

  return NULL;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Using default number of customers: 20\n");
    printf("Usage: %s <number of customers>\n", argv[0]);

    customerCount = 20;
  }
  else
  {
    customerCount = atoi(argv[1]);
  }

  pthread_t customers[customerCount], machine;
  int customerIds[customerCount];

  // Initialize semaphore with 0 coffees available
  sem_init(&coffeeSemaphore, 0, 0);
  pthread_mutex_init(&printMutex, NULL);
  pthread_mutex_init(&customersServedMutex, NULL);

  // Start coffee machine thread
  if (pthread_create(&machine, NULL, coffeeMachine, NULL) != 0)
  {
    perror("Failed to create the coffee machine thread");
    exit(EXIT_FAILURE);
  }

  // Start customer threads
  for (int i = 0; i < customerCount; i++)
  {
    customerIds[i] = i + 1; // Customer ID (1-based)
    if (pthread_create(&customers[i], NULL, customer, &customerIds[i]) != 0)
    {
      perror("Failed to create a customer thread");
      exit(EXIT_FAILURE);
    }
  }

  // Join customer threads
  for (int i = 0; i < customerCount; i++)
  {
    if (pthread_join(customers[i], NULL) != 0)
    {
      perror("Failed to join a customer thread");
      exit(EXIT_FAILURE);
    }
  }

  // Join the coffee machine thread
  if (pthread_join(machine, NULL) != 0)
  {
    perror("Failed to join the coffee machine thread");
    exit(EXIT_FAILURE);
  }

  // Cleanup
  sem_destroy(&coffeeSemaphore);
  pthread_mutex_destroy(&printMutex);
  pthread_mutex_destroy(&customersServedMutex);

  printf("All customers have received their coffee.\n");
  return 0;
}
