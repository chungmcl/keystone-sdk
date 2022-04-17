#include "eapp_utils.h"
#include "string.h"
#include "edge_call.h"
#include <syscall.h>

#define OCALL_PRINT_STRING 1

unsigned long ocall_print_string(char* string);

// toread: fuzzying: secure VAX retrospective, "trusted browsers uncertain times," 

// TODO(chungmcl): Add option for turning fuzzy on/off
// look at keystone/linux-keystone-driver/keystone-ioctl.c, 
// keystone SDK (have a way for host to turn on flags), 
// keystone SM (check for flags, turn flags on)

// TODO(chungmcl): Write a test
// Make a host app that
// forks into two threads --
// one thread makes eapp do stuff (write, etc.)
// other thread watches over shared mem
// to see that stuff is written as expected
// at the right time

// We want to run a "failed" test (one which DOESN'T use the timing_buffer)
// Use that as a "benchmark" to compare a "successful" test against (show that the timing_buffer does indeed hide)

int main(){
  ocall_print_string("deadfish");

  //char* fish = "I'm fish";
  //ocall_print_string(fish);
  //write_to_shared((void*)fish, (uintptr_t)0 + SYSCALL_SHAREDWRITE, (size_t)(strlen(fish) + 1));
  //printf("test %i\n", SYSCALL_SHAREDWRITE);
  //int test = SYSCALL_SHAREDWRITE + 1;

  EAPP_RETURN(0);
}

unsigned long ocall_print_string(char* string){
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, strlen(string)+1, &retval ,sizeof(unsigned long));
  return retval;
}