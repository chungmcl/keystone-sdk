#include "edge_syscall.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>

// Special edge-call handler for syscall proxying
void
incoming_syscall(struct edge_call* edge_call) {
  struct edge_syscall* syscall_info;

  size_t args_size;

  if (edge_call_args_ptr(edge_call, (uintptr_t*)&syscall_info, &args_size) != 0)
    goto syscall_error;

  // NOTE: Right now we assume that the args data is safe, even though
  // it may be changing under us. This should be safer in the future.

  edge_call->return_data.call_status = CALL_STATUS_OK;

  int64_t ret;

  // Right now we only handle some io syscalls. See runtime for how
  // others are handled.
  switch (syscall_info->syscall_num) {
    case (SYS_openat):;
      sargs_SYS_openat* openat_args = (sargs_SYS_openat*)syscall_info->data;
      ret                           = openat(
          openat_args->dirfd, openat_args->path, openat_args->flags,
          openat_args->mode);
      break;
    case (SYS_unlinkat):;
      sargs_SYS_unlinkat* unlinkat_args =
          (sargs_SYS_unlinkat*)syscall_info->data;
      ret = unlinkat(
          unlinkat_args->dirfd, unlinkat_args->path, unlinkat_args->flags);
      break;
    case (SYS_ftruncate):;
      sargs_SYS_ftruncate* ftruncate_args =
          (sargs_SYS_ftruncate*)syscall_info->data;
      ret = ftruncate(ftruncate_args->fd, ftruncate_args->offset);
      break;
    case (SYS_fstatat):;
      sargs_SYS_fstatat* fstatat_args = (sargs_SYS_fstatat*)syscall_info->data;
      // Note the use of the implicit buffer in the stat args object (stats)
      ret = fstatat(
          fstatat_args->dirfd, fstatat_args->pathname, &fstatat_args->stats,
          fstatat_args->flags);
      break;
    case (SYS_write):;
      sargs_SYS_write* write_args = (sargs_SYS_write*)syscall_info->data;
      ret = write(write_args->fd, write_args->buf, write_args->len);
      break;
    case (SYS_read):;
      sargs_SYS_read* read_args = (sargs_SYS_read*)syscall_info->data;
      ret = read(read_args->fd, read_args->buf, read_args->len);
      break;
    case (SYS_sync):;
      sync();
      ret = 0;
      break;
    case (SYS_fsync):;
      sargs_SYS_fsync* fsync_args = (sargs_SYS_fsync*)syscall_info->data;
      ret                         = fsync(fsync_args->fd);
      break;
    case (SYS_close):;
      sargs_SYS_close* close_args = (sargs_SYS_close*)syscall_info->data;
      ret                         = close(close_args->fd);
      break;
    case (SYS_lseek):;
      sargs_SYS_lseek* lseek_args = (sargs_SYS_lseek*)syscall_info->data;
      ret = lseek(lseek_args->fd, lseek_args->offset, lseek_args->whence);
      break;
    case (SYS_pipe2):;
      int *fds = (int *) syscall_info->data;
      printf("fds[0]: %d, fds[1]: %d\n", fds[0], fds[1]);
      ret = pipe(fds); 
      printf("fds[0]: %d, fds[1]: %d\n", fds[0], fds[1]);
      break;
    case (SYS_epoll_create1):;
      sargs_SYS_epoll_create1 *epoll_args = (sargs_SYS_epoll_create1 *) syscall_info->data;
      printf("epoll-create: %d\n", epoll_args->size);
      epoll_args->size = 1024; 
      ret = epoll_create(epoll_args->size);
      break;
    case (SYS_socket):;
      sargs_SYS_socket *socket_args = (sargs_SYS_socket *) syscall_info->data; 
      ret = socket(socket_args->domain, socket_args->type, socket_args->protocol); 
      break; 
    case (SYS_setsockopt):;
      sargs_SYS_setsockopt *setsockopt_args = (sargs_SYS_setsockopt *) syscall_info->data; 
      printf("[sdk] socket: %d, level: %d, opt_name: %d, opt_val: %d, opt_len: %d\n", 
          setsockopt_args->socket, setsockopt_args->level, setsockopt_args->option_name, setsockopt_args->option_value, setsockopt_args->option_len);
      ret = setsockopt(setsockopt_args->socket, setsockopt_args->level, setsockopt_args->option_name, &setsockopt_args->option_value, setsockopt_args->option_len);
      break;
    case (SYS_bind):;
      sargs_SYS_bind *bind_args = (sargs_SYS_bind *) syscall_info->data; 
      ret = bind(bind_args->sockfd, &bind_args->addr, bind_args->addrlen);
      break;
    case (SYS_listen):;
      sargs_SYS_listen *listen_args = (sargs_SYS_listen *) syscall_info->data; 
      ret = listen(listen_args->sockfd, listen_args->backlog);
      break;
    case (SYS_accept):;
      sargs_SYS_accept *acccept_args = (sargs_SYS_accept *) syscall_info->data; 
      ret = accept(acccept_args->sockfd, &acccept_args->addr, &acccept_args->addrlen);
      break;

    default:
      goto syscall_error;
  }

  /* Setup return value */
  void* ret_data_ptr      = (void*)edge_call_data_ptr();
  *(int64_t*)ret_data_ptr = ret;
  if (edge_call_setup_ret(edge_call, ret_data_ptr, sizeof(int64_t)) != 0)
    goto syscall_error;

  return;

syscall_error:
  edge_call->return_data.call_status = CALL_STATUS_SYSCALL_FAILED;
  return;
}
