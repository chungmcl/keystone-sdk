#include <iostream>
#include <cstdio>
#include "keystone.h"
#include "apps/add_long_entry.h"

#define SECRET_DATA "hello, world!\n"
#define SECRET_SIZE 15

int main()
{

  Keystone enclave;
  keystone_status_t err;
  char buf[SECRET_SIZE] = {0,};

  enclave.init_elf("add_long.eapp_riscv", 4097, add_long_entry);
  enclave.run();

  return 0;
}
