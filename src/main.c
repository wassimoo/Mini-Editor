#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include "main.h"

static EDITOR E;
static struct termios stored_settings;

int main(int argc, char *argv[]){
    initEditor();
    while (1)
    {
        drawScreen();
        process_key(get_key());
    }
    return 1;
}

void insertNewLine(){
    E.cy++;
    E.cx = 0;
    E.coloff = 0;
    E.numrows++;
    E.row_index++;
    E.row = realloc(E.row, E.numrows * sizeof(ROW));
    E.row[E.row_index].chars = malloc(1);
    E.row[E.row_index].chars[0] = '\0';
    E.row[E.row_index].length = 1;
    if (E.numrows > E.screen_rows)
        E.rowoff++;
    //TO BE CONTINUED :)
}

void write_to_file(void){
    E.file = fopen("this.txt", "w");
    //TODO: handle failure on opening file
    int i;
    for (i = 0; i < E.numrows; i++, E.row++)
    {
        fputs(E.row->chars, E.file);
        fputc('\n', E.file);
    }
}

void textAppend(struct Text *txt, char *s, int len){
    char *new = realloc(txt->b, txt->len + len);

    if (new == NULL)
        return;
    memcpy(new + txt->len, s, len);
    txt->b = new;
    txt->len += len;
}

void drawScreen(void){
    struct Text txt = {NULL, 0};
    textAppend(&txt, "\x1b[?25l", 6); /*Hide*/
    textAppend(&txt, "\x1b[H", 3);  /*Go home*/

    int i;
    int rowIndex = E.row_index;
    for (i = 0,rowIndex=E.rowoff;i < E.screen_rows ; i++)
    {
        if(rowIndex < E.numrows){
            textAppend(&txt, E.row[rowIndex].chars, E.row[rowIndex].length);
            textAppend(&txt, "\x1b[0K\r\n", 7);
            rowIndex++;
        }
        else{
                textAppend(&txt, "~\x1b[0K\r\n", 7);
        }
    }
    textAppend(&txt, "\x1b[?25h", 6); /* Show cursor. */
    write(STDOUT_FILENO, txt.b, txt.len);
    cursorGO(E.cy+1,E.cx+1); 
}

/* This shit is fckin working */
void move_cursor(int direction){
    ROW *row = E.row + E.row_index;
    switch (direction)
    {
    case ARROW_DOWN:
        if (E.rowoff + E.cy < E.numrows)
        {
            if (E.cy == E.screen_rows - 1)
                E.rowoff++;
            else
            {
                E.cy++;
            }
            E.row_index++;
        }
        break;

    case ARROW_UP:
        if (E.cy > 0)
        {
            E.row_index--;
            E.cy--;
        }
        else if (E.rowoff > 0)
        {
            E.row_index--;
            E.rowoff--;
        }

        break;

    case ARROW_RIGHT:

        if (E.cx + E.coloff < row->length - 1)
        {
            if (E.cx == E.screen_cols - 1)
                E.coloff++;
            else
                E.cx++;
        }
        else if (E.cx + E.coloff == row->length - 1) /*Let's try to jump to next line */
        {
            if (E.row_index < E.numrows - 1)
            {
                E.cx = 0;
                E.coloff = 0;
                if (E.cy + E.rowoff == E.screen_rows - 1)
                    E.rowoff++;
                else
                {
                    E.cy++;
                }
                E.row_index++;
            }
        }
        break;

    case ARROW_LEFT:
        if (E.cx > 0)
            E.cx--;
        else
        { /*On the left Edge */
            if (E.coloff > 0)
                E.coloff--;
            else
            { /*Go up */
                if (E.row_index > 0)
                {
                    E.cx = E.row[E.row_index - 1].length - 1;
                    if (E.cy == 0)
                    {
                        if (E.rowoff > 0)
                            E.rowoff--;
                    }
                    else
                        E.cy--;
                    E.row_index--;
                    if (E.cx > E.screen_cols - 1)
                    {
                        E.coloff = E.cx - E.screen_cols + 1;
                        E.cx = E.screen_cols - 1;
                    }
                }
            }
        }
        break;
    }
    /*Move cursor to last char if it's out of string due to Up and Down Arrows*/
    row = E.row + E.row_index;
    if (E.cx > row->length - 1)
    {
        E.cx = row->length - 1; //TODO : Handle offset screen cols
    }
    cursorGO(E.cy + 1, E.cx + 1);
}

/*Delete one single char at a specific position */
void deletechar(int pos, ROW *row){
    memmove(row->chars + pos, row->chars + pos + 1, row->length - pos);
    row->length--;
    E.cx--;
}

void deleteProcess(ROW *row, ROW *prevRow) /*incomplet (a little bit complicated)*/{
    if (E.cx > 0)
    { /*we have somthing to delete*/
        deletechar(E.cx - 1, row);
    }
    else if (E.cy > 0)
    { /*handle the case where there's no line suppression*/
        resizeString(prevRow->chars,
                     prevRow->length + row->length - 1);                        /*-1 for one single null term*/
        memmove(prevRow->chars + prevRow->length - 2, row->chars, row->length); /*-2 to move null term + newline term*/
        prevRow->length = prevRow->length + row->length - 1;

        /*move rows by 1 position to the left and free() last row */
        if (E.row_index > E.numrows - 1)
            row = memmove(row, E.row + E.row_index + 1, E.numrows - E.row_index - 1);

        cursorup(1);
        cursorforward(prevRow->length);
        E.row_index--;
        E.numrows--;
    }
}

char *resizeString(char *chars, int newSize){
    char *tmp;
    if (!(tmp = realloc(chars, newSize)))
        exit(-2); /*Could not alloc enough space for row*/
    return tmp;
}

void insertChar(char c){ 
    /*-exec p 'main.c'::E.row+1*/
    /*ROW *debug_r = E.row;*/

    E.row[E.row_index].chars = resizeString(E.row[E.row_index].chars, E.row[E.row_index].length + 1);
    E.row[E.row_index].chars[E.row[E.row_index].length - 1] = c;
    E.row[E.row_index].chars[E.row[E.row_index].length] = '\0';
    // printf("%c", E.row[E.row_index].chars[E.row[E.row_index].length - 1]); //(printf("%c",c))
    E.row[E.row_index].length++;
    /* if (E.cx == E.screen_cols)
        {
            cursordown(1);
            cursorbackward(E.row[E.row_index].length);
            E.cx = 0;
            E.cy++;
        }
        else*/
    E.cx++;
}

int get_key(void){
    char c;
    c = fgetc(stdin);
    if (c == 'z')
    {
        write_to_file();
        echo_on();
        exit(0);
    }

    else if (c == ESC)
    {
        getchar();
        c = getchar();
        /*escape sequence */
        switch (c)
        {
        case 'A':
            return ARROW_UP;
        case 'B':
            return ARROW_DOWN;
        case 'C':
            return ARROW_RIGHT;
        case 'D':
            return ARROW_LEFT;
        case 'H':
            return HOME_KEY;
        case 'F':
            return END_KEY;
        }
    }
    else
    {
        return c;
    }
}

void process_key(int key){
    switch (key)
    {
    case ARROW_LEFT:
    case ARROW_RIGHT:
    case ARROW_UP:
    case ARROW_DOWN:
        move_cursor(key);
        break;
    case BACKSPACE:
        printf("\b");
        printf(" ");
        printf("\b");
        if (E.cx > 0)
            deleteProcess(E.row + E.row_index, NULL);
        else
            deleteProcess(E.row + E.row_index, E.row + E.row_index - 1);
        break;
    case ENTER:
        insertNewLine();
        cursorGO(E.cy + 1, E.cx + 1);
        break;
    default:
        insertChar(key);
        break;
    }
}

void initEditor(){
    echo_off();
    E.row = calloc(1, sizeof(E));
    E.row->length = 1;
    E.row_index = 0;
    E.row->chars = malloc(sizeof(char));
    E.row->chars[0] = '\0';
    E.row_index = 0;
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 1;
    struct winsize w;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
    E.screen_rows = w.ws_row-1;
    E.screen_cols = w.ws_col-1;
}

void echo_off(void){
    struct termios new_settings;
    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 1;
    tcsetattr(0, TCSANOW, &new_settings);
    return;
}

void echo_on(void){
    tcsetattr(0, TCSANOW, &stored_settings);
    return;
}