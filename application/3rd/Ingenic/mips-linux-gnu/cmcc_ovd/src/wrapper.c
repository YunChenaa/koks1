// wrapper.c
#define _GNU_SOURCE
//#define _LARGEFILE64_SOURCE
#define _DEFAULT_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>

// ============================================
// 文件状态函数
// ============================================

// 标准版本
int __xstat(int ver, const char *path, struct stat *buf) {
    (void)ver;
    return stat(path, buf);
}

int __fxstat(int ver, int fd, struct stat *buf) {
    (void)ver;
    return fstat(fd, buf);
}

// 64位版本
#ifdef _LARGEFILE64_SOURCE
int __xstat64(int ver, const char *path, struct stat64 *buf) {
    (void)ver;
    return stat64(path, buf);
}

int __fxstat64(int ver, int fd, struct stat64 *buf) {
    (void)ver;
    return fstat64(fd, buf);
}
#else
// 如果系统不支持 stat64，回退到 stat
int __xstat64(int ver, const char *path, void *buf) {
    (void)ver;
    return stat(path, (struct stat*)buf);
}

int __fxstat64(int ver, int fd, void *buf) {
    (void)ver;
    return fstat(fd, (struct stat*)buf);
}
#endif

// ============================================
// 标准输入输出函数
// ============================================
int __isoc99_sscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsscanf(str, format, args);
    va_end(args);
    return ret;
}

int __isoc99_fscanf(FILE *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vfscanf(stream, format, args);
    va_end(args);
    return ret;
}

// ============================================
// 字符串函数
// ============================================
char *__strdup(const char *s) {
    if (!s) return NULL;
    return strdup(s);
}

// ============================================
// 标准 I/O
// ============================================
int _IO_getc(FILE *stream) {
    return getc(stream);
}

// ============================================
// 断言函数
// ============================================
void __assert_fail(const char *assertion, const char *file, 
                   unsigned int line, const char *function) {
    fprintf(stderr, "Assertion failed: %s (%s: %s: %u)\n", 
            assertion, function ? function : "unknown", file, line);
    abort();
}

// ============================================
// 内存函数
// ============================================
#ifdef __linux__
#include <malloc.h>
size_t malloc_usable_size(void *ptr) {
    return malloc_usable_size(ptr);
}
#else
size_t malloc_usable_size(void *ptr) {
    (void)ptr;
    return 0;
}
#endif

