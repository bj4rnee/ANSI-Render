#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ansi_renderer.h"

#define y 30 
#define x 120
#define p_size 1 //pixel size

// level of trail: 5 -> 4 -> 3 -> 2 -> 1 -> none
//[WIP] size screen accourding to teminal bounds, functionality exists

int screen[y][x] = {0};
bool direction[2] = {true, false}; // [x, y]

void sleep(long milli)
{
    clock_t end, current = clock();
    for (end = current + milli; current < end; current = clock())
        ;
}

void randomizePos()
//CHARS_FILLED    = ['░', '▒', '▓', '█']
{

    srand(time(NULL));
    int random_y_pos = rand() % (y - 10) + 1;
    int random_x_pos = rand() % (x - 30) + 1; 
    screen[random_y_pos][random_x_pos] = 5;
    screen[random_y_pos][random_x_pos + 1] = 5; //wtf thats wrong
    //randomize direction
    direction[0] = random_y_pos % 2;
    direction[1] = random_x_pos % 2;
}

void advance(int lcorner_y, int lcorner_x)
{
    //set new p
    if (direction[0])
    {
        if (direction[1])
        {
            screen[lcorner_y + 1][lcorner_x + 2] = 5;
            screen[lcorner_y + 1][lcorner_x + 3] = 5;
        }
    }
}

void add_b(int l, int col, int c)
{
    for (int i = 0; i < col; i++)
    {
        addToBuffer(l, " ");
    }
    addToBuffer(l, formBGCS("", 255, 255, 255));

    addToBuffer(l, "  ");

    addToBuffer(l, formR());
}

int main()
{
    initRenderer(y, x, true, false);
    randomizePos();
    for (int i = 0; i < y; i++)
    {
        for (int j = 0; j < x; j++)
        {
            if (screen[i][j] == 5)
            {
                add_b(i, j, screen[i][j]);
                j++;
                continue;
            }
        }
    }
    render();
    exitRenderer();

    /*char test[200000];
   for (int i = 0; i < y; i++)
    { 
        for (int j = 0; j < x; j++)
        {
            char str[2];
            snprintf( str, 2, "%d", j%10 );
            strcat(test,str); //
        }
    }
    printf(test);*/
    sleep(100000);
    return 0;
}