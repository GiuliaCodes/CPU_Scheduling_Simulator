#include "fake_process.h"
#include "linked_list.h"
#pragma once


typedef struct {
  ListItem list;
  int pid;
  ListHead events;

  float pred; //this is q(t+1)
  int q_current;
  float q_predicted; //this is q(t)

} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args, int i);     //qui va aggiunto il numero della cpu su cui si chiama la funzione

typedef struct FakeOS{
  
  int cpu_num;        

  FakePCB** running;    //avere più CPU significa che si possono avere più CPU in running

  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn;
  void* schedule_args;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os, int cpu_num);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
