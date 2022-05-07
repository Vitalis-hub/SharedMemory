#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <vector>
using namespace std;

#define MAX_BUFFERS 4000
char *addr;
#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_BUFFER_COUNT_NAME "/sem-buffer-count"
#define SEM_SPOOL_SIGNAL_NAME "/sem-spool-signal"
#define SHARED_MEM_NAME "/posix-shared-mem-example"

struct shared_memory {
    char buf [MAX_BUFFERS] [1000];
    int buffer_index;
    int buffer_print_index;
};

void error(char * msg);
void error (char *msg)
{
    perror (msg);
    exit (1);
}

vector<string> sendPacketData(vector<string> data){
    struct shared_memory *shared_mem_ptr;
    sem_t *mutex_sem, *buffer_count_sem, *spool_signal_sem;
    int fd_shm;
    char mybuf [1000];

    if((mutex_sem = sem_open (SEM_MUTEX_NAME, 0)) == SEM_FAILED){
        error("sem_open");
    }
    cout << mutex_sem << endl;

    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR, 0)) == -1) {
        error ("shm_open");
    }

    if ((shared_mem_ptr = (shared_memory*) mmap (NULL, sizeof (struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        error ("mmap");
    }
    shared_mem_ptr -> buffer_index = shared_mem_ptr -> buffer_print_index = 0;

    if((buffer_count_sem = sem_open(SEM_BUFFER_COUNT_NAME, 0, 0, 0)) == SEM_FAILED){
        error("sem_open");
    }
    cout << "here.." << endl;
    if ((spool_signal_sem = sem_open (SEM_SPOOL_SIGNAL_NAME, 0)) == SEM_FAILED) {
        error ("sem_open");
    }

    char *cp;
    for(int i =0; i < data.size(); i++){
        char buf [1000];
        const char *c = data[i].c_str();
        strncpy(buf,c, data[i].length());
        int length = strlen(buf);
        // if(buf[length - 1] == '\n'){
        //     buf[length - 1] = '0';
        // }

        if(sem_wait(buffer_count_sem) == -1){
            error("semi_wait: buffer_count_sem");
        }
        
        if(sem_wait(mutex_sem) == -1){
            error("sem_wait: mutex_sem");
        }

        cout << "mutex " << endl;
        string s = "";
        for (int j = 0; j < 1000; j++) {
            s = s + buf[j];
        }
        cout << s << endl;
        sprintf (shared_mem_ptr -> buf [shared_mem_ptr -> buffer_index], "%s\n", buf);
        (shared_mem_ptr -> buffer_index)++;
        if(shared_mem_ptr -> buffer_index == MAX_BUFFERS){
            shared_mem_ptr -> buffer_index = 0;
        }

        if(sem_post(mutex_sem) == -1){
            error("sem_post: mutex_sem");
        }

        if(sem_post(spool_signal_sem) == -1){
            error("sem_post: (spool_signal_sem");
        }
        cout << i << endl;
        
        if( i + 1 == data.size()){
            char buf [1000];
            if(sem_wait(buffer_count_sem) == -1){
                error("semi_wait: buffer_count_sem");
            }
            
            if(sem_wait(mutex_sem) == -1){
                error("sem_wait: mutex_sem");
            }
            string end = "END~";
            const char *c = end.c_str();
            strncpy(buf, c, end.length());
            int length = strlen(buf);
            // if(buf[length - 1] == '\n'){
            //     buf[length - 1] = '0';
            // }
            sprintf (shared_mem_ptr -> buf [shared_mem_ptr -> buffer_index], "%s", buf);
            (shared_mem_ptr -> buffer_index)++;
            if(shared_mem_ptr -> buffer_index == MAX_BUFFERS){
                shared_mem_ptr -> buffer_index = 0;
            }
            if(sem_post(mutex_sem) == -1){
            error("sem_post: mutex_sem");
            }

            if(sem_post(spool_signal_sem) == -1){
                error("sem_post: (spool_signal_sem");
            }
            cout << "END here is recognized" << endl;
        }
    }
    vector<string> v3;
    cout << "The string that contains target are: " << endl;
    while(1){
        if (sem_wait (spool_signal_sem) == -1)
            error ("sem_wait: spool_signal_sem");
        strcpy (mybuf, shared_mem_ptr -> buf [shared_mem_ptr -> buffer_print_index]);
        (shared_mem_ptr -> buffer_print_index)++;
        if (shared_mem_ptr -> buffer_print_index == MAX_BUFFERS)
           shared_mem_ptr -> buffer_print_index = 0;
        int length = strlen(mybuf);
        // if(mybuf[length - 1] == '0'){
        //     mybuf[length - 1] = '\n';
        // }
        std::string s(mybuf);
        if(s.rfind("END~", 0) == 0) {
            cout << "END Here is recognized2" << endl;
            break;
        }
        cout << s.length() << endl;
        v3.push_back(s);
        if (sem_post (buffer_count_sem) == -1)
            error ("sem_post: buffer_count_sem");
    }
    if (munmap (shared_mem_ptr, sizeof (struct shared_memory)) == -1)
        error ("munmap");
    sem_unlink(SEM_MUTEX_NAME); // Note sem_close() is invoked automatically at exit
    sem_unlink(SEM_BUFFER_COUNT_NAME);
    sem_unlink(SEM_SPOOL_SIGNAL_NAME);
    return v3;
    
}
