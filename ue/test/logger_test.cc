#define NTHREADS 100
#define NMSGS    100

#include <stdio.h>
#include "logger.h"

using namespace srsue;

typedef struct {
  logger *l;
  int thread_id;
}args_t;

void* thread_loop(void *a) {
  args_t *args = (args_t*)a;
  char buf[100];
  for(int i=0;i<NMSGS;i++)
  {
    sprintf(buf, "Thread %d: %d", args->thread_id, i);
    args->l->log(buf);
  }
}

void write(std::string filename) {
  logger l(filename);
  pthread_t threads[NTHREADS];
  args_t    args[NTHREADS];
  for(int i=0;i<NTHREADS;i++) {
    args[i].l = &l;
    args[i].thread_id = i;
    pthread_create(&threads[i], NULL, &thread_loop, &args[i]);
  }
  for(int i=0;i<NTHREADS;i++) {
    pthread_join(threads[i], NULL);
  }
}

bool read(std::string filename) {
  bool pass = true;
  bool written[NTHREADS][NMSGS];
  int thread, msg;
  int r;

  for(int i=0;i<NTHREADS;i++) {
    for(int j=0;j<NMSGS;j++) {
      written[i][j] = false;
    }
  }
  FILE *f = fopen(filename.c_str(), "r");
  if(f!=NULL) {
    while(fscanf(f, "Thread %d: %d\n", &thread, &msg)) {
      written[thread][msg] = true;
    }
    fclose(f);
  }
  for(int i=0;i<NTHREADS;i++) {
    for(int j=0;j<NMSGS;j++) {
      if(!written[i][j]) pass = false;
    }
  }
  return pass;
}

int main(int argc, char **argv) {
  bool result;
  std::string f("log.txt");
  write(f);
  result = read(f);
  remove(f.c_str());
  if(result) {
    printf("Passed\n");
    exit(0);
  }else{
    printf("Failed\n;");
    exit(1);
  }
}
