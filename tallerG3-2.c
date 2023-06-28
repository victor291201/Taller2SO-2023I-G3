#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wait.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>

int main( int argc, char **argv ){
    int nH;

    //pidiendo el numero de columnas por consola
    printf("escriba el numero de sensores(hijos): ");
    scanf("%d",&nH);

    //declarando el vector de pids en la memoria compartida
    int shm_idPid = shmget(IPC_PRIVATE,nH*sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR);
    int *memcPid = (int *)shmat(shm_idPid, 0, 0);

    //declarando el vector de tiempos en la memoria compartida
    int shm_idTime = shmget(IPC_PRIVATE,nH*sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR);
    int *memcTime = (int *)shmat(shm_idTime, 0, 0);

    //rellenando el vector de tiempos de ceros
    for(int i=0;i<nH;i++){
        memcTime[i]=0;
    }

    //declarando el vector de palabras en la memoria compartida
    int max_line = 20;
    int *shm_idStr = (int *) malloc(nH * sizeof(int));
    char **memcStr = (char **) malloc(nH * sizeof(char*));;
    char *memcChar = NULL;
    for(int i = 0;i<nH;i++){
        shm_idStr[i] = shmget(IPC_PRIVATE,max_line*sizeof(char), IPC_CREAT | S_IRUSR | S_IWUSR);
        memcStr[i] = (char *)shmat(shm_idStr[i], 0, 0);
        strcpy(memcStr[i],"");
    }
    
    //declarando la funcion para obtener el id del proceso
    int idH(int pid){
        for(int i=0;i<nH;i++){
            if(pid == memcPid[i]){
                return i;
            }
        }
        return -1;
    }
    
    //guardando el pid del padre
    pid_t pidp = getpid();

    //creando los hijos y almacenando los pids
    int idHijo;
    for(idHijo =0;idHijo<nH;idHijo++){
        int var = fork();
        if(!var){
            break;
        }
        memcPid[idHijo] = var;
    }

   
    //creando la logica para cada proceso
    if(pidp == getpid()){

        //logica del padre
        sleep(1);
        while (1){
            int pid,time;
            char bol[2],ord[max_line];
            
            for(int i=0;i<nH;i++){
                printf("hijo numero [%d] con id [%d]\n",i,memcPid[i]);
            }
            printf("ingrese el pid del proceso: \n");
            scanf("%d",&pid);
            printf("ingrese el tiempo del proceso: \n");
            scanf("%d",&time);
            printf("ingrese la orden del proceso: \n");
            scanf("%s",ord);
            //obteniendo el idHijo del proceso
            int id = idH(pid);
            strcpy(memcStr[id],ord); 
            sleep(1);
            memcTime[id] = time;
            printf("desea salir?(y/n): \n");
            scanf("%s",bol);
            if(strcmp(bol,"y") == 0)
                break;
        }
        
        //esperando que terminen los hijos 
        for(int i = 0; i<nH;i++){
            kill(memcPid[i],SIGUSR1);
		    wait(NULL);
        }

        //cerrando las n instancias de memoria dinamica
        printf("terminando el padre...\n");

    }else{
        //logica de los hijos
        int bol = 1;
        void sighandler( int sig ){
            bol = 0;
        }
        void *oldhandler = signal( SIGUSR1, sighandler); 
        if(oldhandler == SIG_ERR){perror("signal:");exit(EXIT_FAILURE);  }
        while (bol){
            if(memcTime[idHijo] != 0){
                printf("hijo [%d] recibio la señal [%s] para ejecutar en [%d]ms \n",idHijo,memcStr[idHijo],memcTime[idHijo]);
                usleep(memcTime[idHijo]);
                printf("hijo [%d] ejecuto la señal [%s] luego de esperar [%d]ms \n",idHijo,memcStr[idHijo],memcTime[idHijo]);
                strcpy(memcStr[idHijo],"");
                memcTime[idHijo]=0;
            }
        }
        if(signal(SIGUSR1, oldhandler) == SIG_ERR){
            perror("signal:");
            exit(EXIT_FAILURE);
        }
        printf("terminando el hijo %d ...\n",idHijo);
    }
    return 0;
}