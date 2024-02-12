#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {  //TODO: DELETE?
  int quantum;
} SchedRRArgs;


typedef struct {
  float decay_coeff;  //this is a
  //float prediction; //this is q(t+1)   //in FakePCB now
  //int quantum;                        //DELETE?? CHANGE???
} SchedSJFArgs;

void schedRR(FakeOS* os, void* args_){  //TODO: delete?
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


FakePCB* Choose_Next(FakeOS* os, SchedSJFArgs* args, ListHead* head) {
  //SJF must choose, from the list of PCBs in running, the PCB with the min value of prediction!
    float min=999;
    FakePCB* sjfChoice= NULL;
    ListItem* aux=os->ready.first;

    while(aux){
      FakePCB* pcb=(FakePCB*)aux;

      //quantum prediction: q(t+1) = a * q_current + (1-a) * q(t)
      pcb->pred= args->decay_coeff * pcb->q_current + ( 1 - args->decay_coeff ) * pcb->q_predicted;
      printf("\tready pid: %d, prediction: %.04f, q_current: %d, q_predicted: %.04f\n", pcb->pid, pcb->pred, pcb->q_current, pcb->q_predicted);

      if (pcb->pred < min) {
        min=pcb->pred;
        sjfChoice= pcb;
      }

      aux=aux->next;

    }     //questo ciclo calcola per ogni PCB in ready la predizione, e sceglie il pcb con quella minore
    
    if (!os->running) {
      printf("\tSupposed to chose: %d\n", sjfChoice->pid);
      printf("Min: %f\n", min);
    }

    //Preemption:           //to be tested!
    //se c'è un processo in running: si deve verificare che abbia "tempo rimanente predetto" minore, e in questo caso si deve levare il pcb in running e mettere quello scelto...
    if ( os->running) {
      printf ("\n\tPid %d is running, checking preemption\n", os->running->pid);
      printf("\tRunning prediction: %f\n", os->running->pred);
      if (os->running->pred - os->running->q_current > sjfChoice->pred) {      //os->running->pred - os->running->q_current  sarebbe il tempo rimanente atteso?? VERIFICA (oppure solo os->running->pred?)
        printf("\tpid%d remaining time (predicted) %.04f; pid%d pred %.04f\n", os->running->pid, os->running->pred- os->running->q_current, sjfChoice->pid, sjfChoice->pred);
        printf("\tPREEMPTING: pid%d with pid%d \n", os->running->pid, sjfChoice->pid);
        os->running->q_predicted= os->running->pred - os->running->q_current;    //aggiorno la predizione del processo?? VERIFICA
        List_pushBack(&os->ready,(ListItem*) os->running);    //rimuovo il running e lo metto in ready
      } 
      else {
        printf("\tNo preemption needed\n"); 
        return os->running;  // se no preemption needed, devi lasciare os->running!!!
      }   
    }
    

    FakePCB* chosen= (FakePCB*) List_detach(&os->ready, (ListItem*) sjfChoice);

    sjfChoice->q_current=0;   //anche quando è preempted?? Penso di si perchè q_current è il cpu burst time effettivo
    sjfChoice->q_predicted=min;  

    return chosen;

}

void schedSJF(FakeOS* os, void* args_) {      //puoi riunificare le due funzioni
 SchedSJFArgs* args=(SchedSJFArgs*)args_;    
  //printf("Using SJF\n");
  
    // look for the first process in ready
    // if none, return
    if (! os->ready.first)
      return;
      
    FakePCB* pcb= Choose_Next(os, args, &os->ready);
    os->running=pcb;

    //TO DO: preemption :(
    

    //DO YOU NEED THIS?  
  /*
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
    }  */

 
 
}

int main(int argc, char** argv) {
  FakeOS_init(&os);

  /* this is for RR:
  SchedRRArgs srr_args;
  srr_args.quantum=5;
  os.schedule_args=&srr_args;
  os.schedule_fn=schedRR;
  */

  SchedSJFArgs sjf_args;
  sjf_args.decay_coeff=0.5;   
  //sjf_args.prediction=999;    
  //sjf_args.quantum=25;       //can be deleted??
   
  os.schedule_args= &sjf_args;
  os.schedule_fn=schedSJF;  //for now, it functions as RR
  
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
  while(os.running
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
}
