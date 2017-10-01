#define cursorup(x) printf("\033[%dA", (x))
#define cursordown(x) printf("\033[%dB", (x))
#define cursorforward(x) printf("\033[%dC", (x))
#define cursorbackward(x) printf("\033[%dD", (x))
#define cursorGO(y,x) printf("\033[%d;%dH", y , x)
enum KEY_ACTION
{
    KEY_NULL = 0,    /* NULL */
    CTRL_C = 3,      /* Ctrl-c */
    CTRL_D = 4,      /* Ctrl-d */
    CTRL_F = 6,      /* Ctrl-f */
    CTRL_H = 8,      /* Ctrl-h */
    TAB = 9,         /* Tab */
    CTRL_L = 12,     /* Ctrl+l */
    ENTER = 10,      /* Enter */
    CTRL_Q = 17,     /* Ctrl-q */
    CTRL_S = 19,     /* Ctrl-s */
    CTRL_U = 21,     /* Ctrl-u */
    ESC = 27,        /* Escape */
    BACKSPACE = 127, /* Backspace */
    /* The following are just soft codes, not really reported by the
         * terminal directly. */
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

typedef struct
{
    u_int length;
    char *chars;
} ROW;

typedef struct
{
    u_int cx,
        cy, /*Cursor position */
        screen_rows,
        screen_cols,
        rowoff,  //To the up
        coloff, //To the left
        numrows;
    ROW *row;
    u_int row_index;
    FILE *file;
} EDITOR;

struct Text {
    char *b;
    int len;
};

void textAppend(struct Text *txt, char *s, int len);

void drawScreen(void);

char *resizeString(char *chars ,int newSize);

void process_key(int key);

void insertRow(void);

void insertNewLine();

void insertChar(char c);

void insertTab(void);

void initEditor();

void deleteProcess(ROW* row,ROW* prevRow);

int get_key(void);

void write_to_file(void);

void move_cursor(int direction);

void echo_on(void);

void echo_off(void);
