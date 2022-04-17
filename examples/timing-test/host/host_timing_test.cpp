#include <thread>
#include <chrono>
#include <cstdlib>
#include <string>
#include <ctime>
#include <unistd.h>
#include <assert.h>
// Keystone
#include <edge_call.h>
#include <keystone.h>


using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::thread;

void test_basic_functionality(uintptr_t sharedBufferPtr);
void spy(uintptr_t target, char* out);
unsigned long print_string(char* str);
void print_string_wrapper(void* buffer);
#define OCALL_PRINT_STRING 1

int
main(int argc, char** argv) {
  Keystone::Enclave enclave;
  Keystone::Params params;
  params.setFreeMemSize(1024 * 1024);
  // Difference between DEFAULT_UNTRUSTED_PTR and enclave.getSharedBuffer()?
  params.setUntrustedMem(DEFAULT_UNTRUSTED_PTR, 1024 * 1024);
  enclave.init(argv[1], argv[2], params);
  enclave.registerOcallDispatch(incoming_call_dispatch);
  /* We must specifically register functions we want to export to the
     enclave. */
  register_call(OCALL_PRINT_STRING, print_string_wrapper);
  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  // chungmcl
  char* spy_out = (char*)malloc(2048);
  thread spy_thread(spy, (uintptr_t)enclave.getSharedBuffer(), spy_out);
  enclave.run();
  spy_thread.join();

  void* shared_buff = enclave.getSharedBuffer();
  uintptr_t expected_dest = (uintptr_t)shared_buff + 100;

  int shared_buff_int = *(int*)shared_buff;
  printf("buff param 0: %i\n", shared_buff_int);
  printf("spy out: %s\n", spy_out);

  test_basic_functionality((uintptr_t)expected_dest);

  return 0;
}

void test_basic_functionality(uintptr_t expected_dest) {
  char* string = (char*)expected_dest;
  printf("result: %s\n", string);
}

void spy(uintptr_t target, char* out) {
  // best way to check if memory changed? (shared buff is very big)
  auto begin = std::chrono::high_resolution_clock::now();
  // use a set position
  // while (nothing changed) keep looping
  sleep(2);
  auto end = std::chrono::high_resolution_clock::now();
  int64_t duration =  std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
  sprintf(out, "duration (ns): %i\n", duration);
  // sprintf(out, "test\n");
}

/***
 * An example call that will be exposed to the enclave application as
 * an "ocall". This is performed by an edge_wrapper function (below,
 * print_string_wrapper) and by registering that wrapper with the
 * enclave object (below, main).
 ***/
unsigned long
print_string(char* str) {
  return printf("Enclave said: \"%s\"\n", str);
}

/***
 * Example edge-wrapper function. These are currently hand-written
 * wrappers, but will have autogeneration tools in the future.
 ***/
void
print_string_wrapper(void* buffer) {
  /* Parse and validate the incoming call data */
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  /* Pass the arguments from the eapp to the exported ocall function */
  ret_val = print_string((char*)call_args);

  /* Setup return data from the ocall function */
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(
          edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  /* This will now eventually return control to the enclave */
  return;
}
