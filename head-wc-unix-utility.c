#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE 1048576  // 1 MB buffer

/**
 * Finds the length of a null-terminated string.
 *
 * @param str The input string.
 * @return The length of the string.
 */
size_t strLength(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * Counts newlines, words, and bytes in a buffer and updates the counts.
 *
 * @param buffer The character buffer to count from.
 * @param lines  Pointer to the line count.
 * @param words  Pointer to the word count.
 * @param bytes  Pointer to the byte count.
 */
void countLinesWordsBytes(const char *buffer, int *lines, int *words, int *bytes) {
    int inWord = 0;
    for (size_t i = 0; buffer[i] != '\0'; i++) {
        // Count bytes.
        (*bytes)++;
        // Check for newlines.
        if (buffer[i] == '\n') {
            (*lines)++;
        }
        // Check for word boundaries.
        if (isspace(buffer[i])) {
            inWord = 0;
        } else if (!inWord) {
            inWord = 1;
            (*words)++;
        }
    }
}

/**
 * The main function for the wc utility.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return 0 if the program runs successfully, otherwise an error code.
 */
int main(int argc, char *argv[]) {
    int showLines = 0;
    int showWords = 0;
    int showBytes = 0;
    int totalLines = 0;
    int totalWords = 0;
    int totalBytes = 0;
    int fileDescriptor;
    int option;
    int firstFile = 1;

    // Parse command-line options using getopt.
    while ((option = getopt(argc, argv, "clw")) != -1) {
        switch (option) {
            case 'c':
                showBytes = 1;  // Print byte count.
                break;
            case 'l':
                showLines = 1;  // Print line count.
                break;
            case 'w':
                showWords = 1;  // Print word count.
                break;
            default:
                write(STDERR_FILENO, "Invalid option\n", 15);
                exit(EXIT_FAILURE);
        }
    }

    // Determine the file descriptor to read from, considering standard input.
    if (optind < argc) {
        while (optind < argc) {
            if (strcmp(argv[optind], "-") == 0) {
                fileDescriptor = STDIN_FILENO;
            } else {
                // Open the specified file for reading.
                fileDescriptor = open(argv[optind], O_RDONLY);
                if (fileDescriptor == -1) {
                    write(STDERR_FILENO, "Error opening file\n", 19);
                    exit(EXIT_FAILURE);
                }
            }

            // Disable printf buffering to ensure unbuffered output.
            setbuf(stdout, NULL);

            char buffer[BUFFER_SIZE];
            ssize_t bytesRead;
            int lines = 1;
            int words = 1;
            int bytes = 1;

            // Read data from the file descriptor in chunks into the buffer.
            while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0) {
                if (showLines || showWords || showBytes || 
                    (!showLines && !showWords && !showBytes)) 
                {
                    // Count lines, words, and bytes if requested.
                    countLinesWordsBytes(buffer, &lines, &words, &bytes);
                }
            }
            
            if (!showLines && !showWords && !showBytes) {
                // Default case, print all
                printf("%d %d %d ", lines, words, bytes);
            }
            if (showLines) {
                // Print line count.
                printf("%d ", lines);
            }
            if (showWords) {
                // Print word count.
                printf("%d ", words);
            }
            if (showBytes) {
                // Print byte count.
                printf("%d ", bytes);
            }

            // Print the file name.
            write(STDOUT_FILENO, argv[optind], strLength(argv[optind]));
            write(STDOUT_FILENO, "\n", 1);

            // Accumulate the counts for the total.
            totalLines += lines;
            totalWords += words;
            totalBytes += bytes;

            // Close the file descriptor if not standard input.
            if (fileDescriptor != STDIN_FILENO) {
                close(fileDescriptor);
            }

            optind++;

            // If there are multiple files, print the total count after the first file.
            if (optind < argc && firstFile) {
                firstFile = 0;
            }
        }
    }

    // If there are multiple files, print the total count after last file
    if (!firstFile && (showLines || showWords || showBytes || 
            (!showLines && !showWords && !showBytes))) 
        {
        if (!showLines && !showWords && !showBytes) {
            // Default case, print all
            printf("%d %d %d ", totalLines, totalWords, totalBytes);
        }
        if (showLines) {
            // Print line count.
            printf("%d ", totalLines);
        }
        if (showWords) {
            // Print word count.
            printf("%d ", totalWords);
        }
        if (showBytes) {
            // Print byte count.
            printf("%d ", totalBytes);
        }
        write(STDOUT_FILENO, "total\n", 7);
    }

    return 0;
}
