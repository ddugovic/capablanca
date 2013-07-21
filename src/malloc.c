/*
   Copyright (C) Andrew Tridgell 2002
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
  a malloc wrapper library for the dynamically loaded part of
  chessd. This is used both as a malloc checker library and to prevent
  some bad interactions between the dl library and malloc
*/


#include "includes.h"
//#undef free
//#undef malloc

#define MAGIC1 0x28021967
#define MAGIC2 0x26011966

struct malloc_list {
	void *ptr;
	struct malloc_list *next;
	struct malloc_list *prev;
};

static struct malloc_list *mlist;
static int m_total;

struct malloc_header {
	struct malloc_list *m;
	unsigned size;
	unsigned magic;
};

/*
  a magic-value protected malloc wrapper, 
  which can be globally freed using a single call
*/
void *m_malloc(size_t size)
{
	void *ptr;
	struct malloc_list *m;
	struct malloc_header *h;

	if (size == 0) return NULL;

	ptr = malloc(size+sizeof(struct malloc_header));

	if (!ptr) return NULL;

	m = malloc(sizeof(*m));

#if MALLOC_DEBUG
	fprintf(stderr,"malloc %p   %u\n", ptr, size);
	fprintf(stderr,"malloc %p\n", m);
#endif

	if (!m) {
		free(ptr);
		return NULL;
	}

	m->ptr = ptr;
	m->next = mlist;
	m->prev = NULL;
	if (mlist) {
		mlist->prev = m;
	}
	mlist = m;

	m_total++;

	memset(ptr, 0, size+sizeof(struct malloc_header));

	h = ptr;
	h->size = size;
	h->magic = MAGIC1;
	h->m = m;

	return (void *)(h+1);
}

/*
  determine the size of a malloced region
*/
static size_t m_malloc_size(void *ptr)
{
	struct malloc_header *h;
	if (!ptr) return 0;

	h = (struct malloc_header *)ptr;
	h--;

	return h->size;
}

/*
  check that a pointer is OK
*/
void m_malloc_check(void *ptr)
{
	struct malloc_header *h;

	if (!ptr) return;

	h = (struct malloc_header *)ptr;
	h--;

	if (h->magic != MAGIC1) {
		d_printf("malloc corruption ptr=%p v1=%x size=%d\n", 
			 ptr, h->magic, h->size);
	}
}

/*
  a paranoid free() function that checks for simple corruption
*/
int m_free(void *ptr)
{
	struct malloc_header *h;
	struct malloc_list *m;

	if (!ptr) return 0;

	h = (struct malloc_header *)ptr;
	h--;

	if (h->magic != MAGIC1) {
		d_printf("malloc corruption ptr=%p v1=%x size=%d\n", 
			ptr, h->magic, h->size);
		return -1;
	}

	m = h->m;
	if (!m || m->ptr != h) {
		d_printf("malloc list corruption!\n");
	}
	if (m->next) {
		m->next->prev = m->prev;
	}
	if (m->prev) {
		m->prev->next = m->next;
	}
	if (m == mlist) {
		mlist = m->next;
	}

	m_total--;

#if MALLOC_DEBUG
	fprintf(stderr,"free   %p   %u\n", h, h->size);
	fprintf(stderr,"free   %p\n", m);
#endif

	memset(ptr, 'Z', h->size); /* paranoia */
	memset(m, 0, sizeof(*m));
	free(m);
	memset(h, 0, sizeof(*h));
	free(h);
	return 0;
}

/*
  check all allocated memory
*/
void m_check_all(void)
{
	struct malloc_list *m;
	for (m=mlist; m; m=m->next) {
		m_malloc_check(m->ptr + sizeof(struct malloc_header));
	}
}


/*
  free all allocated memory
*/
void m_free_all(void)
{
	struct malloc_list *m, *next;
	d_printf("Freeing %d chunks\n", m_total);
	for (m=mlist; m; m=next) {
		next = m->next;
		m_total--;

#if MALLOC_DEBUG
		fprintf(stderr,"free   %p\n", m->ptr);
		fprintf(stderr,"free   %p\n", m);
#endif
		free(m->ptr);
		free(m);
	}
	mlist = NULL;
}

/* a calloc wrapper */
void *m_calloc(size_t n, size_t size)
{
	void *ptr;
	ptr = m_malloc(n*size);
	if (ptr) memset(ptr, 0, n*size);
	return ptr;
}

/* strdup wrapper */
char *m_strdup(const char *s)
{
	int len = strlen(s);
	char *ret;
	ret = m_malloc(len+1);
	memcpy(ret, s, len+1);
	return ret;
}

/* strndup wrapper */
char *m_strndup(const char *s, size_t n)
{
	int len = strnlen(s, n);
	char *ret;
	ret = m_malloc(len+1);
	memcpy(ret, s, len);
	ret[len] = 0;
	return ret;
}

/* a free() call that zeros the pointer to prevent re-use */
void m_safe_free(void **ptr, const char *file, unsigned line)
{
	if (!*ptr) {
		d_printf("NULL free at %s(%u)!\n", file, line);
		return;
	}
	if (m_free(*ptr) != 0) {
		d_printf("bad free at %s(%u)!\n", file, line);
	}
	(*ptr) = NULL;
}


/* a vasprintf wrapper */
int m_vasprintf(char **strp, const char *fmt, va_list ap)
{
	int n;

	(*strp) = NULL;

	n = vsnprintf(NULL, 0, fmt, ap);
	if (n == 0) return 0;

	(*strp) = m_malloc(n+1);
	n = vsnprintf(*strp, n+1, fmt, ap);
	
	return n;
}

/* a asprintf wrapper */
int m_asprintf(char **strp, const char *fmt, ...)
{
	int n;
	va_list ap;

	va_start(ap, fmt);
	n = m_vasprintf(strp, fmt, ap);
	va_end(ap);

	return n;
}

/* a realloc wrapper */
void *m_realloc(void *ptr, size_t size)
{
	void *ret;
	size_t msize;

	if (!ptr) {
		return m_malloc(size);
	}

	if (size == 0) {
		m_safe_free(&ptr, __FILE__, __LINE__);
		return NULL;
	}

	ret = m_malloc(size);

	msize = m_malloc_size(ptr);
	memcpy(ret, ptr, size<msize?size:msize);
	m_safe_free(&ptr, __FILE__, __LINE__);
	return ret;
}
