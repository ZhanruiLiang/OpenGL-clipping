#include<iostream>
#include<string>
#include<cassert>
#include<cmath>
#include<cstdio>
#include<cstring>
#include<vector>
#include<tuple>
#include <pthread.h>
#include<GL/freeglut.h>

#define For(i, n) for(i = 0; i < n; i++)
#pragma GCC diagnostic ignored "-Wsign-compare"

#ifdef DEBUG
#include "debug.hpp"
#endif

typedef long long LL;
using namespace std;

typedef GLfloat Color[3];
typedef tuple<int, int> Point;

Color BG_COLOR = {1., 1., 1.};
Color POINT_COLOR = {.8, .2, .2};
Color FONT_COLOR = {.2, .2, .2};
Color POLY_COLOR = {.2, .8, .2};
Color CLIPPED_POLY_COLOR = {.9, .2, .4};
Color RECT_COLOR = {.2, .2, .6};

const double PI = acos(-1);
const double EPS = 1e-10;
int gWinW, gWinH;
int gWinID;
vector<Point> poly, clippedPoly, clipRect;
Point rectP1, rectP2;
Point currentP;

pthread_mutex_t gLock;

const int ESCKEY = 27; // ASCII value of escape character

const int WAIT_POLYGON = 0;
const int CREATE_POLYGON = 1;
const int WAIT_RECT = 2;
const int CREATE_RECT = 3;
const int END = 4;

/*
 *  0: Waiting Polygon           ____                                         
 *   | ===============          |    \ Left Mouse Click                                   
 *   | Left Mouse Click         v    |                                     
 *   \_____>    1: Create Polygon ---/                                      
 *                 ==============|
 *            Right Mouse Click  |                                         
 *                               |                                         
 *  2: Waiting Rectangle  <------/
 *   | =================          ______                                         
 *   | Left Mouse Down           |      \ Mouse Motion                                       
 *   |                           v      |  
 *    \____>    3: Create Rectangle ----/                                      
 *                 ================                                        
 *                Left Mouse Up  |
 *                               |
 *  4: END      <----------------/                                            
 *     ===                                                                 
 */

int gState = WAIT_POLYGON;
// int gState = WAIT_RECT;

int sgn(double x){
    return (x > EPS) - (x < - EPS);
}

Point operator-(const Point& p1, const Point& p2){
    return make_tuple(get<0>(p1) - get<0>(p2), get<1>(p1) - get<1>(p2));
}

bool solve_cross_point(
        const Point& p1, const Point& p2,
        const Point& ep1, const Point& ep2,
        Point& cp){
    double a11, a12, a21, a22, b1, b2, d, t;
    tie(a11, a21) = p2 - p1;
    tie(a12, a22) = ep1 - ep2;
    tie(b1, b2) = ep1 - p1;
    d = a11 * a22 - a21 * a12;
    if(sgn(d) == 0){
        return false;
    }
    t = (b1 * a22 - b2 * a12) / d;
    assert(sgn(t) >= 0 and sgn(t - 1) <= 0);
    cp = make_tuple(get<0>(p1) + a11 * t, get<1>(p1) + a21 * t);
    return true;
}

int side(const Point& p, const Point& ep1, const Point& ep2){
    // sgn((ep2 - ep1) x (p1 - ep1))
    double dx, dy, dex, dey;
    tie(dex, dey) = ep2 - ep1;
    tie(dx, dy) = p - ep1;
    return sgn(dex * dy - dey * dx);
}

vector<Point> make_clip_rect(const Point& p1, const Point& p2){
    vector<Point> clipRect;
    clipRect.push_back(p1);
    clipRect.push_back(make_tuple(get<0>(p2), get<1>(p1)));
    clipRect.push_back(p2);
    clipRect.push_back(make_tuple(get<0>(p1), get<1>(p2)));
    return clipRect;
}

vector<Point> clip(const vector<Point> &poly, const vector<Point>& clipPoly){
    /* clipPoly should be convex and CCW. poly can be concave.
     */
    int iClip, iInput, n;
    vector<Point> inputs, outputs;
    Point p1, p2, ep1, ep2, cp;
    int side1, side2;
    n = clipPoly.size();
    outputs = poly;
    For(iClip, n){
        ep1 = clipPoly[(iClip + n - 1) % n];
        ep2 = clipPoly[iClip];
        inputs = outputs;
        outputs.clear();
        p1 = inputs.back();
        side1 = side(p1, ep1, ep2);
        For(iInput, inputs.size()){
            p2 = inputs[iInput];
            side2 = side(p2, ep1, ep2);
            if(side2 >= 0){
                if(side1 < 0){
                    solve_cross_point(p1, p2, ep1, ep2, cp);
                    outputs.push_back(cp);
                }
                outputs.push_back(p2);
            }else if(side1 >= 0){
                solve_cross_point(p1, p2, ep1, ep2, cp);
                outputs.push_back(cp);
            }
            p1 = p2;
            side1 = side2;
        }
    }
    return outputs;
}

void handle_mouse(int button, int state, int x, int y){
    // pthread_mutex_lock(&gLock);
    if(gState == WAIT_POLYGON){
        if(button == GLUT_LEFT_BUTTON and state == GLUT_DOWN){
            gState = CREATE_POLYGON;
            currentP = make_tuple(x, y);
            poly.push_back(make_tuple(x, y));
        }
    }else if(gState == CREATE_POLYGON){
        if(button == GLUT_LEFT_BUTTON and state == GLUT_DOWN){
            poly.push_back(make_tuple(x, y));
        }else if(button == GLUT_RIGHT_BUTTON and state == GLUT_DOWN){
            gState = WAIT_RECT;
        }
    }else if(gState == WAIT_RECT){
        if(button == GLUT_LEFT_BUTTON and state == GLUT_DOWN){
            gState = CREATE_RECT;
            rectP1 = make_tuple(x, y);
        }
    }else if(gState == CREATE_RECT){
        if(button == GLUT_LEFT_BUTTON and state == GLUT_UP){
            rectP2 = make_tuple(x, y);
            if(rectP1 != rectP2){
                clipRect = make_clip_rect(rectP1, rectP2);
                clippedPoly = clip(poly, clipRect);
            }
            gState = END;
        }
    }
    glutPostRedisplay();
    // pthread_mutex_unlock(&gLock);
}

void handle_mouse_move(int x, int y){
    // pthread_mutex_lock(&gLock);
    currentP = make_tuple(x, y);
    if(gState == CREATE_POLYGON or gState == CREATE_RECT){
        if(rectP1 != currentP){
            clipRect = make_clip_rect(rectP1, currentP);
            clippedPoly = clip(poly, clipRect);
        }
        glutPostRedisplay();
    }
    // pthread_mutex_unlock(&gLock);
}

void reshape(int w, int h){
    gWinW = w;
    gWinH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, gWinW, 0, gWinH);
    // convert screen coordinate to gl coordinate
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0., gWinH, 0.);
    glScalef(1., -1., 1.);
#ifdef DEBUG
    // print_mat();
#endif
}

void draw_poly(const vector<Point>& poly, bool close=true){
    int x, y;
    int i;
    glBegin(close ? GL_LINE_LOOP : GL_LINE_STRIP);{
        For(i, poly.size()){
            tie(x, y) = poly[i];
            glVertex2i(x, y);
        }
    }glEnd();
    glBegin(GL_POINTS);{
        glColor3fv(POINT_COLOR);
        For(i, poly.size()){
            tie(x, y) = poly[i];
            glVertex2i(x, y);
        }
    }glEnd();
}

void display(){
    int x, y;
    int i;
    // pthread_mutex_lock(&gLock);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3fv(FONT_COLOR);
    printw(20, 20, 0, "state: %d", gState);
    printw(20, 40, 0, "currentP: %d, %d", get<0>(currentP), get<1>(currentP));
    printw(20, 60, 0, "rectP1: %d, %d", get<0>(rectP1), get<1>(rectP1));
    printw(20, 80, 0, "rectP2: %d, %d", get<0>(rectP2), get<1>(rectP2));
    if(gState == CREATE_POLYGON or gState == WAIT_RECT){
        // display the created point, and current point
        if(gState == CREATE_POLYGON){
            poly.push_back(currentP);
        }
        glColor3fv(POLY_COLOR);
        draw_poly(poly, (gState == WAIT_RECT));
        if(gState == CREATE_POLYGON){
            poly.pop_back();
        }
    }
    if(gState >= CREATE_RECT){
        glColor3fv(POLY_COLOR);
        draw_poly(poly);

        glColor3fv(RECT_COLOR);
        draw_poly(clipRect);

        glLineWidth(3);
        glColor3fv(CLIPPED_POLY_COLOR);
        draw_poly(clippedPoly);
        glLineWidth(1);
    }
    glutSwapBuffers();
    // pthread_mutex_unlock(&gLock);
}

void init(){
    glClearColor(BG_COLOR[0], BG_COLOR[1], BG_COLOR[2], 1.);
    // glEnable(GL_LINE_SMOOTH);
    glPointSize(6);
    pthread_mutex_init(&gLock, NULL);
}
void handle_key(unsigned char key, int x, int y){
    pthread_mutex_lock(&gLock);
    if(key == ESCKEY){
        pthread_mutex_unlock(&gLock);
        glutLeaveMainLoop();
    }
    pthread_mutex_unlock(&gLock);
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    gWinW = 640;
    gWinH = 360;
    glutInitWindowSize(gWinW, gWinH);
    glutInitWindowPosition(20, 20);
    /*
     * create window
     */
    int gWinID = glutCreateWindow("GL - Assignment 4");
    // bind callbacks
    glutKeyboardFunc(handle_key);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(handle_mouse);
    glutMotionFunc(handle_mouse_move);
    glutPassiveMotionFunc(handle_mouse_move);
    // init and start
    init();
    glutMainLoop();
}
