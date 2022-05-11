#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../ansi_renderer.h"

int main()
{
    srand(time(NULL));
    setTitle("test 6: check for segfault in 3d buffer");
    initRenderer(30, 120, true, false);
    bufferSet_s(0, 0, "Animation:", 125,125,125,0,0,0);
    //addToBuffer_and_free(0, formCS("Animation:", 125, 125, 125));
    //addToBuffer(0, formR());

    for (int i = 1; i < 30; i++)
    {
        //addToBuffer_and_free(i, formCS("rendering...", 255, 255, 255));
        bufferSet_s(i,0,"rendering...", 255, 255, 255,0,0,0);
    }
    //addToBuffer(29, formR());

    for (int k = 0; k < 10; k++)
    {
        int r = rand() % 255 + 0;
        int g = rand() % 255 + 0;
        int b = rand() % 255 + 0;
        render();
        bufferSet(0,0,'#',187,24,187,0,0,0);
        for (int i = 1; i < 30; i++)
        {
            //addToBuffer_and_free(i, formBGCS("                                                                                                                        ", r, g, b));
            bufferSet_s(i,0,"                                                                                                                        ",0,0,0,r,g,b);
        }
        //addToBuffer(29, formR());
        //bufferSet_s(1, 3, "this is a test string", 24, 145, 255, 0, 0, 0);
        ar_sleep(900);
    }
    exitRenderer();
    ar_sleep(400);
    printf("done...\n\a"); //beep :}
    ar_sleep(60000);
    return 0;
}