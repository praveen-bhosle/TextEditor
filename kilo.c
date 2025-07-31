#include <unistd.h> 
#include <termios.h>
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1f )  

struct termios original_termios ;

void disableRawMode() ; 
void enableRawMode() ; 
void die(const char* ) ;
int main() ; 


void enableRawMode() { 
    if(tcgetattr(STDIN_FILENO,&original_termios)==-1) die("tcgetattr") ;
    atexit(disableRawMode) ;  
    struct termios raw = original_termios ; 
    raw.c_iflag &= ~(IXON) ; 
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | ICRNL )  ; // turning off echo and canonical mode.
    raw.c_oflag &= ~(OPOST) ; 
    raw.c_cc[VMIN]=0 ;  // mimimum bytes to be read before read returns.
    raw.c_cc[VTIME]=1;  // the time for which will read() will wait to return, 1 unit = 100ms. 
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)==-1) die("tcsetattr") ; 
}

void disableRawMode()  { 
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&original_termios) == -1 ) die("tcsetattr")  ;
}

void die(const char* s) { 
    perror(s) ; 
    exit(1) ; 
}

int main(void){ 
    printf("hi there how are you.\n"); 
    enableRawMode() ;
    while( 1) { 
        char c = '\0' ; 
        if(read(STDIN_FILENO , &c , 1 ) == -1 ) die("read") ; 
        if(iscntrl(c)) { 
            printf("%d\r\n",c) ; 
        }
        else { 
            printf("%d ('%c')\r\n", c , c ) ; 
        }
        if(c== CTRL_KEY('q')) break ;
    } ; 
    return 0 ; 
} 