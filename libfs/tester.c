#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "disk.h"

int main() {
    // Mount the file system
    printf("Mounting file system...\n");
    if (fs_mount("disk.fs") == -1) {
        printf("Failed to mount file system\n");
        return EXIT_FAILURE;
    }

    // Create a file
    printf("Creating a file...\n");
    if (fs_create("test.txt") == -1) {
        printf("Failed to create a file\n");
        fs_umount();
        return EXIT_FAILURE;
    }

    // Display information about the file system
    printf("Displaying information about the file system...\n");
    if (fs_info() == -1) {
        printf("Failed to display information about the file system\n");
        fs_umount();
        return EXIT_FAILURE;
    }

    // Unmount the file system
    printf("Unmounting file system...\n");
    if (fs_umount() == -1) {
        printf("Failed to unmount file system\n");
        return EXIT_FAILURE;
    }

    printf("Test completed successfully\n");

    return EXIT_SUCCESS;
}
