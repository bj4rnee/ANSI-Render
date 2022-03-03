//[WIP] give direct, array-like access to horizontal render buffer      ('pixel' uses space of 40 chars; 39 opcode and 1 char) => 3D render buffer
//[WIP] implement true high performance difference rendering            (manual conventional render mode / difference mode toggle; engine has to auto detect changes)
// TODO check allocated memory for errors (good practice) like so:
/*   if (!ptr)
        err(1, "ptr");*/

// TODO check syscalls GetStdHandle, etc. for errors and handle them like line ~40
// TODO overflow check with 'strncpy' on every 'strcpy' call
// TODO add functionality for interactive / text input
// TODO make ansi_renderer C++ compatible

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ANSI ESCAPES & GENERAL FUNCTIONALITY
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef _WIN32
// Some old MinGW/CYGWIN distributions don't define this:
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE stdoutHandle;
static DWORD outModeInit;

#define ANSI_FORMAT 39 // number of chars for ansi format
int cols = -1;         // x
int rows = -1;         // y
COORD old_screen_buffer_size;

void setupConsole(void)
{
    DWORD outMode = 0;
    stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (stdoutHandle == INVALID_HANDLE_VALUE)
    {
        exit(GetLastError());
    }

    if (!GetConsoleMode(stdoutHandle, &outMode))
    {
        exit(GetLastError());
    }

    outModeInit = outMode;

    // Enable ANSI escape codes
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(stdoutHandle, outMode))
    {
        exit(GetLastError());
    }
}
void restoreConsole(void)
{
    // Reset colors
    printf("\x1b[0m");

    // Reset console mode
    if (!SetConsoleMode(stdoutHandle, outModeInit))
    {
        exit(GetLastError());
    }
}

#else
void setupConsole(void) {}

void restoreConsole(void)
{
    // Reset colors
    printf("\x1b[0m");
}
#endif

enum ClearCodes
{
    CLEAR_FROM_CURSOR_TO_END,
    CLEAR_FROM_CURSOR_TO_BEGIN,
    CLEAR_ALL
};

void clearScreen(void)
{
    printf("\x1b[%dJ", CLEAR_ALL);
}

void clearScreenToBottom(void)
{
    printf("\x1b[%dJ", CLEAR_FROM_CURSOR_TO_END);
}

void clearScreenToTop(void)
{
    printf("\x1b[%dJ", CLEAR_FROM_CURSOR_TO_BEGIN);
}

void clearLine(void)
{
    printf("\x1b[%dK", CLEAR_ALL);
}

void clearLineToRight(void)
{
    printf("\x1b[%dK", CLEAR_FROM_CURSOR_TO_END);
}

void clearLineToLeft(void)
{
    printf("\x1b[%dK", CLEAR_FROM_CURSOR_TO_BEGIN);
}

void moveUp(int positions)
{
    printf("\x1b[%dA", positions);
}

void moveDown(int positions)
{
    printf("\x1b[%dB", positions);
}

void moveRight(int positions)
{
    printf("\x1b[%dC", positions);
}

void moveLeft(int positions)
{
    printf("\x1b[%dD", positions);
}

void moveLeftAbsolute(void)
{
    printf("\x1b[1G");
}

void moveTo(int row, int col)
{
    printf("\x1b[%d;%df", row, col);
}

void hideCursor(void)
{
    printf("\033[?25l");
}

void ar_showCursor(void)
{
    printf("\033[?25h");
}

void moveToNoFormat(int n, int m)
{
    printf("\x1b[%d;%dH", n, m);
}

// like putColorString but with coordinates and a single char
void drawChar(char c, int x, int y, int r, int g, int b, int bg_r, int bg_g, int bg_b)
{
    printf("\x1b[%i;%iH", y, x);
    printf("\x1b[38;2;%d;%d;%dm", r, g, b);
    printf("\x1b[48;2;%d;%d;%dm", bg_r, bg_g, bg_b);
    printf("%c", c);
}

// set title of terminal window. Returns 0 if successful
// IMPORTANT: this function can be used without initializing render engine!!
int setTitle(char *s)
{
#if defined(_WIN32)
    HANDLE handleConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitle(s);
    return 0;
#elif defined(__APPLE__) || defined(__linux__)
    printf("\033]0;%s%c", s, '\007');
    return 0;
#endif
    return 1;
}

// removes terminal scrollbar and resizes the window. NO PROMISE CAN BE GIVEN ABOUT THE SUCCESS OF THE RESIZING OPERATION
// returns 0 if successful
int remove_scrollbar_and_resize(int x_, int y_)
{
#if defined(__APPLE__) // can't remove scrollbar on MacOS
    printf("\e[8;%d;%dt", x_, y_);
    return 1;
#elif defined(__linux__) // can't remove scrollbar on Linux
    // resize -s x_ y_
    return 1;
#elif defined(_WIN32)
    /*CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    old_screen_buffer_size = csbi.dwSize;

    COORD new_screen_buffer_size;
    new_screen_buffer_size.X = x_;
    new_screen_buffer_size.Y = y_;

    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), new_screen_buffer_size);*/
    CONSOLE_SCREEN_BUFFER_INFOEX csbiex;
    csbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    GetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &csbiex);
    csbiex.dwSize.X = x_;
    csbiex.dwSize.Y = y_;
    SMALL_RECT s1;
    s1.Top = 0;
    s1.Left = 0;
    s1.Bottom = y_;
    s1.Right = x_;
    csbiex.srWindow = s1;
    COORD mws1;
    mws1.X = x_;
    mws1.Y = y_;
    csbiex.dwMaximumWindowSize = mws1;
    SetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &csbiex);
    return 0;
#endif
    return 1;
}

// writes terminal dimensions to 'rows' and 'cols'
// returns 0 if successful
int window_dimensions()
{
#if defined(__APPLE__) || defined(__linux__)
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    rows = w.ws_row;
    cols = w.ws_col;
    return 0;
#elif defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    return 0;
#endif
    return 1;
}

// Returns number of columns (x-Dimension)

int ar_getCols()
{
    window_dimensions();
    return cols;
}

// Returns number of rows (y-Dimension)
// IMPORTANT: this function can be used without initializing render engine!!
int ar_getRows()
{
    window_dimensions();
    return rows;
}

// RENDERER

int lines = 1;     // max number of lines (y)
int maxLine = 100; // Max character length of line (x)
bool enableBufferFlush = true;
bool enableDifferenceMode = false;
bool **dif_matrix = NULL;   // needs to be dynamicly allocated to not potentially blow the stack
char *concat_buffer = NULL; // concatinated string buffer
char **buffer = NULL;
char ***buffer_3d = NULL; // three dimensional render buffer

void ar_sleep(long milli)
{
    clock_t end, current = clock();
    for (end = current + milli; current < end; current = clock())
        ;
}

// [WIP] add overflow checks to 'addToBuffer' and 'replaceInBuffer'

int addToBuffer(int row, char *s)
{
    if (row <= (lines - 1)) // no overflow check yet!!
    {
        char *newL = strcat(buffer[row], s);
        strcpy(buffer[row], newL);
        return 0;
    }
    else
    {
        return 1;
    }
}

int replaceInBuffer(int row, char *s)
{
    if (row <= (lines - 1)) //&& (sizeof(s) <= maxLine * sizeof(char)
    {
        strcpy(buffer[row], s);
        return 0;
    }
    else
    {
        return 1;
    }
}

// same function as 'addToBuffer' but the parameter 's' is free'd
int addToBuffer_and_free(int row, char *s)
{
    if (row <= (lines - 1)) // no overflow check yet!!
    {
        char *newL = strcat(buffer[row], s);
        strcpy(buffer[row], newL);
        free(s);
        return 0;
    }
    else
    {
        free(s);
        return 1;
    }
}

// same function as 'replaceInBuffer' but the parameter 's' is free'd
int replaceInBuffer_and_free(int row, char *s)
{
    if (row <= (lines - 1)) //&& (sizeof(s) <= maxLine * sizeof(char)
    {
        strcpy(buffer[row], s);
        free(s);
        return 0;
    }
    else
    {
        free(s);
        return 1;
    }
}

//                 y        x
int bufferSet(int row, int col, char c, u_short r, u_short g, u_short b, u_short bg_r, u_short bg_g, u_short bg_b)
{
    if (row <= (lines - 1) && col <= (maxLine / (ANSI_FORMAT + 1)))
    {
        char *buf = (char *)calloc(ANSI_FORMAT + 1, (sizeof(char)));
        snprintf(buf, ANSI_FORMAT + 1, "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm%c", r, g, b, bg_r, bg_g, bg_b, c);
        strcpy(buffer_3d[row][col], buf); // TODO forbidden call
        free(buf);
        return 0;
    }
    else
    {
        return 1;
    }
}

int bufferSet_s(int row, int col, char *s, u_short r, u_short g, u_short b, u_short bg_r, u_short bg_g, u_short bg_b)
{
    if (row <= (lines - 1) && col <= (maxLine / (ANSI_FORMAT + 1)))
    {
        bool truncated = false;
        char *buf_0 = (char *)calloc(ANSI_FORMAT + 1, (sizeof(char)));
        snprintf(buf_0, ANSI_FORMAT + 1, "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm%c", r, g, b, bg_r, bg_g, bg_b, s[0]);
        strcpy(buffer_3d[row][col], buf_0);
        char buf_1[2] = "\0";
        // check if string fits in current line
        if (col + strlen(s) <= (maxLine / (ANSI_FORMAT + 1)))
        {
            for (int i = 1; i < strlen(s); i++)
            {
                buf_1[0] = s[i];
                strcpy(buffer_3d[row][col + i], buf_1);
            }
        }
        else // string does not fit in current line
        {
            int i;
            for (i = 1; i < ((maxLine / (ANSI_FORMAT + 1)) - col); i++) // fill current line
            {
                buf_1[0] = s[i];
                strcpy(buffer_3d[row][col + i], buf_1);
            }
            // fill consecutive lines
            int c = 0;
            i++; // i = ((maxLine / (ANSI_FORMAT + 1)) - col)
            int rem = strlen(s) - ((maxLine / (ANSI_FORMAT + 1)) - col);
            for (int j = row + 1; j < row + 1 + ((strlen(s) - ((maxLine / (ANSI_FORMAT + 1)) - col)) / (maxLine / (ANSI_FORMAT + 1))); j++) // for every extra line
            {
                if (j > (lines - 1))
                {
                    truncated = true;
                    break;
                }
                c = min(rem, (maxLine / (ANSI_FORMAT + 1)));
                // fill line if it exists
                // for (int i = ((maxLine / (ANSI_FORMAT + 1)) - col); i < strlen(s); i++)
                for (int n = 0; n < c; n++)
                {
                    buf_1[0] = s[i];
                    strcpy(buffer_3d[j][n], buf_1);
                    rem--;
                    i++;
                }
            }
        }

        free(buf_0);
        if (truncated == true)
        {
            return 2;
        }
        return 0;
    }
    else
    {
        return 1;
    }
}

void flushBuffer()
{
    for (int i = 0; i < lines; i++)
    {
        memset(buffer[i], 0, sizeof(buffer[i][0]) * maxLine);
    }
}

void flushConcatBuffer()
{
    memset(concat_buffer, 0, sizeof(concat_buffer[0]) * maxLine * lines);
}

void flushBuffer_3d()
{
    memset(buffer_3d, 0, (lines * sizeof(char *)) + (lines * (maxLine / (ANSI_FORMAT + 1)) * sizeof(char **)) + (lines * (maxLine / (ANSI_FORMAT + 1)) * (ANSI_FORMAT + 1) * sizeof(char)));
}

void flushDifMatrix()
{
    for (int i = 0; i < lines; i++)
    {
        memset(dif_matrix[i], 0, sizeof(dif_matrix[i][0]) * (maxLine / (ANSI_FORMAT + 1)));
    }
}

/**
 * @brief deletes current line and updates it
 *
 * @return int 0
 */
int renderLine(char *line)
{
    // moveLeftAbsolute(); redundant
    // clearLine(); shit performance
    printf(line);
    fflush(stdout);
    // clearLineToRight();
    return 0;
}

void putColorString(char *s, int r, int g, int b)
{
    printf("\x1b[38;2;%d;%d;%dm%s", r, g, b, s);
}

void putBGColorString(char *s, int r, int g, int b)
{
    printf("\x1b[48;2;%d;%d;%dm%s", r, g, b, s);
}

// POINTER returned from this function MUST BE FREE'D
char *formCS(char *s, int r, int g, int b)
{
    int l = strlen(s) + 20;
    char *buf = (char *)calloc(l, (sizeof(char))); // yap, this will cause a memory leak :)
    snprintf(buf, l, "\x1b[38;2;%d;%d;%dm%s", r, g, b, s);
    return buf;
}

// POINTER returned from this function MUST BE FREE'D
char *formBGCS(char *s, int r, int g, int b)
{
    int l = strlen(s) + 20;
    char *buf = (char *)calloc(l, (sizeof(char)));
    snprintf(buf, l, "\x1b[48;2;%d;%d;%dm%s", r, g, b, s);
    return buf;
}

// default colorartion
void putPlainString(char *s)
{
    printf("\x1b[0m");
    printf(s);
}

char *formR(void)
{
    return "\x1b[0m";
}

//                       y    ,           x      ,             true        ,           false
int initRenderer(int lineCount, int maxLineLength, bool flushBufferOnRender, bool useDifferenceMode)
{
    remove_scrollbar_and_resize(maxLineLength, lineCount);
    window_dimensions();
    setupConsole();
    if (lineCount <= rows)
    {
        lines = lineCount;
    }
    else
    {
        lines = rows;
    }

    if (maxLineLength <= cols)
    {
        maxLine = maxLineLength * (ANSI_FORMAT + 1);
    }
    else
    {
        maxLine = cols * (ANSI_FORMAT + 1);
    }

    enableDifferenceMode = useDifferenceMode;
    enableBufferFlush = flushBufferOnRender;

    buffer = (char **)calloc(lines, sizeof(char *)); // allocate standard render buffer
    for (int i = 0; i < lines; i++)
    {
        buffer[i] = (char *)calloc(maxLine, sizeof(char));
    }

    concat_buffer = (char *)calloc(lines * maxLine, sizeof(char *)); // allocate concatenated render buffer

    dif_matrix = calloc(lines, sizeof(bool *)); // allocate difference matrix
    for (int i = 0; i < lines; i++)
    {
        dif_matrix[i] = calloc((maxLine / (ANSI_FORMAT + 1)), sizeof(bool));
    }

    // TODO this is utter garbage
    buffer_3d = malloc((lines * sizeof(char *)) + (lines * (maxLine / (ANSI_FORMAT + 1)) * sizeof(char **)) + (lines * (maxLine / (ANSI_FORMAT + 1)) * (ANSI_FORMAT + 1) * sizeof(char))); // dynamically allocate 3d render buffer
    for (int i = 0; i < lines; ++i)
    {
        buffer_3d[i] = (char **)(buffer_3d + lines) + i * (maxLine / (ANSI_FORMAT + 1));
        for (int j = 0; j < (maxLine / (ANSI_FORMAT + 1)); ++j)
        {
            buffer_3d[i][j] = (char *)((buffer_3d + lines + lines * (maxLine / (ANSI_FORMAT + 1))) + i * (maxLine / (ANSI_FORMAT + 1)) * (ANSI_FORMAT + 1) + j * (ANSI_FORMAT + 1));
        }
    }
    flushBuffer_3d();

    size_t vbuf = -1;
    if ((lines * maxLine) < INT_MAX)
    {
        vbuf = lines * maxLine * sizeof(char);
    }
    else
    {
        vbuf = INT_MAX * sizeof(char);
    }
    setvbuf(stdout, NULL, _IOFBF, vbuf); // VERY important line, speeds up output A LOT!! (see 'tutorialspoint.com/c_standard_library/c_function_setvbuf.htm' for doc)

    // flushBuffer();
    hideCursor();
    clearScreen();
    return 0;
}

int exitRenderer()
{
    printf(formR());
    restoreConsole();
    ar_showCursor(); // TODO fix cursor not showing up
    // avoid memory leak
    for (int i = 0; i < lines; i++)
    {
        free(buffer[i]);
        free(dif_matrix[i]);
    }
    free(buffer);
    free(dif_matrix);
    free(concat_buffer);
    free(buffer_3d);
    setvbuf(stdout, NULL, _IONBF, 0); // reset buffer TODO find out default value
    return 0;
}

/** Renders one full frame
 *
 * @return 0
 */
int render()
{
    if (!enableDifferenceMode)
    {
        // concatenated buffer renderer
        moveToNoFormat(1, 1);
        for (int i = 0; i < lines; i++)
        {
            strcat(concat_buffer, buffer[i]);
            if (i != lines - 1) // i isn't the last line
            {
                strcat(concat_buffer, "\n");
            }
        }
        renderLine(concat_buffer);
        flushConcatBuffer();

        /* NON-concatenated buffer renderer
        for (int i = 0; i < lines; i++)
        {
        moveToNoFormat((i + 1), 1);
        renderLine(buffer[i]);
        }*/

        if (enableBufferFlush)
        {
            flushBuffer();
            flushBuffer_3d();
        }
        return 0;
    }
    // true difference mode renderer
    return 1;
}

/*
int main()
{
    //TESTCUBE
    initRenderer(10, 50, false);
    addToBuffer(0, formCS("Animation:", 125, 125, 125));

    for (int i = 1; i < lines; i++)
    {
        addToBuffer(i, formCS("rendering...", 255, 255, 255));
    }

    while (true)
    {
        int r = rand() % 255 + 0;
        int g = rand() % 255 + 0;
        int b = rand() % 255 + 0;
        render();
        for (int i = 1; i < lines; i++)
        {
            replaceInBuffer(i, formCS("###################", r, g, b));
        }
        ar_sleep(900);
    }
    exitRenderer();


    //CHRISTMAS TREE
    initRenderer(18, 30, true);
    while (true)
    {
        addToBuffer(0, formCS("         |", 230, 235, 82));

        addToBuffer(1, "        -+-");

        addToBuffer(2, formCS("         A", 6, 71, 18));

        addToBuffer(3, "        /=\\");

        addToBuffer(4, formCS("      i", 230, 168, 25));
        addToBuffer(4, formCS("/ ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(4, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(4, formCS("O", 255, 255, 255));
        }
        addToBuffer(4, formCS(" \\", 6, 71, 18));
        addToBuffer(4, formCS("i", 230, 168, 25));

        addToBuffer(5, formCS("      /=====\\", 6, 71, 18));

        addToBuffer(6, "      /  ");
        addToBuffer(6, formCS("i", 230, 168, 25));
        addToBuffer(6, formCS("  \\", 6, 71, 18));

        addToBuffer(7, formCS("    i", 230, 168, 25));
        addToBuffer(7, formCS("/ ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(7, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(7, formCS("O", 255, 255, 255));
        }
        addToBuffer(7, formCS(" * ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(7, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(7, formCS("O", 255, 255, 255));
        }
        addToBuffer(7, formCS(" \\", 6, 71, 18));
        addToBuffer(7, formCS("i", 230, 168, 25));

        addToBuffer(8, formCS("    /=========\\", 6, 71, 18));

        addToBuffer(9, formCS("    /  * ", 6, 71, 18));
        addToBuffer(9, formCS("i", 230, 168, 25));
        addToBuffer(9, formCS(" *  \\", 6, 71, 18));

        addToBuffer(10, formCS("  i", 230, 168, 25));
        addToBuffer(10, formCS("/ ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(10, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(10, formCS("O", 255, 255, 255));
        }
        addToBuffer(10, formCS("   *   ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(10, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(10, formCS("O", 255, 255, 255));
        }
        addToBuffer(10, formCS(" \\", 6, 71, 18));
        addToBuffer(10, formCS("i", 230, 168, 25));

        addToBuffer(11, formCS("  /=============\\", 6, 71, 18));

        addToBuffer(12, formCS("  /  ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(12, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(12, formCS("O", 255, 255, 255));
        }
        addToBuffer(12, "   ");
        addToBuffer(12, formCS("i", 230, 168, 25));
        addToBuffer(12, "   ");
        if (rand() % 2 == 0)
        {
            addToBuffer(12, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(12, formCS("O", 255, 255, 255));
        }
        addToBuffer(12, formCS("  \\", 6, 71, 18));

        addToBuffer(13, formCS("i", 230, 168, 25));
        addToBuffer(13, formCS("/ *   ", 6, 71, 18));
        if (rand() % 2 == 0)
        {
            addToBuffer(13, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(13, formCS("O", 255, 255, 255));
        }
        addToBuffer(13, "   ");
        if (rand() % 2 == 0)
        {
            addToBuffer(13, formCS("O", 158, 24, 24));
        }
        else
        {
            addToBuffer(13, formCS("O", 255, 255, 255));
        }
        addToBuffer(13, formCS("   * \\", 6, 71, 18));
        addToBuffer(13, formCS("i", 230, 168, 25));

        addToBuffer(14, formCS("/=================\\", 6, 71, 18));

        addToBuffer(15, formCS("       |  `|", 87, 44, 1));

        addToBuffer(16, formCS("      `| ' |", 87, 44, 1));

        addToBuffer(17, formCS("~~~~~~~", 230, 230, 230));
        addToBuffer(17, formCS("|___|", 87, 44, 1));
        addToBuffer(17, formCS("~~~~~~~", 230, 230, 230));
        render();
        ar_sleep(900);
    }
    exitRenderer();

    return 0;
}
*/