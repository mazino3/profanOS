#ifndef TIME_ID
#define TIME_ID 1004

#define get_func_addr ((int (*)(int, int)) *(int *) 0x199994)

/*
int is_leap_year(int year);
int time_calc_unix(time_t *time);
int time_gen_unix();
void time_add(time_t *time, int seconde);
void time_jet_lag(time_t *time);
void ms_sleep(uint32_t ms);
*/

#define is_leap_year ((int (*)(int)) get_func_addr(TIME_ID, 2))
#define time_calc_unix ((int (*)(time_t *)) get_func_addr(TIME_ID, 3))
#define time_gen_unix ((int (*)()) get_func_addr(TIME_ID, 4))
#define time_add ((void (*)(time_t *, int)) get_func_addr(TIME_ID, 5))
#define time_jet_lag ((void (*)(time_t *)) get_func_addr(TIME_ID, 6))
#define ms_sleep ((void (*)(uint32_t)) get_func_addr(TIME_ID, 7))

#endif
