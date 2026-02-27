/* macro, not a function, but must be provided */
#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr)                                                           \
  ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))

void __assert_fail(const char *expr, const char *file, unsigned int line,
                   const char *func);
#endif
