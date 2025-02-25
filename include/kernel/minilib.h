#ifndef MINILIB_H
#define MINILIB_H

#define UNUSED(x) (void)(x)
#define ARYLEN(x) (int)(sizeof(x) / sizeof((x)[0]))

void str_cat(char s1[], char s2[]);
int str_len(char s[]);
void str_cpy(char s1[], char s2[]);
void int2str(int n, char s[]);
int str2int(char s[]);
int str_cmp(char s1[], char s2[]);
int str_ncmp(char s1[], char s2[], int n);
int str_count(char s[], char c);
void str_append(char s[], char c);

void kprintf(char *fmt, ...);

void mem_copy(uint8_t *source, uint8_t *dest, int nbytes);
void mem_set(uint8_t *dest, uint8_t val, uint32_t len);
void mem_move(uint8_t *source, uint8_t *dest, int nbytes);

void free(void *addr);
void *malloc(uint32_t size);
void *realloc(void *ptr, uint32_t size);
void *calloc(uint32_t size);

#endif
