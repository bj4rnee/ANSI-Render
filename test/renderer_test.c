#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ansi_renderer.h"

void test_drawChar(char c, int x, int y, int r, int g, int b, int bg_r, int bg_g, int bg_b)
{
    printf("\x1b[%i;%iH", y, x);
    printf("\x1b[38;2;%d;%d;%dm", r, g, b);
    printf("\x1b[48;2;%d;%d;%dm", bg_r, bg_g, bg_b);
    printf("%c", c);
}

int main()
{
    srand(time(NULL));
    setTitle("test 5: 3d buffer");
    initRenderer(30, 120, false, false);
    addToBuffer_and_free(0, formCS("Animation:", 125, 125, 125));
    addToBuffer(0, formR());

    for (int i = 1; i < 30; i++)
    {
        addToBuffer_and_free(i, formCS("rendering...", 255, 255, 255));
    }
    addToBuffer(29, formR());

    for (int k = 0; k < 10; k++)
    {
        int r = rand() % 255 + 0;
        int g = rand() % 255 + 0;
        int b = rand() % 255 + 0;
        render();
        for (int i = 1; i < 30; i++)
        {
            replaceInBuffer_and_free(i, formBGCS("                                                                                                                        ", r, g, b));
        }
        addToBuffer(29, formR());
        //bufferSet(1,3,'a',24,0,255,0,0,0);
        ar_sleep(900);
    }
    exitRenderer();
    ar_sleep(400);
    printf("done...\n\a");
    ar_sleep(60000);
    return 0;
}