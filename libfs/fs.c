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
struct Superblock superblock;
struct RootDirectory root_directory; //
struct FatBlock fatblock;

int fs_mount(const char *diskname)
{
	// Open the virtual disk file
	if (block_disk_open(diskname) == -1)
	{
		fprintf(stderr, "Failed to open virtual disk file %s\n", diskname);
		return -1;
	}

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

	// count number of empty root directories
	int Num_empty_entries = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) // less than because 0 - 127 is 128 entries
	{
		if (strcmp(root_directory.entries[i].filename, "") == 0)
		{
			Num_empty_entries++;
		}
	}

	int fat_free_numerator;
	int Num_empty_fat_entries = 0;
	for (int i = 0; i < superblock.fat_blocks; i++) // less than because 0 - 127 is 128 entries
	{
		for (size_t i = 0; i < sizeof(fatblock.entries) / sizeof(fatblock.entries[0]); i++)
		{
			if (fatblock.entries[i] == 0)
			{
				Num_empty_fat_entries++;
			}
		}
	}

	if (Num_empty_fat_entries == superblock.data_blocks)
	{
		fat_free_numerator = Num_empty_fat_entries - 1;
	}

	printf("FS INFO:\n");
	printf("total_blk_count=%d\n", block_disk_count());
	printf("fat_blk_count=%d\n", superblock.fat_blocks);
	printf("rdir_blk=%d\n", superblock.root_index);
	printf("data_blk=%d\n", superblock.data_start);
	printf("data_blk_count=%d\n", superblock.data_blocks);
	printf("fat_free_ratio=%d/%d\n", fat_free_numerator, superblock.data_blocks);
	printf("rdir_free_ratio=%d/%d\n", Num_empty_entries, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	// Find an empty entry in the root directory
	int empty_entry_index = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root_directory.entries[i].filename, "") == 0)
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
	strcpy(root_directory.entries[empty_entry_index].filename, filename);
	root_directory.entries[empty_entry_index].size = 0;
	root_directory.entries[empty_entry_index].first_block_data = FAT_EOC;

	
	printf("Filename: %s\n", root_directory.entries[empty_entry_index].filename);
    printf("Size: %u\n", root_directory.entries[empty_entry_index].size);
    printf("First Block: %u\n", root_directory.entries[empty_entry_index].first_block_data);
    printf("\n");

	printf("index =%d\n", empty_entry_index);
	


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
	 for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        // Check if the filename is empty, indicating an empty entry
		printf("Filename: %s\n", root_directory.entries[0].filename);
		break;
        if (strcmp(root_directory.entries[i].filename, "") != 0) {
			printf("we are here\n");
            printf("Filename: %s\n", root_directory.entries[i].filename);
            printf("Size: %u\n", root_directory.entries[i].size);
            printf("First Block: %u\n", root_directory.entries[i].first_block_data);
            printf("\n");
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