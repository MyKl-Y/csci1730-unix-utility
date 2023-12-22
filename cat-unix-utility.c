int main(int argc, char *argv[]) {
    // Check if there are no arguments or if '-' is provided as an argument
    if (argc == 1 || (argc == 2 && argv[1][0] == '-' && argv[1][1] == '\0')) {
        // Read from standard input and write to standard output
        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, bytesRead);
        }
        if (bytesRead < 0) {
            // Handle read error
            perror("read");
            exit(EXIT_FAILURE);
        }
    } else {
        // Loop through each file provided as argument and print its content
        for (int i = 1; i < argc; ++i) {
            int fileDescriptor;

            if (argv[i][0] == '-' && argv[i][1] == '\0') {
                // Use standard input
                fileDescriptor = STDIN_FILENO;
            } else {
                fileDescriptor = open(argv[i], O_RDONLY);
                if (fileDescriptor == -1) {
                    // Handle file open error
                    perror("open");
                    exit(EXIT_FAILURE);
                }
            }

            char buffer[4096];
            ssize_t bytesRead;
            while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0) {
                write(STDOUT_FILENO, buffer, bytesRead);
            }

            if (bytesRead < 0) {
                // Handle read error
                perror("read");
                exit(EXIT_FAILURE);
            }

            if (fileDescriptor != STDIN_FILENO) {
                // Close the file after reading, but not for standard input
                close(fileDescriptor);
            }
        }
    }

    return 0;
}
