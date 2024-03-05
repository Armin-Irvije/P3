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

struct RootDirectory
{
	// 32 byte entry per file
	char filename[16];
	uint32_t size;
	uint16_t first_block_data;
	char padding[10];
} __attribute__((packed));

struct FatBlock
{
	uint16_t entry[FAT_SIZE]; // Array of 16-bit entries
} __attribute__((packed));

// global variables
struct Superblock *superblock;
struct RootDirectory root_directory[FS_FILE_MAX_COUNT]; // root directory array size 128
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
	if (block_read(0, superblock) == -1)
	{
		fprintf(stderr, "Failed to read superblock from disk\n");
		return -1;
	}

	if (block_read(superblock->root_index, root_directory) < 0)
	{
		printf("fs_mount: read root dir error\n");
		return -1;
	}

	// Allocate memory for the FAT blocks
	fatblock = (struct FatBlock *)malloc((superblock->fat_blocks) * BLOCK_SIZE); // multipying # of fat blocks for correct allocation
	// Read each FAT block and start count at 1
	for (int i = 1; i <= superblock->fat_blocks; i++)
	{
		if (block_read(i, (char *)fatblock + BLOCK_SIZE * (i - 1)))
		{
			printf("fs_mount: read FAT block %d error\n", i);
			return -1;
		}
	}

	return 0;
}

// whenever fs_umount() is called, all meta-information and file data must have been written out to disk.
int fs_umount(void)
{

	if (block_write(superblock->root_index, root_directory) < 0)
	{
		printf("fs_mount: read root dir error\n");
		return -1;
	}

	for (int i = 1; i <= superblock->fat_blocks; i++)
	{
		if (block_write(i, (char *)fatblock + BLOCK_SIZE * (i - 1)))
		{
			printf("fs_mount: read FAT block %d error\n", i);
			return -1;
		}
	}

	// Close the virtual disk file
	if (block_disk_close() == -1)
	{
		fprintf(stderr, "Failed to close virtual disk file\n");
		return -1;
	}

	// Free allocated memory
	free(superblock);

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
		if (strcmp(root_directory[i].filename, "") == 0)
		{
			Num_empty_entries++;
		}
	}

	int fat_free_numerator = 0;
	for (int i = 0; i < superblock->fat_blocks; i++)
	{
		for (int j = 0; j < FAT_SIZE; j++)
		{
			if (fatblock[i].entry[j] == 0)
			{
				fat_free_numerator++;
			}
		}
	}

	// root_directory->entries[empty_entry_index].first_block_data = FAT_EOC;
	// fatblock->entry[2049] = 69;

	// PRINT FAT BLOCKS
	// for (int block_index = 0; block_index < superblock->fat_blocks; block_index++)
	// {
	// 	printf("Block %d:\n", block_index);

	// 	// Get the current FAT block
	// 	struct FatBlock current_block = fatblock[block_index];

	// 	// Iterate over each entry within the block
	// 	for (int entry_index = 0; entry_index < FAT_SIZE; entry_index++)
	// 	{
	// 		if (current_block.entry[entry_index] == FAT_EOC || current_block.entry[entry_index] != 0)
	// 		{
	// 			printf("Entry %d: %hu\n", entry_index, current_block.entry[entry_index]);
	// 		}
	// 	}
	// }
	// /// PRINT FAT BLOCKS

	// for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	// {
	// 	// Check if the filename is not empty
	// 	if (root_directory[i].filename[0] != '\0')
	// 	{
	// 		printf("Entry %d: first_block_data = %hu\n", i, root_directory[i].first_block_data);
	// 	}
	// }

	printf("FS Info:\n");
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
	root_directory[empty_entry_index].first_block_data = FAT_EOC;

	// Free data blocks occupied by the file in the FAT
	// (This step would require additional implementation based on your file system structure)

	return 0;
}

void clear_fat_entries(struct FatBlock *fatblock, uint16_t entry_index)
{
	uint16_t index = entry_index;

	// Iterate through the FAT entries until FAT_EOC is encountered
	while (fatblock->entry[index] != FAT_EOC)
	{
		// Store the current FAT entry
		uint16_t current_entry = fatblock->entry[index];

		// Clear the current FAT entry by setting it to zero
		fatblock->entry[index] = 0;

		// Move to the next FAT entry
		index = current_entry;
	}

	// Check if the current entry is FAT_EOC
	if (fatblock->entry[index] == FAT_EOC)
	{
		// If it is, set the FAT_EOC entry to zero and break out of the loop
		fatblock->entry[index] = 0;
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
		if (strcmp(root_directory[i].filename, filename) == 0)
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

	clear_fat_entries(fatblock, root_directory[file_index].first_block_data);

	// Clear the entry for the file
	strcpy(root_directory[file_index].filename, "");
	root_directory[file_index].size = 0;
	root_directory[file_index].first_block_data = 0;

	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		// Check if the filename is empty, indicating an empty entry
		if (strcmp(root_directory[i].filename, "") != 0)
		{

			printf("file: %s, ", root_directory[i].filename);
			printf("Size: %u, ", root_directory[i].size);
			printf("data_blk: %u\n", root_directory[i].first_block_data);
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