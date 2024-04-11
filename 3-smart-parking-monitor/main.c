#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_CARS 25
#define ROWS 5
#define COLUMNS 5
#define FREE ""

#define LOG_MESSAGE_LENGTH 200
#define MAX_LOG_MESSAGES 10

// ANSI Escape Codes for coloring
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_CYAN "\x1b[36m"

typedef struct
{
  char text[LOG_MESSAGE_LENGTH];
  TickType_t expiryTick;
} LogMessage;

// Registration number starts from 2001 for simplicity
volatile char parking[ROWS][COLUMNS][9];
volatile int freeSpaces = MAX_CARS;
volatile char latestIncoming[9], latestOutgoing[9];

SemaphoreHandle_t parkingMutex;
TaskHandle_t incomingTaskHandle, outgoingTaskHandle, displayTaskHandle, manualControlTaskHandle;

QueueHandle_t logQueue;

// Task prototypes
void incomingCarsTask(void *params);
void outgoingCarsTask(void *params);
void displayParkingTask(void *params);
void manualControlTask(void *params);

void setup()
{
  parkingMutex = xSemaphoreCreateMutex();
  logQueue = xQueueCreate(MAX_LOG_MESSAGES, sizeof(LogMessage));

  if (logQueue == NULL)
  {
    // Handle error in queue creation
    printf("Failed to create log queue.\n");
  }

  // Initialize all parking spots to FREE
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      strcpy_s(parking[i][j], sizeof(char) * 9, FREE);
    }
  }

  xTaskCreate(incomingCarsTask, "IncomingCars", configMINIMAL_STACK_SIZE, NULL, 2, &incomingTaskHandle);
  xTaskCreate(outgoingCarsTask, "OutgoingCars", configMINIMAL_STACK_SIZE, NULL, 2, &outgoingTaskHandle);
  xTaskCreate(displayParkingTask, "DisplayParking", configMINIMAL_STACK_SIZE, NULL, 1, &displayTaskHandle);
  xTaskCreate(manualControlTask, "ManualControl", configMINIMAL_STACK_SIZE, NULL, 1, &manualControlTaskHandle);

  vTaskStartScheduler();
}

void logMessage(const char *format, ...)
{
  // Ensure the queue is created
  if (logQueue == NULL)
  {
    return; // Optionally, handle the uninitialized queue case
  }

  LogMessage log;
  va_list args;

  // Start variadic arguments to format the string
  va_start(args, format);
  vsnprintf(log.text, LOG_MESSAGE_LENGTH, format, args);
  va_end(args);

  log.expiryTick = xTaskGetTickCount();

  // Send formatted log message to the queue
  if (xQueueSendToBack(logQueue, &log, 0) != pdPASS)
  {
    // Handle failed to send message
  }
}

void clearInputBuffer()
{
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
  {
  }
}

// A simple function to get the current system time as a string (HH:MM:SS format)
void getCurrentTimeString(char *buffer, size_t bufferSize)
{
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  strftime(buffer, bufferSize, "%H:%M:%S", tm_info);
}

void generateUKNumberPlate(char *plate)
{
  const char *letters = "ABCDEFGHKLMNOPRSVWY";
  int year = 1 + rand() % 99;    // Years from 2001 to 2099
  int isSecondHalf = rand() % 2; // Randomly decide if it's the first or second half of the year

  if (isSecondHalf)
  {
    year += 50; // Add 50 for registrations in the second half of the year
  }

  // Adjust year for the actual numbering scheme
  year = (year % 100) < 50 ? (year % 100) : ((year % 100) - 50);

  // Generate two random letters
  char firstLetter = letters[rand() % 19];
  char secondLetter = letters[rand() % 19];

  // Generate three random letters for the suffix
  char suffix1 = letters[rand() % 19];
  char suffix2 = letters[rand() % 19];
  char suffix3 = letters[rand() % 19];

  sprintf_s(plate, sizeof(char) * 9, "%c%c%02d %c%c%c", firstLetter, secondLetter, year, suffix1, suffix2, suffix3);
}

bool isParkingSpaceFree(int row, int column)
{
  return (strcmp(parking[row][column], FREE) == 0);
}

bool assignParkingSpace(char plate[9], int rowCoord, int colCoord)
{
  // If specific coordinates are provided (non-negative), try to assign the space
  if (rowCoord >= 0 && colCoord >= 0)
  {
    if (parking[rowCoord][colCoord] == FREE)
    {

      // Update the latest incoming car's registration number
      strcpy_s(latestIncoming, sizeof(latestIncoming), plate);
      strcpy_s(parking[rowCoord][colCoord], sizeof(char) * 9, plate);

      logMessage("Car with plate %s assigned to (%d, %d).\n", plate, rowCoord, colCoord);
      freeSpaces--;
      return true;
    }
    else
    {
      logMessage("Requested space (%d, %d) is not free. Assigning random free space...\n", rowCoord, colCoord);
    }
  }

  // Either no specific coordinates provided or requested space wasn't free, find a random free space
  int allFreeSpaces[ROWS * COLUMNS][2];
  int freeCount = 0;

  for (int r = 0; r < ROWS; ++r)
  {
    for (int c = 0; c < COLUMNS; ++c)
    {
      if (isParkingSpaceFree(r, c))
      {
        allFreeSpaces[freeCount][0] = r;
        allFreeSpaces[freeCount][1] = c;
        ++freeCount;
      }
    }
  }

  if (freeCount > 0)
  {
    int randomIndex = rand() % freeCount;

    // Update the latest incoming car's registration number
    strcpy_s(latestIncoming, sizeof(latestIncoming), plate);
    strcpy_s(parking[allFreeSpaces[randomIndex][0]][allFreeSpaces[randomIndex][1]], sizeof(char) * 9, plate);

    logMessage("Car with plate %s randomly assigned to (%d, %d).\n", plate, allFreeSpaces[randomIndex][0], allFreeSpaces[randomIndex][1]);

    freeSpaces--;
    return true;
  }
  else
  {
    logMessage("No free parking spaces available.\n");
    return false;
  }
}

void removeCarFromSpace(int row, int col)
{

  // Update the latest outgoing car's registration number
  strcpy_s(latestOutgoing, sizeof(latestOutgoing), parking[row][col]);

  // Mark the parking space as
  strcpy_s(parking[row][col], sizeof(parking[row][col]), FREE);

  // Increment the count of free spaces
  freeSpaces++;

  logMessage("Car %s has left from space (%d, %d). Free spaces: %d\n", latestOutgoing, row, col, freeSpaces);
}

void incomingCarsTask(void *params)
{
  while (1)
  {
    int carsToEnter = (rand() % 5) + 1; // Randomly choose between 1 to 5 cars entering

    if (xSemaphoreTake(parkingMutex, portMAX_DELAY) == pdTRUE)
    {
      for (int i = 0; i < carsToEnter; ++i)
      {

        char plate[9];
        generateUKNumberPlate(plate);
        // assign a random spot
        assignParkingSpace(plate, -1, -1);
      }
      xSemaphoreGive(parkingMutex);
    }

    // Simulate the time it takes for the next group of cars to arrive
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void outgoingCarsTask(void *params)
{
  while (1)
  {
    if (xSemaphoreTake(parkingMutex, portMAX_DELAY) == pdTRUE)
    {
      // Only proceed if there are less than MAX_CARS free spaces, indicating occupancy
      if (freeSpaces < MAX_CARS)
      {
        int carsToRemove = 2; // Number of cars leaving

        for (int i = 0; i < carsToRemove; ++i)
        {
          // Ensure we still have cars to remove
          if (freeSpaces < MAX_CARS)
          {
            int found = 0;
            int attempts = 0; // To avoid infinite loops in sparse occupancy

            while (!found && attempts < MAX_CARS)
            {
              int row = rand() % ROWS;
              int col = rand() % COLUMNS;
              if (!isParkingSpaceFree(row, col))
              {
                removeCarFromSpace(row, col);
                found = 1; // An occupied space has been found and cleared
              }
              attempts++;
            }

            if (attempts >= MAX_CARS)
            {
              // Could not find a car to remove after checking all spaces
              logMessage("Attempted to remove a car but none were found.\n");
              break; // Exit the loop as there may not be enough cars to remove
            }
          }
        }
      }
      else
      {
        logMessage("Parking lot is empty. No cars to remove.\n");
      }

      xSemaphoreGive(parkingMutex);
    }

    // Simulate the time it takes for cars to leave
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

bool isValidPlate(const char *plate)
{
  int len = strlen(plate);
  // Check if length is within the expected range
  printf("length: %d", len);
  if (len < 7 || len > 8)
  {
    return false;
  }

  // Example validation: The first two characters should be letters
  if (!isalpha(plate[0]) || !isalpha(plate[1]))
  {
    return false;
  }

  // More detailed format validation can be added here
  // E.g., checking for specific character positions, numbers, etc.

  return true; // Plate passes basic validation
}

void manualControlTask(void *params)
{
  char input;
  int row, col;
  char plate[9];

  while (1)
  {
    input = getch(); // Assuming getch() or similar function is available

    if ((input == 'I' || input == 'i') && xSemaphoreTake(parkingMutex, portMAX_DELAY))
    {
      // Suspend automated car handling tasks
      vTaskSuspend(incomingTaskHandle);
      vTaskSuspend(outgoingTaskHandle);
      vTaskSuspend(displayTaskHandle);

      // Manual addition logic
      printf("Enter parking row (0-4): ");
      scanf_s("%d", &row);
      printf("Enter parking column (0-4): ");
      scanf_s("%d", &col);

      // Inside the if block for manual addition ('I' or 'i')
      printf("Enter car plate (e.g., AB51 CDE): ");
      clearInputBuffer();
      fgets(plate, sizeof(plate), stdin);
      // Remove the newline character if fgets reads one
      plate[strcspn(plate, "\n")] = 0;

      if (isValidPlate(plate))
      {
        if (row >= 0 && row < ROWS && col >= 0 && col < COLUMNS)
        {
          if (isParkingSpaceFree(row, col))
          {
            strcpy_s(parking[row][col], sizeof(char) * 9, plate);
            printf("Car %s added manually at (%d, %d).\n", plate, row, col);
          }
          else
          {
            printf("Space (%d, %d) is already occupied.\n", row, col);
          }
        }
        else
        {
          printf("Invalid row or column.\n");
        }
      }
      else
      {
        printf("Invalid plate format.\n");
      }

      xSemaphoreGive(parkingMutex);
      // Resume tasks
      vTaskResume(incomingTaskHandle);
      vTaskResume(outgoingTaskHandle);
      vTaskResume(displayTaskHandle);
    }
    else if ((input == 'O' || input == 'o') && xSemaphoreTake(parkingMutex, portMAX_DELAY))
    {
      // Suspend tasks
      vTaskSuspend(incomingTaskHandle);
      vTaskSuspend(outgoingTaskHandle);
      vTaskSuspend(displayTaskHandle);

      // Manual removal logic
      printf("Enter parking row (0-4) for removal: ");
      scanf_s("%d", &row);
      printf("Enter parking column (0-4) for removal: ");
      scanf_s("%d", &col);

      if (row >= 0 && row < ROWS && col >= 0 && col < COLUMNS)
      {
        if (!isParkingSpaceFree(row, col))
        {
          printf("Car %s removed manually from (%d, %d).\n", parking[row][col], row, col);
          strcpy_s(parking[row][col], sizeof(char) * 9, FREE); // Mark the space as free
        }
        else
        {
          printf("Space (%d, %d) is already free.\n", row, col);
        }
      }
      else
      {
        printf("Invalid row or column.\n");
      }

      xSemaphoreGive(parkingMutex);

      // Resume tasks
      vTaskResume(incomingTaskHandle);
      vTaskResume(outgoingTaskHandle);
      vTaskResume(displayTaskHandle);
    }
    else if (input == 'ESC')
    {
      printf("Exiting manual mode.\n");
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to debounce inputs
  }
}

void displayOutput()
{
  system("cls"); // Clear the console

  char timeString[9]; // Enough space for HH:MM:SS\0
  getCurrentTimeString(timeString, sizeof(timeString));

  // Display the header
  printf(ANSI_COLOR_CYAN "                    Smart Parking Monitoring System\n" ANSI_COLOR_RESET);
  printf("==========================================================================\n\n");
  printf(ANSI_COLOR_GREEN "Latest Incoming Car: %s\n" ANSI_COLOR_RESET, latestIncoming);
  printf(ANSI_COLOR_RED "Latest Outgoing Car: %s\n" ANSI_COLOR_RESET, latestOutgoing);
  printf("\n================= Parking Spaces: %d/%d (%d spaces left) ==================\n\n", MAX_CARS - freeSpaces, MAX_CARS, freeSpaces);

  // Parking grid with better spacing and color
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      if (isParkingSpaceFree(i, j))
      {
        printf(ANSI_COLOR_GREEN "[    FREE    ] " ANSI_COLOR_RESET);
      }
      else
      {
        printf(ANSI_COLOR_RED "[  %-10s] " ANSI_COLOR_RESET, parking[i][j]); // Fixed width for registration number
      }
    }
    printf("\n");
  }
  printf("\n===============================");
  printf(ANSI_COLOR_YELLOW " [%s] " ANSI_COLOR_RESET, timeString);
  printf("===============================\n");
}

void displayParkingTask(void *params)
{
  while (1)
  {
    if (xSemaphoreTake(parkingMutex, portMAX_DELAY) == pdTRUE)
    {
      displayOutput();

      xSemaphoreGive(parkingMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
  }
}

int main()
{
  setup();
  for (;;)
    ;
}
