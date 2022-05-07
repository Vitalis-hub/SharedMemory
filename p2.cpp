#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <pthread.h>
#define MAX_BUFFERS 4000
#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_BUFFER_COUNT_NAME "/sem-buffer-count"
#define SEM_SPOOL_SIGNAL_NAME "/sem-spool-signal"
#define SHARED_MEM_NAME "/posix-shared-mem-example"
pthread_mutex_t a_mutex = PTHREAD_MUTEX_INITIALIZER;

using std::endl; using std::string;
using namespace std;

struct shared_memory {
    char buf [MAX_BUFFERS] [1000];
    int buffer_index;
    int buffer_print_index;
};


struct shared_memory *shared_mem_ptr;
sem_t *mutex_sem, *buffer_count_sem, *spool_signal_sem;
int fd_shm, fd_log;
int count_op = 0;
vector<string> result_to_send;

void error (char *msg);


void error (char *msg)
{
    perror (msg);
    exit (1);
}


string WHITESPACE = "\n";
 
    string rtrim(const string &s)
    {
        size_t end = s.find_last_not_of(WHITESPACE);
        return (end == std::string::npos) ? "" : s.substr(0, end + 1);
    }

int containWordIgnoreCase(const char *line, const char *word, size_t n) {
    size_t m = strlen(word);

    char *lower_line = (char*) calloc(n+1, sizeof(char));
    for (size_t i = 0; i < n; ++i) {
       lower_line[i] = tolower((unsigned char)line[i]);
    }
    char *lower_word = (char*) calloc(m+1, sizeof(char));
    for (size_t i = 0; i < m; ++i) {
       lower_word[i] = tolower((unsigned char)word[i]);
    } 

    //printf("%s, %s\n", lower_line, lower_word);
    char * ret = strstr (lower_line, lower_word);
    //printf("%s\n", ret);
    int return_val  = 0;
    if ( ret != NULL )
        if ( strlen(ret)==m || !isalnum(ret[m]))
        {
              if(ret == lower_line)
                return_val = 1;
              else
              {ret--;
                 if (!isalnum(ret[0]))
                    return_val = 1;
               }
        }
    free(lower_line);
    free(lower_word);

    return return_val;
}

typedef struct{
    vector<string> v_tmp;
    const char * target;
    int begin;
    int end;
    vector<string> res_vec;
} MY_ARGS;

typedef struct{
    vector<string> v_tmp;
    int begin;
    int end;
    vector<int> res;
} MYARGS;


MY_ARGS args1;
MY_ARGS args2;
MY_ARGS args3;
MY_ARGS args4;
void * reducer(MYARGS * args){
    cout << "Called Reducer--->" << endl;
    MYARGS* my_args = (MYARGS *) args;
    vector<string> v_tmp = my_args -> v_tmp;
    int first = my_args -> begin;
    int last = my_args -> end;
 
    int i = first;
    int counter = 0;
    char buf[1000],*cp;
    
    for (; i<last; i++){
        strcpy(buf, v_tmp[i].c_str());
        cout << v_tmp[i] << endl;
        int length = strlen (buf);
        // if (buf [length - 1] == '\n')
        //    buf [length - 1] = '0';
        
        if (sem_wait (buffer_count_sem) == -1)
            error ("sem_wait: buffer_count_sem");

        if (sem_wait (mutex_sem) == -1)
            error ("sem_wait: mutex_sem");

        sprintf (shared_mem_ptr -> buf [shared_mem_ptr -> buffer_index], "%s\n", buf);
        (shared_mem_ptr -> buffer_index)++;
        if (shared_mem_ptr -> buffer_index == MAX_BUFFERS)
            shared_mem_ptr -> buffer_index = 0;
        if (sem_post (mutex_sem) == -1)
            error ("sem_post: mutex_sem");
    
        if (sem_post (spool_signal_sem) == -1)
            error ("sem_post: (spool_signal_sem");

        if(i + 1 ==last){
            char buf [1000];
            string end = "END~";
            cout << end << endl;
            const char * c = end.c_str();
            strncpy(buf, c, end.length());
            int length = strlen(buf);
            // if(buf[length - 1] == '\n'){
            //     buf[length - 1] = '0';
            // }
            if (sem_wait (buffer_count_sem) == -1)
            error ("sem_wait: buffer_count_sem");

            if (sem_wait (mutex_sem) == -1)
                error ("sem_wait: mutex_sem");
            sprintf (shared_mem_ptr -> buf [shared_mem_ptr -> buffer_index], "%s\n", buf);
            (shared_mem_ptr -> buffer_index)++;
            if(shared_mem_ptr -> buffer_index == MAX_BUFFERS){
                shared_mem_ptr -> buffer_index = 0;
            }
            if (sem_post (mutex_sem) == -1)
                error ("sem_post: mutex_sem");
    
            if (sem_post (spool_signal_sem) == -1)
                error ("sem_post: (spool_signal_sem");
        }
        my_args -> res.push_back(i);
    }
    return NULL;
}

void * find_and_output(void* args){
    cout << "Called" << endl;
    MY_ARGS* my_args = (MY_ARGS*) args;
    vector<string> v_tmp = my_args -> v_tmp;
    const char * target = my_args -> target;
    int first = my_args -> begin;
    int last = my_args -> end;
    cout << "Mid-->" << endl;
    int i = first;
    cout << i << endl;
    
    int counter = 0;
    for (; i<last; i++){
        const char * tmp_line = v_tmp[i].c_str();
        if (containWordIgnoreCase(tmp_line, target, strlen(tmp_line))) {
            my_args -> res_vec.push_back(v_tmp[i]);
            //result_to_send.push_back(v_tmp[i]);
        }
        if(i+1 == last){
            pthread_mutex_lock(&a_mutex );
            ++count_op;
            if(count_op == 4){
                count_op = 0;
                vector<string> res;
                for(vector<string>::iterator t = args1.res_vec.begin(); t!=args1.res_vec.end(); t++){
                    res.push_back(*t);
                }

                for(vector<string>::iterator t = args2.res_vec.begin(); t!=args2.res_vec.end(); t++){
                    res.push_back(*t);
                }

                for(vector<string>::iterator t = args3.res_vec.begin(); t!=args3.res_vec.end(); t++){
                    res.push_back(*t);
                }

                for(vector<string>::iterator t = args4.res_vec.begin(); t!=args4.res_vec.end(); t++){
                    res.push_back(*t);
                }
                cout << "Called Reducer" << endl;
                MYARGS args5 = {res, 0, res.size()};
                reducer(&args5);
            }
            pthread_mutex_unlock( &a_mutex );
        }
    }
    cout << "Got to the end!" << endl;
    return NULL;
}


void Manager(string targetWord){
    char mybuf[1000];
    
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        error ("sem_open");

    if((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT , 0660)) == -1){
        error("shm_open");
    }

    if(ftruncate(fd_shm, sizeof(struct shared_memory)) == -1){
        error("ftruncate");
    }

    if ((shared_mem_ptr = (shared_memory*) (mmap (NULL, sizeof (struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0))) == MAP_FAILED) {
        error ("mmap");
    }
    shared_mem_ptr -> buffer_index = shared_mem_ptr -> buffer_print_index = 0;
    
    if ((buffer_count_sem = sem_open (SEM_BUFFER_COUNT_NAME, O_CREAT, 0660, MAX_BUFFERS)) == SEM_FAILED)
        error ("sem_open");
    cout << "here" << endl;

    if ((spool_signal_sem = sem_open (SEM_SPOOL_SIGNAL_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        error ("sem_open");
    
    if (sem_post (mutex_sem) == -1)
        error ("sem_post: mutex_sem");
    vector<string> v3;
    // cout << (string) (v3[v3.size() - 1]) << endl;
    int i  = 0;
    while(1){
        if (sem_wait (spool_signal_sem) == -1)
            error ("sem_wait: spool_signal_sem");
        strcpy (mybuf, shared_mem_ptr -> buf [shared_mem_ptr -> buffer_print_index]);
        (shared_mem_ptr -> buffer_print_index)++;
        if (shared_mem_ptr -> buffer_print_index == MAX_BUFFERS)
           shared_mem_ptr -> buffer_print_index = 0;
        std::string s(mybuf);

        if(s.rfind("END~", 0) == 0) {
            cout << "END Here is recognized" << endl;
            break;
        }
        i++;
        int length = strlen(mybuf);
        //  if(mybuf[length - 1] == '0'){
        //     mybuf[length - 1] = '\n';
        // }
        cout << s << endl;
        cout << s.length() << endl;
        s = rtrim(s);
        v3.push_back(s);

        if (sem_post (buffer_count_sem) == -1) {
            error ("sem_post: buffer_count_sem");
        }
        
    }
    int how_many = v3.size();

    pthread_t th1;
    pthread_t th2;
    pthread_t th3;
    pthread_t th4;
    char *target = new char[targetWord.length() + 1];
    strcpy(target, targetWord.c_str());
    int first_par = (int)how_many/4;
    int second_par = (int) how_many / 2;
    int third_par = (int) (how_many * 0.75);
    args1 = {v3, target, 0, first_par};
    args2 = {v3, target, first_par, second_par};
    args3 = {v3, target, second_par, third_par};
    args4 = {v3, target, third_par, how_many};
    pthread_create(&th1, NULL, find_and_output, &args1);
    pthread_create(&th2, NULL, find_and_output, &args2);
    pthread_create(&th3, NULL, find_and_output, &args3);
    pthread_create(&th4, NULL, find_and_output, &args4);
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);
    pthread_join(th4, NULL);

}
struct CommandLineException {
    CommandLineException(int m,int a)
    {   cout << endl;
        cout << "Too many file names on the command line.";
        cout << endl;
        cout << "A maximum of " << m << " file names can appear on the command line.";
        cout << endl;
        cout << a << " file names were entered.";
        cout << endl;
        cout << "p01 (<input file name> (<output file name>))";
    }
};

int main(int argc, char* argv[])
{   try {
        char target[255];
        switch (argc) {
            case 1://no files on the command line
                cout << "Enter the target word: ";
                cin >> target;
                cout << endl;
            break;
            case 2://input file on the command line/prompt for output file
                strcpy(target,argv[1]);
            break;
            default:
                throw CommandLineException(1,argc-1);
                break;
        }
        string x;
        //ifstream file;
        std::string targetWord= std::string(target);
        Manager(targetWord);
    } catch ( ... ) {
        cout << endl;
        cout << "Program Terminated!";
        cout << endl;
        cout << "I won't be back!";
        cout << endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}