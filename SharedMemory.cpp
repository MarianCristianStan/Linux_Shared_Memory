#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>

const char* SHARED_MEMORY_NAME = "/my_shared_memory";
const char* SEMAPHORE_NAME = "/my_semaphore";

int main() {
    // am creat o constanta pentru size-ul alocat variabilei shared_memory (tip int)
    const size_t SIZE = sizeof(int);
    int* shared_memory;

    // am creat un semafor, pentru gestionarea zonei critice (terenul neutre) si verificam daca a fost creat cu succes
    sem_t* semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
        std::cerr << "Semaphore creation failed.\n";
        return EXIT_FAILURE;
    }

    // am create memoria partajata si am realizat "mapparea" acesteia
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "Shared memory creation failed.\n";
        return EXIT_FAILURE;
    }
    ftruncate(shm_fd, SIZE);
    shared_memory = (int*)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        std::cerr << "Memory mapping failed.\n";
        return EXIT_FAILURE;
    }

    // initializarea memorie partajate (numararea de la 1)
    *shared_memory = 1;

    // am creat un proces copil (exact ca in laboratorul trecut), astfel avem in executie 2 procese (parinte si copil )
    pid_t pid = fork();

    //verificam daca a fost creat cu succes
    if (pid < 0) {
        std::cerr << "Fork failed.\n";
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // procesul copil isi incepe executia task-ului
        bool running = true;
        while(running) {
            // cand accesam zona critica, adica memoria partajata de cele 2 procese, avem nevoie de un semafor, acesta blocheaza accesul altor procese,
            // in cazul nostru doar "child" are acces
            sem_wait(semaphore);
            int value = *shared_memory;
            // verificam daca nu am ajuns inca la valoarea dorita
            if(value < 1000){
            // simulam un nr, daca restul impartirii la 2 este 0, nu facem nimic, daca este 1, atunci incrementam memoria partajata
            int guess = rand() % 2;
            if (guess == 0) {
                std::cout << "Child Process: Get 0\n";
            } else {
                // pentru vizualizarea mai buna a executie 
                usleep(100000); 
                //incrementam si scriem un mesaj ca procesul copil (child) a actualizat variabila din memorie
                value++;
                *shared_memory = value;
                std::cout << "Child Process: Get 1 and wrote " << value << " to memory\n";
                }
            }else{
                //daca am ajuns la final, oprim loop-ul
                running = false;
            }
            // eliberare semafor
             sem_post(semaphore);
        }
    } else {
        // procesul parinte isi incepe executia task-ului
        bool running = true;
        while(running) { 
            // cand accesam zona critica, adica memoria partajata de cele 2 procese, avem nevoie de un semafor, acesta blocheaza accesul altor procese
            // in cazul nostru doar "parent" are acces
            sem_wait(semaphore);
            int value = *shared_memory;

            // verificam daca nu am ajuns inca la valoarea dorita
            if(value < 1000)
            {   
            // simulam un nr, daca restul impartirii la 2 este 0, nu facem nimic, daca este 1, atunci incrementam memoria partajata
            int guess = rand() % 2;
            if (guess == 0) {
                std::cout << "Parent Process: Get 0\n";
            } else {
                // pentru vizualizarea mai buna a executie
                usleep(100000); 
                //incrementam si scriem un mesaj ca procesul parinte (parent) a actualizat variabila din memorie
                value++;
                *shared_memory = value;
                std::cout << "Parent Process: Get 1 and wrote " << value << " to memory\n";
            }
          
            }
            else {
                //daca am ajuns la final, oprim loop-ul
                running = false;
            }
            // eliberarea semaforului
            sem_post(semaphore);
        }
    
        // asteapta dupa procesul copil sa-si termine executia
        waitpid(pid, NULL, 0);

        // facem close si unlink pentru semafor
        sem_close(semaphore);
        sem_unlink(SEMAPHORE_NAME);

        // facem unmap si unlink pentru memoria partajata
        munmap(shared_memory, SIZE);
        close(shm_fd);
        shm_unlink(SHARED_MEMORY_NAME);
    }
    return 0;
}