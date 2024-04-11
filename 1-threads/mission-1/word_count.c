#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h> // For isspace()

#define THREAD_COUNT 4

typedef struct
{
  char *buffer;
  size_t start;
  size_t end;
  long totalWords;
  long countA;
  long countThe;
} ThreadArgs;

void *countWords(void *args);

// Helper function to find the start of the next word
size_t findNextWordStart(char *buffer, size_t start, size_t end)
{
  while (start < end && !isspace(buffer[start]))
    start++;
  while (start < end && isspace(buffer[start]))
    start++;
  return start;
}

// Helper function to find the end of the last word
size_t findLastWordEnd(char *buffer, size_t start, size_t end)
{
  while (end > start && isspace(buffer[end]))
    end--;
  while (end > start && !isspace(buffer[end]))
    end--;
  return end + 1; // Include the last word
}

int main()
{
  // Assuming file is read into a buffer
  FILE *file = fopen("Harry_Potter.txt", "r");
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);
  char *buffer = (char *)malloc(fileSize + 1);
  fread(buffer, 1, fileSize, file);
  buffer[fileSize] = '\0'; // Ensure null-terminated string
  fclose(file);

  // replace newlines with spaces
  for (int i = 0; i < fileSize; i++)
  {
    if (buffer[i] == '\n')
    {
      buffer[i] = ' ';
    }
  }

  pthread_t threads[THREAD_COUNT];
  ThreadArgs threadArgs[THREAD_COUNT];
  size_t lengthPerThread = fileSize / THREAD_COUNT;

  for (int i = 0; i < THREAD_COUNT; i++)
  {
    threadArgs[i].buffer = buffer;
    threadArgs[i].start = i * lengthPerThread;
    if (i > 0)
    { // Adjust start to avoid cutting words in half
      threadArgs[i].start = findNextWordStart(buffer, threadArgs[i].start, fileSize);
    }
    threadArgs[i].end = (i + 1) * lengthPerThread;
    if (i < THREAD_COUNT - 1)
    { // Adjust end to avoid cutting words in half
      threadArgs[i].end = findLastWordEnd(buffer, threadArgs[i].start, threadArgs[i].end);
    }
    else
    {
      threadArgs[i].end = fileSize; // Last thread goes to the end of the file
    }

    pthread_create(&threads[i], NULL, countWords, &threadArgs[i]);
  }

  // Initialize variables to hold the aggregated results
  long totalWords = 0, totalA = 0, totalThe = 0;

  // Join threads and aggregate results
  for (int i = 0; i < THREAD_COUNT; i++)
  {
    pthread_join(threads[i], NULL); // Wait for thread completion
    totalWords += threadArgs[i].totalWords;
    totalA += threadArgs[i].countA;
    totalThe += threadArgs[i].countThe;
  }

  // Print the aggregated results
  printf("Total words counted: %ld\n", totalWords);
  printf("Occurrences of 'a': %ld\n", totalA);
  printf("Occurrences of 'the': %ld\n", totalThe);

  free(buffer); // Don't forget to free the allocated memory
  return 0;
}

void *countWords(void *args)
{
  ThreadArgs *threadArgs = (ThreadArgs *)args;
  char *buffer = threadArgs->buffer + threadArgs->start;
  size_t bufferSize = threadArgs->end - threadArgs->start;
  long wordCount = 0, countA = 0, countThe = 0;

  // Temporary word storage to compare with 'a' and 'the'
  char word[50]; // Assuming words won't exceed 49 characters

  size_t wordIndex = 0; // Index for characters in a word
  for (size_t i = 0; i < bufferSize; i++)
  {
    char c = buffer[i];
    // Check if character is part of a word
    if (isalpha(c))
    {
      word[wordIndex++] = tolower(c); // Store and convert to lowercase for comparison
    }
    else if (wordIndex > 0)
    {
      // End of a word has been reached
      word[wordIndex] = '\0'; // Null-terminate the word
      wordCount++;            // Increment total word count

      // Compare to 'a' and 'the'
      if (strcmp(word, "a") == 0)
        countA++;
      else if (strcmp(word, "the") == 0)
        countThe++;

      wordIndex = 0; // Reset for next word
    }
  }

  // Handle case where file ends with a word without trailing punctuation
  if (wordIndex > 0)
  {
    word[wordIndex] = '\0'; // Null-terminate the last word
    wordCount++;            // Increment word count for the last word

    // Compare to 'a' and 'the'
    if (strcmp(word, "a") == 0)
      countA++;
    else if (strcmp(word, "the") == 0)
      countThe++;
  }

  // Store the results back in the structure
  threadArgs->totalWords = wordCount;
  threadArgs->countA = countA;
  threadArgs->countThe = countThe;

  return NULL;
}
