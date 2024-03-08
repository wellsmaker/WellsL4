#ifndef BENCHMARK_TIMING_H_
#define BENCHMARK_TIMING_H_


void read_timer_start_of_swap(void);
void read_timer_end_of_swap(void);
void read_timer_start_of_isr(void);
void read_timer_end_of_isr(void);
void read_timer_start_of_tick_handler(void);
void read_timer_end_of_tick_handler(void);
void read_timer_end_of_userspace_enter(void);


#endif
