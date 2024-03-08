#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
#define FAT_SIZE 2048
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct Superblock
{
	char signature[8]; // Signature equal to "ECS150FS"
	uint16_t total_blocks;
	uint16_t root_index;
	uint16_t data_start;
	uint16_t data_blocks;
	uint8_t fat_blocks;
	uint8_t padding[4079];
} __attribute__((packed));

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

struct FileDescriptor
{
	char filename[FS_FILENAME_LEN];
	int offset;
};

// global variables
struct Superblock *superblock;
struct RootDirectory root_directory[FS_FILE_MAX_COUNT]; // root directory array size 128
struct FatBlock *fatblock;
static struct FileDescriptor fileD[FS_OPEN_MAX_COUNT];

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

	if (block_disk_close() == -1)
	{
		fprintf(stderr, "Failed to close virtual disk file\n");
		return -1;
	}

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
	for (int i = 0; i < superblock->data_blocks; i++)
	{

		if (fatblock->entry[i] == 0) // find an fat entry that is free
		{
			fat_free_numerator++;
		}
	}

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

int get_file_size(const char *filename)
{
	// Open the file
	FILE *file = fopen(filename, "rb");
	if (file == NULL)
	{
		perror("Error opening file");
		return -1;
	}

	// Move the file pointer to the end of the file
	if (fseek(file, 0, SEEK_END) != 0)
	{
		perror("Error seeking file");
		fclose(file);
		return -1;
	}

	long file_size = ftell(file);
	if (file_size == -1L)
	{
		perror("Error getting file size");
		fclose(file);
		return -1;
	}

	fclose(file);
	return file_size;
}

void fill_fat_entries(struct FatBlock *fatblock, uint16_t start_index, uint16_t end_index)
{
	uint16_t current_index = start_index;

	while (current_index != end_index)
	{

		uint16_t next_index = current_index + 1;

		// Set the current FAT entry to point to the next index
		fatblock->entry[current_index] = next_index;
		// Move to the next FAT entry
		current_index = next_index;
	}

	fatblock->entry[end_index] = FAT_EOC;
}

int fs_create(const char *filename)
{
	if (strlen(filename) * sizeof(char) > FS_FILENAME_LEN)
	{
		return -1;
	}

	int empty_entry_index = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		// its already in the directory
		if (strcmp(root_directory[i].filename, filename) == 0)
		{
			return -1;
		}
		if (strcmp(root_directory[i].filename, "") == 0)
		{
			empty_entry_index = i;
			break;
		}
	}

	if (empty_entry_index == -1)
	{
		return -1;
	}

	strcpy(root_directory[empty_entry_index].filename, filename);

	int file_size = get_file_size(filename);
	if (file_size == -1)
	{
		return -1;
	}

	if (file_size == 0)
	{
		// for zero sized files, no FAT blocks need to be allocated
		root_directory[empty_entry_index].size = 0;
		root_directory[empty_entry_index].first_block_data = FAT_EOC;
		return 0;
	}

	// Calculate the number of blocks required to store the file
	int num_blocks = file_size / BLOCK_SIZE;
	if (file_size % BLOCK_SIZE != 0)
	{
		num_blocks++;
	}

	// Find an empty slot in the FAT to start writing the file
	int start_block = -1;
	for (int j = 0; j < FAT_SIZE * superblock->fat_blocks; j++)
	{
		if (fatblock->entry[j] == 0)
		{
			start_block = j;
			break;
		}
	}
	if (start_block == -1)
	{
		return -1;
	}

	root_directory[empty_entry_index].size = file_size; // Write the file size and start block information to the root directory entry
	root_directory[empty_entry_index].first_block_data = start_block;
	fill_fat_entries(fatblock, start_block, start_block + num_blocks - 1);

	return 0;
}

void clear_fat_entries(struct FatBlock *fatblock, uint16_t entry_index)
{
	uint16_t index = entry_index;

	// Iterate through the FAT entries until FAT_EOC is encountered
	while (fatblock->entry[index] != FAT_EOC)
	{
		uint16_t current_entry = fatblock->entry[index];
		fatblock->entry[index] = 0;
		index = current_entry;
	}

	if (fatblock->entry[index] == FAT_EOC) // Check if the current entry is EOC
	{
		// set the FAT_EOC entry to zero and break out of the loop
		fatblock->entry[index] = 0;
	}
}

int fs_delete(const char *filename)
{

	if (root_directory == NULL || superblock == NULL)
	{
		return -1;
	}

	int file_index = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) // search for the file in the root directory
	{
		if (strcmp(root_directory[i].filename, filename) == 0)
		{
			file_index = i;
			break;
		}
	}

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
	if (block_disk_count() == -1)
	{
		fprintf(stderr, "No underlying virtual disk was opened\n");
		return -1;
	}

	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		// Check if an empty entry
		if (strcmp(root_directory[i].filename, "") != 0)
		{

			printf("file: %s, ", root_directory[i].filename);
			printf("Size: %u, ", root_directory[i].size);
			printf("data_blk: %u\n", root_directory[i].first_block_data);
		}
	}
	return 0;
}

static int numOpen = 0;
int fs_open(const char *filename)
{

	// error checking
	if (numOpen > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		// check if the filename equals the arguments passed
		if (strcmp(root_directory[i].filename, filename) != 0)
		{
			// iterate throught the fd array
			for (int j = 0; j < FS_OPEN_MAX_COUNT; j++)
			{

				if (strcmp(fileD[j].filename, "") == 0)
				{ // check an empty spot
					numOpen++;
					strcpy(fileD[j].filename, filename);
					fileD[j].offset = 0;

					return j;
				}
			}
		}
	}

	return -1;
}

int fs_close(int fd)
{
	// Check if the file descriptor is within range
	if (fd < 0 || fd >= FS_OPEN_MAX_COUNT)
	{
		return -1;
	}
	// Reset values associated with the file descriptor
	strcpy(fileD[fd].filename, "");
	fileD[fd].offset = 0;
	numOpen--;

	return 0;
}

// get offset/size here
int fs_stat(int fd)
{
	// error check
	if (superblock == NULL || root_directory == NULL || fatblock == NULL)
	{
		return -1;
	}

	// error check
	if (fd < 0 || fd >= FS_OPEN_MAX_COUNT || strcmp(fileD[fd].filename, "") == 0)
	{
		return -1;
	}

	const char *filename = fileD[fd].filename;

	// Search for the file in the root directory
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root_directory[i].filename, filename) == 0)
		{
			// return its size
			return root_directory[i].size;
		}
	}

	// File not found
	return -1;
}

// actually change offset here
int fs_lseek(int fd, size_t offset)
{
	// Check if the file descriptor is valid
	if (fd < 0 || fd >= FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	fileD[fd].offset = offset;
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{

	if (superblock == NULL || root_directory == NULL || fatblock == NULL)
	{
		return -1;
	}

	if (fd < 0 || fd >= FS_OPEN_MAX_COUNT || strcmp(fileD[fd].filename, "") == 0)
	{
		return -1;
	}

	if (buf == NULL)
	{
		return -1;
	}

	// Retrieve the filename associated with the fd
	const char *filename = fileD[fd].filename;
	size_t start_offset = fileD[fd].offset;

	size_t start_block;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{

		if (strcmp(root_directory[i].filename, filename) == 0)
		{
			start_block = root_directory[i].first_block_data;
		}
	}

	// Calculate the starting and ending block indices for the write operation
	size_t num_blocks = (count + BLOCK_SIZE - 1) / BLOCK_SIZE; // Round up division
	size_t end_block = start_block + num_blocks - 1;
	size_t bytes_written = 0;
	size_t remaining_bytes = count;
	void *current_buf = buf;

	for (size_t block_index = start_block; block_index <= end_block; block_index++)
	{
		// calculate the offset for writing
		size_t block_offset = (block_index == start_block) ? (start_offset % BLOCK_SIZE) : 0;
		// find the number of bytes to write
		size_t bytes_to_write = MIN(BLOCK_SIZE - block_offset, remaining_bytes);
		// Write data from the buffer to the block on disk
		if (block_write(superblock->data_start + block_index, (char *)current_buf) < 0)
		{
			return -1;
		}

		// Update everything
		bytes_written += bytes_to_write;
		remaining_bytes -= bytes_to_write;
		current_buf += bytes_to_write;
	}

	fileD[fd].offset += bytes_written;
	return bytes_written;
}

int fs_read(int fd, void *buf, size_t count)
{
	// error checking
	if (superblock == NULL || root_directory == NULL || fatblock == NULL)
	{
		return -1;
	}

	if (fd < 0 || fd >= FS_OPEN_MAX_COUNT || strcmp(fileD[fd].filename, "") == 0)
	{
		return -1;
	}

	if (buf == NULL)
	{
		return -1;
	}

	// Retrieve the filename associated with the file descriptor
	const char *filename = fileD[fd].filename;
	// Get the starting offset and size
	size_t start_offset = fileD[fd].offset;
	size_t file_size = get_file_size(filename);

	// doesn't exceed the file size
	if (start_offset + count > file_size)
	{
		count = file_size - start_offset;
	}

	size_t start_block;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{

		if (strcmp(root_directory[i].filename, filename) == 0)
		{
			start_block = root_directory[i].first_block_data;
		}
	}

	// Calculate the starting and ending block indices for the read operation
	size_t num_blocks = file_size / BLOCK_SIZE;
	size_t end_block = start_block + num_blocks;

	size_t bytes_read = 0;
	size_t remaining_bytes = count;
	void *current_buf = buf;
	void *bounce_buf = malloc(BLOCK_SIZE); // allocate memory

	for (size_t block_index = start_block; block_index <= end_block; block_index++)
	{

		size_t block_offset = (block_index == start_block) ? (start_offset % BLOCK_SIZE) : 0;
		size_t bytes_to_read = MIN(BLOCK_SIZE - block_offset, remaining_bytes);
		// Read data from the block into the bounce buffer
		if (block_read(superblock->data_start + block_index, bounce_buf) < 0)
		{
			free(bounce_buf);
			return -1;
		}

		// Copy data from the bounce buffer to the user
		memcpy((char *)current_buf, (char *)bounce_buf + block_offset, bytes_to_read);

		// update everything
		bytes_read += bytes_to_read;
		remaining_bytes -= bytes_to_read;
		current_buf += bytes_to_read;
	}

	fileD[fd].offset += bytes_read;
	free(bounce_buf);
	return bytes_read;
}