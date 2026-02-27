compile tcc
-----------
gcc -m32 -nostdlib -nostdinc -fno-builtin -static \
    -I./lib/include \
    -I./tcc/include \
    lib/*.c lib/stdio/*.c tcc/*.c \
    -o tcc.elf


string.h

void  *memmove(void *dest, const void *src, size_t n);
char  *strstr(const char *haystack, const char *needle);
char  *strerror(int errnum);
char  *strpbrk(const char *s, const char *accept);

stdlib.h

long               strtol(const char *nptr, char **endptr, int base);
unsigned long      strtoul(const char *nptr, char **endptr, int base);
long long          strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double             strtod(const char *nptr, char **endptr);
long double        ldexpl(long double x, int exp);
void               qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
char              *getenv(const char *name);
void               free(void *ptr);
int                abs(int j);

time.h

typedef long time_t;

struct tm {
    int tm_sec;    /* 0-60 */
    int tm_min;    /* 0-59 */
    int tm_hour;   /* 0-23 */
    int tm_mday;   /* 1-31 */
    int tm_mon;    /* 0-11 */
    int tm_year;   /* years since 1900 */
    int tm_wday;   /* 0-6, Sunday=0 */
    int tm_yday;   /* 0-365 */
    int tm_isdst;  /* daylight saving flag */
};

time_t      time(time_t *tloc);
struct tm  *localtime(const time_t *timep);

semaphore.h

typedef struct {
    int value;
    /* platform-specific wait queue */
} sem_t;

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);


assert.h

/* macro, not a function, but must be provided */
#ifdef NDEBUG
#  define assert(expr) ((void)0)
#else
#  define assert(expr) \
     ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))

void __assert_fail(const char *expr, const char *file,
                   unsigned int line, const char *func);
#endif

/* flags needed */
#define O_RDONLY  0
#define O_WRONLY  1
#define O_RDWR    2
#define O_CREAT   0x40
#define O_TRUNC   0x200
#define O_BINARY  0       /* no-op on non-Windows */

/* 2-arg form (already present) */
int   open(const char *pathname, int flags);
/* 3-arg form (missing) */
int   open(const char *pathname, int flags, mode_t mode);

FILE *fdopen(int fd, const char *mode);
off_t lseek(int fd, off_t offset, int whence);
int   unlink(const char *pathname);
char *getcwd(char *buf, size_t size);
int   execvp(const char *file, char *const argv[]);
