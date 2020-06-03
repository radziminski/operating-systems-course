#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define COPY_WRITE_BUFF_SIZE 2048

void help();
int copy_read_write(int fd_from, int fd_to);
int copy_mmap(int fd_from, int fd_to);
int check_arguments(int arg_num, bool m_flag);
int open_files(int *file_in, int *file_out, char *input_file, char *output_file);
int close_files(int *file_in, int *file_out);

int main(int argc, char *argv[])
{
    int opt = 0;
    bool m_flag = false;

    // Getting option arguments
    while ((opt = getopt(argc, argv, ":hm")) != -1)
    {
        switch (opt)
        {
        case 'm':
            m_flag = true;
            break;
        case 'h':
            help();
            return 0;
        case '?':
            printf("Unkonwn option provided. Please use -h for help.\n");
            return 1;
        }
    }

    // Handling too few or too many arguments
    if (check_arguments(argc, m_flag) != 0)
        return 1;

    // File names
    char *input_file_name = argv[1];
    char *output_file_name = argv[2];

    if (m_flag)
    {
        input_file_name = argv[2];
        output_file_name = argv[3];
    }

    // Opening files
    int input_fd, output_fd;

    if (open_files(&input_fd, &output_fd, input_file_name, output_file_name) != 0)
        return 1;

    // Performing copy
    int res = 0;
    if (m_flag)
        res = copy_mmap(input_fd, output_fd);
    else
        res = copy_read_write(input_fd, output_fd);

    // Closing files
    if (close_files(&input_fd, &output_fd) != 0)
        return 1;

    return res;
}

void help()
{
    printf("help\n");
}

int check_arguments(int arg_num, bool m_flag)
{
    if (arg_num <= 2 || (m_flag && arg_num <= 3))
    {
        printf("Too few arguments provided\n");
        help();
        return 1;
    }

    // Handling too many arguments
    if ((!m_flag && arg_num >= 4) || (m_flag && arg_num >= 5))
    {
        printf("Too many arguments provided\n");
        help();
        return 1;
    }

    return 0;
}

int open_files(int *file_in, int *file_out, char *input_file, char *output_file)
{
    // Opening input file
    *file_in = open(input_file, O_RDONLY);
    if (*file_in < 0)
    {
        perror("There was an error while opening the input file\n");
        return 1;
    }

    // Getting file status so i can copy it to output
    struct stat input_status;
    if (fstat(*file_in, &input_status) < 0)
    {
        perror("There was an error while loading input file status\n");
        return 1;
    }

    // Opening output file
    *file_out = open(output_file, O_RDWR | O_CREAT, input_status.st_mode);
    if (*file_out < 0)
    {
        perror("There was an error while opening the output file\n");
        return 1;
    }

    return 0;
}

int close_files(int *file_in, int *file_out)
{
    if (close(*file_in) < 0)
    {
        perror("There was an error while closing the input file\n");
        return 1;
    }

    if (close(*file_out) < 0)
    {
        perror("There was an error while closing the output file\n");
        return 1;
    }
    return 0;
}

int copy_mmap(int fd_from, int fd_to)
{
    struct stat input_status;
    if (fstat(fd_from, &input_status) == -1)
    {
        perror("There was an error while loading input file status\n");
        return 1;
    }

    char *input_buff = mmap(NULL, input_status.st_size, PROT_READ, MAP_SHARED, fd_from, 0);
    if (input_buff == (void *)-1)
    {
        perror("There was an error while mapping to memory\n");
        return 1;
    }

    int file_turncate_res = ftruncate(fd_to, input_status.st_size);
    if (file_turncate_res < 0)
    {
        perror("There was an error while turncating (changing to input size) output file size\n");
        return 1;
    }

    char *output_buff = mmap(NULL, input_status.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_to, 0);
    if (output_buff == (void *)-1)
    {
        perror("There was an error while mapping to memory\n");
        return 1;
    }

    // Copying the file
    char *copying_res = memcpy(output_buff, input_buff, input_status.st_size);
    if (copying_res == (void *)-1)
    {
        perror("There was an error while copying the file (memcpy)\n");
        return 1;
    }

    return 0;
}

int copy_read_write(int fd_from, int fd_to)
{
    int curr_read, curr_write;
    char buff[COPY_WRITE_BUFF_SIZE];

    while ((curr_read = read(fd_from, buff, COPY_WRITE_BUFF_SIZE)) > 0)
    {
        curr_write = write(fd_to, buff, curr_read);
        if (curr_write <= 0)
        {
            perror("There was an error while writing to the file\n");
            return 1;
        }
    }

    return 0;
}