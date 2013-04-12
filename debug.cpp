#include "debug.hpp"
#include <cstdio>
#include <stdarg.h>

void print_mat(){
    GLdouble mat[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, mat);
    int i, j;
    printf("============\n");
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            printf("\t%.3f ", mat[j * 4 + i]);
        }
        printf("\n");
    }
}

int _vscprintf (const char * format, va_list pargs) { 
    int retval; 
    va_list argcopy; 
    va_copy(argcopy, pargs); 
    retval = vsnprintf(NULL, 0, format, pargs); 
    va_end(argcopy); 
    return retval; 
}

//-------------------------------------------------------------------------
//  Draws a string at the specified coordinates.
//-------------------------------------------------------------------------
void printw(float x, float y, float z, const char* format, ...) {
    va_list args;	//  Variable argument list
    int len;		//	String length
    int i;			//  Iterator
    char * text;	//	Text

    //  Initialize a variable argument list
    va_start(args, format);

    //  Return the number of characters in the string referenced the list of arguments.
    //  _vscprintf doesn't count terminating '\0' (that's why +1)
    len = _vscprintf(format, args) + 1; 

    //  Allocate memory for a string of the specified size
    text = new char[len];

    //  Write formatted output using a pointer to the list of arguments
    vsnprintf(text, len, format, args);

    //  End using variable argument list 
    va_end(args);

    //  Specify the raster position for pixel operations.
    glRasterPos3f (x, y, z);

    //  Draw the characters one by one
    for (i = 0; text[i] != '\0'; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);

    //  Free the allocated memory for the string
    delete text;
}
