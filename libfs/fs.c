#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

struct Superblock
{
	char signature[8];	   // Signature (must be equal to "ECS150FS")
	uint16_t total_blocks; // Total amount of blocks of virtual disk
	uint16_t root_index;   // Root directory block index
	uint16_t data_start;   // Data block start index
	uint16_t data_blocks;  // Amount of data blocks
	uint8_t fat_blocks;	   // Number of blocks for FAT
	uint8_t padding[4079]; // Unused/Padding
} __attribute__((packed)); // Ensure packed attribute for correct layout

int fs_mount(const char *diskname)
{
	// Open the virtual disk file
	if (block_disk_open(diskname) == -1)
	{
		fprintf(stderr, "Failed to open virtual disk file %s\n", diskname);
		return -1;
	}

	// Read the superblock from the disk
	struct Superblock superblock;
	if (block_read(0, &superblock) == -1)
	{
		fprintf(stderr, "Failed to read superblock from disk\n");
		return -1;
	}

	return 0;
}

int fs_umount(void) {
	
    // Close the virtual disk file
    if (block_disk_close() == -1) {
        fprintf(stderr, "Failed to close virtual disk file\n");
        return -1;
    }

    return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}
