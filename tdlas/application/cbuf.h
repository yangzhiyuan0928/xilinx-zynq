#ifndef CBUF_H
#define CBUF_H
#include <stdio.h>
#include <stdlib.h>
 
#include "defines.h"
/* Opaque buffer element type.  This would be defined by the application. */
typedef struct { unsigned char buf[FIFO_SIZE+4]; } ElemType;
 
/* Circular buffer object */
typedef struct {
    int         size;   /* maximum number of elements           */
    int         start;  /* index of oldest element              */
    int         end;    /* index at which to write new element  */
    ElemType   *elems;  /* vector of elements                   */
} CircularBuffer;

typedef unsigned char u8;
void cbInit(CircularBuffer *cb, int size);
void cbFree(CircularBuffer *cb);
void cbFree(CircularBuffer *cb);
int cbIsFull(CircularBuffer *cb) ;
int cbIsEmpty(CircularBuffer *cb);
void cbWrite(CircularBuffer *cb, ElemType *elem) ;
void cbRead(CircularBuffer *cb, ElemType *elem) ;

#endif
