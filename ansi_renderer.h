#pragma once

#include <_bsd_types.h>

void ar_sleep(long milli);
int ar_getRows();
int ar_getCols();
int setTitle(char *s);

int bufferSet_s(int row, int col, char *s, u_short r, u_short g, u_short b, u_short bg_r, u_short bg_g, u_short bg_b);
int bufferSet(int row, int col, char c, u_short r, u_short g, u_short b, u_short bg_r, u_short bg_g, u_short bg_b);

int addToBuffer(int row, char *s);
int addToBuffer_and_free(int row, char *s);
int replaceInBuffer(int row, char *s);
int replaceInBuffer_and_free(int row, char *s);
char *formCS(char *s, int r, int g, int b);
char *formBGCS(char *s, int r, int g, int b);
char *formR(void);
int initRenderer(int lineCount, int maxLineLength, bool flushBufferOnRender, bool useDifferenceMode);
int exitRenderer();
int render();
