#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_TRUNC 0x200
#define O_BINARY 0

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
