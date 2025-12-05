/* rl_client.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "rl_client.h"

static int rl_sock_fd = -1;

struct __attribute__((packed)) RlMsg {
  double reward_prev;
  double state[RL_STATE_DIM];
};

struct __attribute__((packed)) RlResp {
  int32_t action;
};

void rl_init(void) {
  const char* sock_path = getenv("AFL_RL_SOCK");
  if (!sock_path) {
    /* 소켓 경로 지정 안 하면 RL 비활성화 */
    return;
  }

  rl_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (rl_sock_fd < 0) {
    perror("rl socket");
    rl_sock_fd = -1;
    return;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

  if (connect(rl_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("rl connect");
    close(rl_sock_fd);
    rl_sock_fd = -1;
    return;
  }

  fprintf(stderr, "[RL] Connected to %s\n", sock_path);
}

/* state, 이전 step reward를 보내고 action(0~3)을 받아온다 */
uint8_t rl_step(const double* state, double reward_prev) {

  if (rl_sock_fd < 0) {
    return 1; /* NORMAL default */
  }

  struct RlMsg msg;
  msg.reward_prev = reward_prev;
  memcpy(msg.state, state, sizeof(double) * RL_STATE_DIM);

  ssize_t to_write = sizeof(msg);
  uint8_t* p = (uint8_t*)&msg;
  while (to_write > 0) {
    ssize_t w = write(rl_sock_fd, p, to_write);
    if (w <= 0) {
      perror("rl write");
      close(rl_sock_fd);
      rl_sock_fd = -1;
      return 1;
    }
    to_write -= w;
    p += w;
  }

  struct RlResp resp;
  uint8_t* r = (uint8_t*)&resp;
  ssize_t to_read = sizeof(resp);

  while (to_read > 0) {
    ssize_t rd = read(rl_sock_fd, r, to_read);
    if (rd <= 0) {
      perror("rl read");
      close(rl_sock_fd);
      rl_sock_fd = -1;
      return 1;
    }
    to_read -= rd;
    r += rd;
  }

  int32_t a = resp.action;
  if (a < 0) a = 0;
  if (a > 3) a = 3;

  return (uint8_t)a;
}