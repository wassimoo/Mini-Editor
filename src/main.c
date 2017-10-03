#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "main.h"

#define TABSPC 4
#define WAITBEFORESAVE 3
#define VERSION "0.0.1"
#define VERSIONLENTH 5

static EDITOR E;
static struct termios stored_settings;

int main(int argc, char *argv[]){
    if(argc < 2){
        E.fileName = strdup("tmp.txt");
        E.isTmp = 1;
    }
    else{
        E.fileName = strdup(argv[1]);
        E.isTmp = 0;
    }
    E.fileBaseName = getFileBaseName();
    initEditor();
    u_int waitFor = time(0) + WAITBEFORESAVE;
    while (1 )
    {
        if(time(0) >= waitFor && !E.isTmp){
            if(E.isDirty)
                write_to_file();
            waitFor += WAITBEFORESAVE;
        }
        drawScreen();
        process_key(get_key());
    }
    return 1;
}

void insertNewLine(){
    if(E.cy < E.screen_rows-1)
        E.cy++;
    insertRow();
    E.cx = 0;
    E.coloff = 0;
    E.row_index++;
    if (E.numrows > E.screen_rows && E.cy >= E.screen_rows-1)
        E.rowoff++;
    //TO BE CONTINUED :)
}

void insertRow(void){
    E.numrows++; //add new row to count
    E.row = realloc(E.row,E.numrows*sizeof(ROW)); //realloc rows in memmory to fit number of rows
    E.row[E.numrows-1].chars =  malloc(1);
    E.row[E.numrows-1].chars[0] = '\0';

    int i;
    for(i = E.numrows-1; i>E.row_index;i--){ //We Start moving memory blocks from last block till the block before rowIndex ;
        E.row[i].chars = resizeString(E.row[i].chars,E.row[i-1].length); //Resize strig to fit
        memmove(E.row[i].chars,E.row[i-1].chars,E.row[i-1].length); //move Line to next row
        E.row[i].length = E.row[i-1].length; //update ligne length
    }

    ROW *orig = &E.row[E.row_index];
    
    ROW *dest = &E.row[E.row_index+1];

    dest->length = orig->length - E.cx;
    dest->chars = malloc(dest->length);
    memmove(dest->chars,orig->chars+E.cx,dest->length);
    
    orig->chars = realloc(orig->chars, sizeof(char) * E.cx+1); //Resize with nedded chars  (E.cx) +1 for null term
    orig->length = E.cx+1;
    orig->chars [orig->length-1] = '\0';
    
}

void write_to_file(void){
    if((E.file = fopen(E.fileName, "w+")) == NULL){
        printf("\nCould not open File %s verify permissions.\n",E.fileName);
        exit(-4);
        }

    int i;
    for (i = 0; i < E.numrows; i++)
    {
        fputs(E.row[i].chars, E.file);
        fputc('\n', E.file);
    }
    fclose(E.file);
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
    textAppend(&txt, "\x1b[30;47m", 8); /*Black on white*/  
    textAppend(&txt, "\033[H", 3);  /*Go home*/
    titleBar(&txt);
    textAppend(&txt, "\x1b[0m", 5); /*Cyan*/      
    textAppend(&txt, "\x1b[0K\r\n", 7);
    
    //textAppend(&txt, "\x1b[42m", 5);
    //textAppend(&txt, "\x1b[0m", 5);
    
    int i;
    int rowIndex = E.row_index;
    for (i = 0,rowIndex=E.rowoff;i < E.screen_rows-1 ; i++)
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
    if(E.row_index>0){
            E.row_index--;
        if (E.cy > 1)
            E.cy--;
        else if (E.rowoff > 0)
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
                    if (E.cy == 1)
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

/*Delete single char at a specific position */
void deletechar(int pos, ROW *row){
    memmove(row->chars + pos, row->chars + pos + 1, row->length - pos);
    row->length--;
    E.cx--;
}

void deleteProcess(ROW *row, ROW *prevRow){
    if (E.cx > 0){ /*we have somthing to delete*/
        deletechar(E.cx - 1, row);
    }
    else if (E.cy > 1){ 
        /*move Row to upper Row*/
        prevRow->chars =  resizeString(prevRow->chars,prevRow->length + row->length - 1); /*-1 for one single null term*/
        memmove(prevRow->chars + prevRow->length - 1, row->chars, row->length); /*-2 to move null term + newline term*/
        E.cx = prevRow->length-1;        
        prevRow->length = prevRow->length + row->length - 1;     
        row->chars = realloc(row->chars,1);

    
        /*move all rows by 1 position up and free() last row */
        int i;
        for(i = E.row_index; i < E.numrows-1;i++){
            E.row[i].chars = realloc(E.row[i].chars,E.row[i+1].length);
            memmove(E.row[i].chars,E.row[i+1].chars,E.row[i].length);
            E.row[i].length = E.row[i+1].length;
        }
       
        cursorup(1);
        cursorforward(E.cx);
        E.row_index--;
        E.numrows--;
        E.cy--;
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
    char* pos =  E.row[E.row_index].chars+E.cx;
    memmove(pos+1,pos,E.row[E.row_index].length-E.cx);
    *pos = c;
    E.row[E.row_index].length++;
            /*Possibly \0 missing*/
    E.cx++;
}

void insertTab(void){
    ROW *line = &E.row[E.row_index];
    line->chars = resizeString(line->chars,line->length+TABSPC);
    memmove(line->chars+E.cx+TABSPC,line->chars+E.cx,line->length-E.cx);
    int i;
    for(i = 0;i<TABSPC; i++){
        line->chars[E.cx] = ' ';
        E.cx++;
    }
    line->length+=4;
    cursorforward(TABSPC);
}

int get_key(void){
    char c;
    c = fgetc(stdin);
    if (c == 'Q')
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
        if(c != -1)
            E.isDirty = 1;
        return c;
    }
}

void process_key(int key){
    switch (key)
    {
    case -1:
        break;
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
    case TAB :
        insertTab();
        break; 
    default:
        insertChar(key);
        break;
    }
}

/*Display the current version,the current filename and whether 
 *the current file has been modified on the titlebar*/
void titleBar (struct Text *txt){
    const char *editorName = "MINI ";
    int editorNameLen = strlen(editorName);
    int versionLength = VERSIONLENTH ;
    int fileBaseNameLen = strlen(E.fileBaseName);
    int remSpace = E.screen_cols - VERSIONLENTH - editorNameLen - fileBaseNameLen;
    //TODO : Handle case where remSpace <0;
    remSpace = (int) floor(remSpace/2);
    
    int dots = 0;
    char bar[E.screen_cols+1];

    int i;
    for(i = 0; i < E.screen_cols; i++)
        bar[i] = ' ';
    bar[E.screen_cols] = '\0';
    
    
    memmove(bar,editorName,editorNameLen);
    memmove(bar+editorNameLen,VERSION,VERSIONLENTH);
    memmove(bar+editorNameLen+VERSIONLENTH+remSpace,E.fileBaseName,fileBaseNameLen);
    bar[editorNameLen+VERSIONLENTH+remSpace+fileBaseNameLen] = E.isTmp ? '*' : ' ';;
    textAppend(txt,bar,E.screen_cols+1);
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
    E.cy = 1;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 1;
    struct winsize w;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
    E.screen_rows = w.ws_row-1;
    E.screen_cols = w.ws_col-1;
    E.isDirty = 0;
    
    struct Text txt = {NULL, 0};
    textAppend(&txt, "\x1b[H", 3);    
    titleBar(&txt);
    write(STDOUT_FILENO, txt.b, txt.len);
}

void echo_off(void){
    struct termios new_settings;
    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 1;
    tcsetattr(0, TCSANOW, &new_settings);
    return;
}

void echo_on(void){
    tcsetattr(0, TCSANOW, &stored_settings);
    return;
}

char* getFileBaseName(void){
    int i = strlen(E.fileName)-1 ;

    while(E.fileName[i] != '/' && i > 0){
        i--;
    }

    int baseNameLength = strlen(E.fileName - i);
    char* baseName = malloc(baseNameLength);
    strncpy(baseName, E.fileName, baseNameLength);
    return baseName;
}
//TODO : before pushing : test with long file path .