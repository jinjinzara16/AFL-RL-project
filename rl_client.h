/* rl_client.h */

#ifndef RL_CLIENT_H
#define RL_CLIENT_H

#include <stdint.h>

#define RL_STATE_DIM 8   /* 우리가 환경에서 쓰는 state 차원 */

void   rl_init(void);
uint8_t rl_step(const double* state, double reward_prev);

#endif