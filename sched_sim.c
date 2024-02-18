#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;
/*
typedef struct {
  int quantum;
} SchedRRArgs;
*/

typedef struct {
  float decay_coeff;  //this is a
} SchedSJFArgs;

/*
void schedRR(FakeOS* os, void* args_){
  SchedRRArgs* args=(SchedRRArgs*)args_;

  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;

  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  os->running=pcb;
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
};
*/

FakePCB* Choose_Next(FakeOS* os, SchedSJFArgs* args, ListHead* head, int i) {      
  //SJF must choose, from the list of PCBs in ready, the PCB with the min value of prediction!
  float min=999;              //is there a better initialization??
  FakePCB* sjfChoice= NULL;

  ListItem* aux=os->ready.first;

  while(aux){
    FakePCB* pcb=(FakePCB*)aux;

    //quantum prediction: q(t+1) = a * q_current + (1-a) * q(t):
    pcb->pred= args->decay_coeff * pcb->q_current + ( 1 - args->decay_coeff ) * pcb->q_predicted;
    printf("\tready pid: %d\n\t\tprediction: %.04f, q_current: %d, q_predicted: %.04f\n", pcb->pid, pcb->pred, pcb->q_current, pcb->q_predicted);

    if (pcb->pred < min) {
      min=pcb->pred;
      sjfChoice= pcb;
    }

    aux=aux->next;

  } 
    
  if (!os->running[i]) {
    printf("\tSuppoosed to chose: pid%d on cpu %d\n", sjfChoice->pid, i+1);
    //printf("\tMin: %f\n", min);
  }

  //Preemption quando arriva un processo in ready che abbia predizione minore del "tempo rimanente predetto" del processo in running
  //se c'è un processo in running, si deve verificare che abbia "tempo rimanente predetto" maggiore, e, in questo caso, si deve levare il pcb da running e metterci quello scelto
  FakePCB* running=os->running[i];
  if ( os->running[i]) {
    //printf ("\n\tPid %d is running, checking preemption\n", running->pid);
    //printf("\tRunning prediction: %f\n", os->running[i]->pred);
    if (running->pred - running->q_current > sjfChoice->pred) {      //(tempo predetto - tempo trascorso in cpu) 
      printf("\tpid%d remaining time (predicted): %.04f; pid%d prediction: %.04f\n", running->pid, running->pred- running->q_current, sjfChoice->pid, sjfChoice->pred);
      printf("\tPREEMPTING: pid%d with pid%d \n", running->pid, sjfChoice->pid);
      running->q_predicted= running->pred - running->q_current;    //aggiorno la predizione del running
      List_pushBack(&os->ready,(ListItem*) running);    //rimuovo il running e lo metto in ready
    } 
    else {
      printf("\tNo preemption needed\n"); 
      return running;  // se non si fa preemption, bisogna lasciare il processo in running
    }   
  }

  sjfChoice->q_current=0;
  sjfChoice->q_predicted=min;   

  FakePCB* chosen= (FakePCB*) List_detach(&os->ready, (ListItem*) sjfChoice);

  return chosen;

}

void schedSJF(FakeOS* os, void* args_) {      //potresti riunificare le due funzioni
 SchedSJFArgs* args=(SchedSJFArgs*)args_; 

 for (int i=0; i<os->cpu_num; i++) {
    // look for the first process in ready
    // if none, return
    if (! os->ready.first)
    return;
    
    FakePCB* pcb= Choose_Next(os, args, &os->ready, i);     
    os->running[i]=pcb;     //si assegna il processo scelto alla cpu i-esima
  }
  

 
}

int main(int argc, char** argv) {
  
  int num_cpu;
  printf("Please select a number of cpus:\n");
  scanf("%d", &num_cpu);
  while (num_cpu<1){
    printf("Number of cpu must be >= 1\n");
    scanf("%d", &num_cpu);
  }

  FakeOS_init(&os, num_cpu);

  printf("Number of cpus:%d\n", os.cpu_num);

  /* this is for RR:
  SchedRRArgs srr_args;
  srr_args.quantum=5;
  os.schedule_args=&srr_args;
  os.schedule_fn=schedRR;
  */

  SchedSJFArgs sjf_args;
  sjf_args.decay_coeff=0.5; 
   
  os.schedule_args= &sjf_args;
  os.schedule_fn=schedSJF;
  
  for (int i=1; i<argc; ++i){
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d\n",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);

  for (int i=0; i<os.cpu_num; i++){
    while(os.running[i]
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
    }
  }

  FakeOS_destroy(&os);
}
