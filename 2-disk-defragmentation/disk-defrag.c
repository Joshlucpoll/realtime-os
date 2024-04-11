#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h> // for usleep function

#define ROWS 5
#define COLUMNS 10
#define DISK_SIZE (ROWS * COLUMNS)
#define FILE_COUNT 10

typedef struct
{
    char fileName[20];  // File block info, empty if the block doesn't contain a part of a file
    int nextBlockIndex; // Index to the next block, -1 if it's the last block
    int occupied;       // Whether the block is occupied
} DiskBlock;

typedef struct
{
    char fileName[20];
    char creationDate[20];
    int firstBlockIndex;
} FileInfo;

int numberOfFiles = FILE_COUNT;
DiskBlock disk[ROWS][COLUMNS];
FileInfo files[FILE_COUNT];

void initializeDisk()
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            disk[i][j].occupied = 0;
            disk[i][j].nextBlockIndex = -1; // -1 indicates no next block
            strcpy(disk[i][j].fileName, "");
        }
    }
}

// returns the first unoccupied block it finds.
int findFirstEmptyBlock()
{
    for (int i = 0; i < DISK_SIZE; i++)
    {
        int row = i / COLUMNS;
        int col = i % COLUMNS;
        if (!disk[row][col].occupied)
        {
            return i;
        }
    }
    // Return -1 if no unoccupied blocks found.
    return -1;
}

int selectRandomUnoccupiedBlock()
{
    int blockIndex = rand() % DISK_SIZE;
    int row = blockIndex / COLUMNS;
    int col = blockIndex % COLUMNS;
    while (disk[row][col].occupied)
    {
        blockIndex = rand() % DISK_SIZE;
        row = blockIndex / COLUMNS;
        col = blockIndex % COLUMNS;
    }
    return blockIndex;
}

void deleteFile()
{
    printf("\n\nFiles\n------\n");
    for (int i = 0; i < numberOfFiles; i++)
    {
        printf("%d: %s\n", i + 1, files[i].fileName);
    }
    printf("%d: None\n", numberOfFiles + 1);

    int fileToDelete;
    printf("\nEnter the number of the file to delete, or %d to keep all files: ", numberOfFiles + 1);
    scanf("%d", &fileToDelete);
    getchar(); // consume newline left in the buffer.

    fileToDelete--; // convert from 1-indexed to 0-indexed

    if (fileToDelete >= 0 && fileToDelete < numberOfFiles)
    {
        // Unoccupy the blocks
        DiskBlock *db = &disk[files[fileToDelete].firstBlockIndex / COLUMNS][files[fileToDelete].firstBlockIndex % COLUMNS];
        while (db->nextBlockIndex != -1)
        {
            db->occupied = 0;
            db = &disk[db->nextBlockIndex / COLUMNS][db->nextBlockIndex % COLUMNS];
        }
        db->occupied = 0; // last block

        // Move each following FileInfo up one spot
        for (int i = fileToDelete; i < numberOfFiles - 1; i++)
        {
            files[i] = files[i + 1];
        }
        numberOfFiles--;
        printf("File deleted successfully.\n");
    }
    else if (fileToDelete == numberOfFiles)
    {
        printf("No files were deleted.\n");
    }
    else
    {
        printf("Invalid input, no files were deleted.\n");
    }
}

void addNewFile()
{
    char newFileName[20]; // File Name
    int blockCount;       // Number of blocks

    getchar(); // Clear the input buffer

    printf("\nEnter the file name: ");
    fgets(newFileName, sizeof(newFileName), stdin); // input file name
    newFileName[strcspn(newFileName, "\n")] = 0;    // remove newline from fgets

    // Get the current date
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char newCreationDate[20];
    strftime(newCreationDate, sizeof(newCreationDate), "%Y-%m-%d", tm);

    printf("\nEnter the number of blocks: ");
    scanf("%d", &blockCount); // input number of blocks
    getchar();                // To remove the newline left in the input buffer.

    FileInfo newFile;
    strncpy(newFile.fileName, newFileName, sizeof(newFile.fileName));
    strncpy(newFile.creationDate, newCreationDate, sizeof(newFile.creationDate));
    newFile.firstBlockIndex = -1;
    files[numberOfFiles] = newFile; // Add to the files array.

    // Create blocks
    int previousBlockIndex = -1;
    for (int i = blockCount; i > 0; i--)
    {
        int targetIndex = findFirstEmptyBlock(); // This function returns the first unoccupied block it finds.
        if (targetIndex == -1)
        {
            printf("Disk is full. Cannot add more files.\n");
            return;
        }

        // Init new block
        DiskBlock newBlock;
        strncpy(newBlock.fileName, newFileName, sizeof(newBlock.fileName));
        newBlock.occupied = 1;
        newBlock.nextBlockIndex = -1;
        disk[targetIndex / COLUMNS][targetIndex % COLUMNS] = newBlock;

        // If this is not the first block of the file, link previous block to this
        if (previousBlockIndex != -1)
        {
            disk[previousBlockIndex / COLUMNS][previousBlockIndex % COLUMNS].nextBlockIndex = targetIndex;
        }

        // Update firstBlockIndex in file info if this is the first block
        else
        {
            files[numberOfFiles].firstBlockIndex = targetIndex;
        }

        previousBlockIndex = targetIndex;
    }
    // Increment the total file count.
    numberOfFiles += 1;
    printf("\nFile added successfully.\n");
}

void createFiles()
{
    srand(time(NULL)); // Seed for random number generation
    for (int i = 0; i < numberOfFiles; i++)
    {
        // Generate file details
        sprintf(files[i].fileName, "File%d", i + 1);

        // Generate random creation date
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        int year = rand() % 25 + 2000; // Random year between 2000 and 2020
        int month = rand() % 12 + 1;   // Random month between 1 and 12
        int day = rand() % 28 + 1;     // Random day between 1 and 28
        tm->tm_year = year - 1900;     // Adjust year
        tm->tm_mon = month - 1;        // Adjust month
        tm->tm_mday = day;             // Adjust day
        strftime(files[i].creationDate, sizeof(files[i].creationDate), "%Y-%m-%d", tm);

        files[i].firstBlockIndex = -1; // Initialize to -1

        int blockCount = rand() % 4 + 2; // Random block length from 2 to 5
        int prevBlockIndex = -1;

        for (int b = 0; b < blockCount; b++)
        {
            int blockIndex = selectRandomUnoccupiedBlock();
            int row = blockIndex / COLUMNS;
            int col = blockIndex % COLUMNS;

            disk[row][col].occupied = 1;
            strcpy(disk[row][col].fileName, files[i].fileName);

            // Will update if another block follows
            disk[row][col].nextBlockIndex = -1;

            // Link from the previous block if not the first block
            if (prevBlockIndex != -1)
            {
                disk[prevBlockIndex / COLUMNS][prevBlockIndex % COLUMNS].nextBlockIndex = blockIndex;
            }
            else
            {
                // Set first block for the file
                files[i].firstBlockIndex = blockIndex;
            }

            prevBlockIndex = blockIndex;
        }
    }
}

void displayFiles()
{
    printf("\nFiles\n------");
    for (int i = 0; i < numberOfFiles; i++)
    {
        printf("\nName: %s, Creation Date: %s, First Block Index: %d\n",
               files[i].fileName, files[i].creationDate, files[i].firstBlockIndex);

        int nextIndex = files[i].firstBlockIndex;
        int fileSize = 0;

        while (nextIndex != -1)
        {
            DiskBlock *block = &disk[nextIndex / COLUMNS][nextIndex % COLUMNS];
            printf("    Block at [%d,%d], Next Block Index: %d\n",
                   nextIndex / COLUMNS, nextIndex % COLUMNS, block->nextBlockIndex);
            nextIndex = block->nextBlockIndex;
            fileSize++;
        }

        printf("File Size: %d blocks\n", fileSize);
    }
}

void displayDisk(int clearPrevious)
{

    if (clearPrevious)
    {
        for (int i = 0; i < ROWS; i++)
        {
            printf("\033[A\033[2K"); // move cursor up and clear line
        }
    }

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            printf("%s\t", disk[i][j].occupied ? disk[i][j].fileName : "x");
        }
        printf("\n");
    }
    // printf("\n");
}

// Define a new function to swap two blocks
void swapBlocks(int index1, int index2)
{
    // Swap blocks on the disk
    DiskBlock temp = disk[index1 / COLUMNS][index1 % COLUMNS];
    disk[index1 / COLUMNS][index1 % COLUMNS] = disk[index2 / COLUMNS][index2 % COLUMNS];
    disk[index2 / COLUMNS][index2 % COLUMNS] = temp;

    // Update next block indices
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if (disk[i][j].nextBlockIndex == index1)
                disk[i][j].nextBlockIndex = index2;
            else if (disk[i][j].nextBlockIndex == index2)
                disk[i][j].nextBlockIndex = index1;
        }
    }

    // Update first block indices in files
    for (int i = 0; i < numberOfFiles; i++)
    {
        if (files[i].firstBlockIndex == index1)
            files[i].firstBlockIndex = index2;
        else if (files[i].firstBlockIndex == index2)
            files[i].firstBlockIndex = index1;
    }
}

void defragment()
{
    displayDisk(0);
    // this keeps track of where we are writing to
    int writePos = 0;

    for (int i = 0; i < numberOfFiles; i++)
    {
        int currentBlockIndex = files[i].firstBlockIndex;
        while (currentBlockIndex != -1)
        {
            // Check if the current block is already in the correct position
            if (currentBlockIndex != writePos)
            {
                swapBlocks(writePos, currentBlockIndex);
                // After swapping, the current block's new position is writePos
                currentBlockIndex = writePos;
            }

            // Move to next block in the file
            currentBlockIndex = disk[currentBlockIndex / COLUMNS][currentBlockIndex % COLUMNS].nextBlockIndex;

            // Move to the next position in the disk
            writePos++;

            usleep(200000); // sleep for a short while
            displayDisk(1);
        }
    }
}

int main()
{
    initializeDisk();
    createFiles();

    printf("Initialised disk...\n");
    printf("Created starter files...\n");

    while (1)
    {
        printf("\nChoose an option:\n");
        printf("1. Display Files (mission 1)\n");
        printf("2. Display Disk\n");
        printf("3. Add a File (mission 3)\n");
        printf("4. Delete a File (mission 3)\n");
        printf("5. Defragment Disk (mission 2)\n");
        printf("6. Exit\n");

        int choice;
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            displayFiles();
            break;
        case 2:
            printf("\nDisk\n------\n");
            displayDisk(0);
            break;
        case 3:
            addNewFile();
            break;
        case 4:
            deleteFile();
            break;
        case 5:
            printf("\nDefragmenting disk...\n\n");
            defragment();
            printf("\nDisk defragmented.\n");
            break;
        case 6:
            return 0; // Exit the program
        default:
            printf("Invalid choice! Please enter a value between 1 and 6.\n");
        }
    }
    return 0;
}
