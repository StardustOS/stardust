/* Copyright (C) 2018, Ward Jaradat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Notes:
 * 
 * Some parts were obtained from the standard libraries like dietlibc, 
 * whereas others were implemented from scratch. This provides a fake
 * implementation of the standard header.
 * 
 * Cleaning here is required to remove unnecessary parts which are no
 * longer used in the kernel or by the Java interpreter.
 */

#ifndef SRC_INCLUDE_STDIO_H_
#define SRC_INCLUDE_STDIO_H_

#include <os/config.h>
#include <os/lib.h>
#include <os/wait.h>
#include <os/console.h>
#include <os/wait.h>
#include <os/types.h>
#include <os/time.h>
#include <os/unused.h>
#include <os/stdio.h>

#define FILENAME_MAX 4095

typedef void *FILE;

FILE *stdin;

FILE *stdout;

FILE *stderr;

int ungetc(int c, FILE *stream);

int printf(const char *fmt, ...);

int fprintf(void *s UNUSED_ATTRIBUTE, const char *fmt, ...);

int vfprintf(void *s UNUSED_ATTRIBUTE, const char *fmt, ...);

void perror (const char* prepend);

int fputs(const char *s, FILE *stream);

int rename(const char *oldpath, const char *newpath);

int remove(const char *pathname);

void rewind(FILE *stream);

int setvbuf(FILE *restrict stream, char *restrict buf, int type, size_t size);

/* This is used by the experimental, rudimentary file system ported to this
environment (can be linked from the packages repository) */ 

#ifdef ENABLE_FATFS

#include <sys/stat.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE *fopen (const char *path, const char *mode);

FILE *fopen64 (const char *path, const char *mode);

FILE *fdopen(int fildes, const char *mode);

int fclose(FILE *stream);

long ftell(FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

int fseek(FILE *stream, unsigned long offset, int whence);

int fileno(FILE *stream);

void clearerr(FILE *stream);

int feof(FILE *stream);

int ferror(FILE *stream);

int fflush(FILE *stream);

int fgetc(FILE *stream);

int fgetpos(FILE *restrict stream, fpos_t *restrict pos);

int getc(FILE *stream);

char *fgets(char *s, int size, FILE *stream);

int getchar(void);

int fsetpos(FILE *stream, const fpos_t *pos);

FILE *freopen(const char *restrict pathname, const char *restrict mode, FILE *restrict stream);

size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);

int fseeko64(FILE *stream, off_t offset, int whence);

off_t ftello64(FILE *stream);

int putc(int c, FILE *stream);

#endif

#endif
