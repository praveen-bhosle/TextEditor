/*** includes  ***/
#include <unistd.h> 
#include <termios.h>
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f )  


/*** data  ***/

struct editorConfig { 
   int screenrows ; 
   int screencols ;
   struct termios original_termios ;
};

struct editorConfig E ; 

void disableRawMode() ; 
void enableRawMode() ; 
void die(const char* ) ;
char editorReadKey() ; 
void editorProcessKeypress() ;
int main() ; 


/*** terminal  ***/
void enableRawMode() { 
    if(tcgetattr(STDIN_FILENO,&E.original_termios)==-1) die("tcgetattr") ;
    atexit(disableRawMode) ;  
    struct termios raw = E.original_termios ; 
    raw.c_iflag &= ~(IXON) ; 
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | ICRNL )  ; // turning off echo and canonical mode.
    raw.c_oflag &= ~(OPOST) ; 
    raw.c_cc[VMIN]=0 ;  // mimimum bytes to be read before read returns.
    raw.c_cc[VTIME]=1;  // the time for which will read() will wait to return, 1 unit = 100ms. 
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)==-1) die("tcsetattr") ; 
}

void disableRawMode()  { 
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.original_termios) == -1 ) die("tcsetattr")  ;
}

void die(const char* s) { 
    perror(s) ; 
    exit(1) ; 
}

char  editorReadKey() { 
    int nread ; 
    char c ; 
    while((nread = read(STDIN_FILENO, &c , 1)) != 1   ) { 
        if(nread == -1) die("read") ;  
    } 
    return c ; 
}

int getCursorPosition(int* rows , int* cols) { 
    char buf[32] ; 
    unsigned int i = 0 ; 
    if(write(STDOUT_FILENO,"\x1b[6n",4)!=4) return -1 ; 
    while( i < sizeof(buf) -1 ) { 
        if(read(STDIN_FILENO,&buf[i],1)!=1) break ; 
        if(buf[i] == 'R') break ;
        i++ ;
    }
    buf[i]='\0' ;
    if(buf[0] != '\x1b' || buf[1]!='[' ) return -1 ; 
    if(sscanf(&buf[2] , "%d;%d" , rows , cols)!=2) return -1 ; 
    return 0 ;
}


int getWindowSize(int* rows , int* cols ) {  
    struct winsize ws ; 
    if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0 ) { 
        if(write(STDIN_FILENO,"\x1b[999C\x1b[999B",12)!=12) return -1 ;
        return  getCursorPosition(rows,cols); 
    } else { 
        *cols = ws.ws_col; 
        *rows = ws.ws_row ; 
        return 0  ; 
    }
}
// we are not using H by specifying larger coordinates to reach the edge of the screen because H is not documented to handle the cases when u might go out of screen.
// B and C makes sures that any large value u dont go out of screen.


/*** append buffer  ***/ 

struct abuf { 
    char* b ; 
    int len ; 
} ;

#define ABUF_INIT{NULL,0} 

void abAppend(struct abuf *ab , const char *s , int len ) { 
    char* new = realloc( ab->b , ab->len + len  ) ;
}

/*** input  ***/

void editorProcessKeypress() { 
    char c = editorReadKey() ; 
    switch (c) {
        case CTRL_KEY('q'):
            editorRefreshScreen(); // clearing the screen on exit.
            exit(0) ; 
            break;
    }
}

/** output  ***/

void editorRefreshScreen() { 
    write(STDOUT_FILENO,"\x1b[2J", 4) ;  
    // Escape sequence: \x1b[ ,  consists of two bytes ,  \x1b and [ .
    // Used to  instruct terminal to do stuff. 
    // It takes arguments.
    // 0J is defualt , if u just do <esc>]J , 0J is for clearing the screen starting from the cursor to the end.
    // 1J to clear the screen upto cursor. 
    // 2J to clear the entire screen. 
    // VT100 escape sequences will be used. // Supported by most of the terminals.
    
    write(STDOUT_FILENO,"\x1b[H" , 3 ) ; 
    // H is for positioning the cursor at the desired place. 
    // It take 2 args , i;jH , by default both i and j will be 1.  i -> Row number and j -> column number.
    // Rows and cols indexing is 1 based. 

    editorDrawRows() ; 
    write(STDOUT_FILENO,"\x1b[H",3) ; 
}
void editorDrawRows() { 
    int y ; 
    for(y=0;y<E.screenrows;y++) { 
        write(STDOUT_FILENO,"~",1) ; 
        if(y<E.screenrows-1) { 
            write(STDOUT_FILENO,"\r\n",2) ;
        }
    }
}

/*** init  ***/

void initEditor() { 
    if(getWindowSize(&E.screenrows,&E.screencols)==-1) die("getWindowSize") ; 
}

int main(void){
    enableRawMode() ;
    initEditor() ; 
    while(1) { 
        editorProcessKeypress(); 
    } ; 
    return 0 ; 
} 