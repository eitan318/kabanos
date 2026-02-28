#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
long double ldexpl(long double x, int exp);
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
char *getenv(const char *name);
int abs(int j);
