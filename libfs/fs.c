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
#define FAT_SIZE 2048

struct Superblock
{
	char signature[8]; // Signature equal to "ECS150FS"
	uint16_t total_blocks;
	uint16_t root_index;
	uint16_t data_start;   // Data block start index
	uint16_t data_blocks;  // Amount of data blocks
	uint8_t fat_blocks;	   // Number of blocks for FAT
	uint8_t padding[4079]; // Unused/Padding
} __attribute__((packed)); // Ensure correct layout

struct RDentries
{
	char filename[16];
	uint32_t size;
	uint16_t first_block_data;

	// copied code
	uint16_t last_data_blk;
	uint8_t open;
	char unused[7];
} __attribute__((packed));

struct RootDirectory
{
	// 32 byte entry per file (128 entries total)
	struct RDentries entries[FS_FILE_MAX_COUNT];
	char padding[10];
} __attribute__((packed));

struct FatBlock
{
	uint16_t entries[2048]; // Array of 16-bit entries
} __attribute__((packed));

// global variables
struct Superblock *superblock;
struct RootDirectory *root_directory;
struct FatBlock *fatblock;

// NUll character

int fs_mount(const char *diskname)
{
	// Open the virtual disk file
	if (block_disk_open(diskname) == -1)
	{
		fprintf(stderr, "Failed to open virtual disk file %s\n", diskname);
		return -1;
	}

	// Allocate memory for superblock and root_directory
	superblock = (struct Superblock *)malloc(sizeof(struct Superblock));
	root_directory = (struct RootDirectory *)malloc(sizeof(struct RootDirectory));

	if (block_read(0, superblock) == -1)
	{
		fprintf(stderr, "Failed to read superblock from disk\n");
		return -1;
	}

	if (block_read(superblock->root_index, root_directory->entries) < 0)
	{
		printf("fs_mount: read root dir error\n");
		return -1;
	}

	// Allocate memory for the FAT blocks
	fatblock = (struct FatBlock *)malloc(superblock->fat_blocks * sizeof(struct FatBlock));

	// Read each FAT block
	for (int i = 0; i < superblock->fat_blocks; i++)
	{
		if (block_read(4096 + i, &(fatblock[i].entries)) < 0)
		{
			printf("fs_mount: read FAT block %d error\n", i);
			return -1;
		}
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

	// Free allocated memory
	free(superblock);
	free(root_directory);

	return 0;
}

int fs_info(void)
{
	if (block_disk_count() == -1)
	{
		fprintf(stderr, "No underlying virtual disk was opened\n");
		return -1;
	}

	// count number of empty root directories
	int Num_empty_entries = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root_directory->entries[i].filename, "") == 0)
		{
			Num_empty_entries++;
		}
	}

	int fat_free_numerator;
	int Num_empty_fat_entries = 0;
	for (int i = 0; i < superblock->fat_blocks; i++)
	{
		for (size_t j = 0; j < sizeof(fatblock->entries) / sizeof(fatblock->entries[0]); j++)
		{
			if (fatblock->entries[j] == 0)
			{
				Num_empty_fat_entries++;
			}
		}
	}

	if (Num_empty_fat_entries == superblock->data_blocks)
	{
		fat_free_numerator = Num_empty_fat_entries - 1;
	}

	printf("FS INFO:\n");
	printf("total_blk_count=%d\n", block_disk_count());
	printf("fat_blk_count=%d\n", superblock->fat_blocks);
	printf("rdir_blk=%d\n", superblock->root_index);
	printf("data_blk=%d\n", superblock->data_start);
	printf("data_blk_count=%d\n", superblock->data_blocks);
	printf("fat_free_ratio=%d/%d\n", fat_free_numerator, superblock->data_blocks);
	printf("rdir_free_ratio=%d/%d\n", Num_empty_entries, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	// Find an empty entry in the root directory
	int empty_entry_index = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root_directory->entries[i].filename, "") == 0)
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
	strcpy(root_directory->entries[empty_entry_index].filename, filename);
	root_directory->entries[empty_entry_index].size = 0;
	root_directory->entries[empty_entry_index].first_block_data = FAT_EOC;

	// Free data blocks occupied by the file in the FAT
	// (This step would require additional implementation based on your file system structure)

	return 0;
}

void get_file_blocks(struct FatBlock fat_block, uint16_t start_index, uint16_t *blocks_array)
{
	uint16_t index = start_index;
	size_t num_blocks = 0;

	// Iterate through the FAT block until FAT_EOC is reached or the array is full
	while (index != FAT_EOC && num_blocks < FAT_SIZE)
	{
		// Store the block index in the array
		blocks_array[num_blocks] = index;
		num_blocks++; // Increment the number of blocks associated with the file

		// Move to the next block index
		index = fat_block.entries[index];
	}
}

int fs_delete(const char *filename)
{
	// Check if no file system is currently mounted
	if (root_directory == NULL || superblock == NULL)
	{
		return -1;
	}

	// Search for the file in the root directory
	int file_index = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root_directory->entries[i].filename, filename) == 0)
		{
			file_index = i;
			break;
		}
	}

	// Return -1 if the filename is invalid or not found
	if (file_index == -1)
	{
		return -1;
	}

	// uint16_t blocks_array[FAT_SIZE];

	// // Call get_file_blocks to populate the blocks_array
	// get_file_blocks(fatblock, root_directory->entries[file_index].first_block_data, blocks_array);

	// // Print the block indexes associated with the file
	// printf("Block indexes associated with the file:\n");
	// for (size_t i = 0; blocks_array[i] != FAT_EOC && i < FAT_SIZE; i++)
	// {
	// 	printf("%u ", blocks_array[i]);
	// }
	// printf("\n");

	// Clear the entry for the file
	strcpy(root_directory->entries[file_index].filename, "");
	root_directory->entries[file_index].size = 0;
	root_directory->entries[file_index].first_block_data = 0;

	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		// Check if the filename is empty, indicating an empty entry
		if (strcmp(root_directory->entries[i].filename, "") != 0)
		{

			printf("file: %s, ", root_directory->entries[i].filename);
			printf("Size: %u, ", root_directory->entries[i].size);
			printf("data_blk: %u\n", root_directory->entries[i].first_block_data);
		}
	}
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
	(void)fd;	  // Dummy variable to avoid unused parameter warning
	(void)offset; // Dummy variable to avoid unused parameter warning
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	(void)fd;	 // Dummy variable to avoid unused parameter warning
	(void)buf;	 // Dummy variable to avoid unused parameter warning
	(void)count; // Dummy variable to avoid unused parameter warning
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	(void)fd;	 // Dummy variable to avoid unused parameter warning
	(void)buf;	 // Dummy variable to avoid unused parameter warning
	(void)count; // Dummy variable to avoid unused parameter warning
	return 0;
}