#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FS_FILENAME_LEN 16
#define FS_FILE_MAX_COUNT 128
#define FAT_EOC 0xFFFF

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

struct RootDirectory
{
	char filename[16];
	uint32_t size;
	uint16_t first_block;
	char padding[10];
} __attribute__((packed));

struct RootDirectory root_directory[FS_FILE_MAX_COUNT];
int root_dir_count = 0;

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

int fs_umount(void)
{

	// Close the virtual disk file
	if (block_disk_close() == -1)
	{
		fprintf(stderr, "Failed to close virtual disk file\n");
		return -1;
	}

	return 0;
}

int fs_info(void)
{
	if (block_disk_count() == -1)
	{
		fprintf(stderr, "No underlying virtual disk was opened\n");
		return -1;
	}
	return 0;
}

int fs_create(const char *filename)
{
	// Find an empty entry in the root directory
	int empty_entry_index = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root_directory[i].filename, "") == 0)
		{
			empty_entry_index = i;
			break;
		}
	}

	// If no empty entry is found, return -1
	if (empty_entry_index == -1)
	{
		return -1;
	}

	// Fill out the empty entry with the new filename
	strcpy(root_directory[empty_entry_index].filename, filename);
	root_directory[empty_entry_index].size = 0;
	root_directory[empty_entry_index].first_block = FAT_EOC;

	// Increment the count of files in the root directory
	root_dir_count++;

	return 0;
}

int fs_delete(const char *filename)
{
    /* TODO: Phase 2 */
    (void)filename; // Dummy variable to avoid unused parameter warning
    return 0;
}

int fs_ls(void)
{
    /* TODO: Phase 2 */
    return 0;
}

int fs_open(const char *filename)
{
    /* TODO: Phase 3 */
    (void)filename; // Dummy variable to avoid unused parameter warning
    return 0;
}

int fs_close(int fd)
{
    /* TODO: Phase 3 */
    (void)fd; // Dummy variable to avoid unused parameter warning
    return 0;
}

int fs_stat(int fd)
{
    /* TODO: Phase 3 */
    (void)fd; // Dummy variable to avoid unused parameter warning
    return 0;
}

int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    (void)fd; // Dummy variable to avoid unused parameter warning
    (void)offset; // Dummy variable to avoid unused parameter warning
    return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    (void)fd; // Dummy variable to avoid unused parameter warning
    (void)buf; // Dummy variable to avoid unused parameter warning
    (void)count; // Dummy variable to avoid unused parameter warning
    return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    (void)fd; // Dummy variable to avoid unused parameter warning
    (void)buf; // Dummy variable to avoid unused parameter warning
    (void)count; // Dummy variable to avoid unused parameter warning
    return 0;
}