#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glut.h>

#include "Parser.h"

#define DEFAULT_WINDOW_SIZE_X 800
#define DEFAULT_WINDOW_SIZE_Y 600
#define DEFAULT_WINDOW_POS_X  100
#define DEFAULT_WINDOW_POS_Y   50

#define INPUT_FILE_NAME "tests.txt"

#define INITIAL_TREE_RECT_X1 -0.95
#define INITIAL_TREE_RECT_Y1  0.85
#define INITIAL_TREE_RECT_X2  0.95
#define INITIAL_TREE_RECT_Y2 -0.95

Parser p;
int window;
Tree* treeToDraw = 0;
bool fullscreenMode = false;
bool currentlyParseException = false;
bool showHelp = true;
ParseException parseException("", 0);
std::vector<std::string> tests;
size_t currentTestNumber = 0;
double currentTestNotePosDeltaX = 0;

double treeRectX1 = INITIAL_TREE_RECT_X1;
double treeRectY1 = INITIAL_TREE_RECT_Y1;
double treeRectX2 = INITIAL_TREE_RECT_X2;
double treeRectY2 = INITIAL_TREE_RECT_Y2;

void haltProgram() {
    glutDestroyWindow(window);
    exit(0);
}

double getTextLength(void* font, std::string const& text) {
    int res = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        res += glutBitmapWidth(font, text[i]);
    }
    return static_cast<double>(res);
}

void drawTextXY(double x, double y, void* font, std::string const& text) {
    glRasterPos2d(x, y);
    for (size_t i = 0; i < text.length(); ++i) {
        glutBitmapCharacter(font, text[i]);
    }
}

void drawTextXYEvenIfItDoesNotStartOnWindow(double x, double y, void* font, std::string const& text) {
    for (size_t i = 0; i < text.length(); ++i) {
        glRasterPos2d(x, y);
        glutBitmapCharacter(font, text[i]);
        double charW = static_cast<double>(glutBitmapWidth(font, text[i]));
        x += charW * 2 / glutGet(GLUT_WINDOW_WIDTH);
    }
}

void drawTree(Tree* t, double rX1, double rY1, double rX2, double rY2, double cellH) {
    int h = t->getHeight();
    
    //drawing children
    size_t childrenNum = t->getChildren().size();
    double subCellW = (rX2 - rX1) / childrenNum;
    for (size_t i = 0; i < childrenNum; ++i) {
        //drawing edges to children
        glColor3d(1, 0, 0);
        glBegin(GL_LINES);
        glVertex2d((rX1 + rX2) / 2, rY1 + cellH / 2);
        glVertex2d(rX1 + subCellW * (i + 0.5), rY1 + cellH * 1.5);
        glEnd();
        
        drawTree(t->getChildren()[i], rX1 + subCellW * i, rY1 + cellH, rX1 + subCellW * (i + 1), rY2, cellH);
    }

    int windowW = glutGet(GLUT_WINDOW_WIDTH);
    int windowH = glutGet(GLUT_WINDOW_HEIGHT);
    double textLength = getTextLength(GLUT_BITMAP_HELVETICA_12, t->getNode());
    double halfL = (textLength + 8.0) / windowW;
    double halfH = 16.0 / windowH;
    double textXDelta = textLength / windowW;
    double textYDelta = 10.0 / windowH;

    //drawing self
    glColor3d(0, 0, 0);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2d((rX1 + rX2) / 2 - halfL, rY1 + cellH / 2 - halfH);
    glVertex2d((rX1 + rX2) / 2 + halfL, rY1 + cellH / 2 - halfH);
    glVertex2d((rX1 + rX2) / 2 - halfL, rY1 + cellH / 2 + halfH);
    glVertex2d((rX1 + rX2) / 2 + halfL, rY1 + cellH / 2 + halfH);
    glEnd();

    glColor3d(0, 1, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2d((rX1 + rX2) / 2 - halfL, rY1 + cellH / 2 - halfH);
    glVertex2d((rX1 + rX2) / 2 + halfL, rY1 + cellH / 2 - halfH);
    glVertex2d((rX1 + rX2) / 2 + halfL, rY1 + cellH / 2 + halfH);
    glVertex2d((rX1 + rX2) / 2 - halfL, rY1 + cellH / 2 + halfH);
    glVertex2d((rX1 + rX2) / 2 - halfL, rY1 + cellH / 2 - halfH);
    glEnd();
    
    glColor3d(1, 1, 1);
    drawTextXY((rX1 + rX2) / 2 - textXDelta, rY1 + cellH / 2 - textYDelta, GLUT_BITMAP_HELVETICA_12, t->getNode());
}

void displayFunc() {
    glClear(GL_COLOR_BUFFER_BIT);

    double textDelta = 26.0 / glutGet(GLUT_WINDOW_HEIGHT);

    if (tests.empty()) {
        glColor3d(1, 1, 0);
        std::ostringstream oss;
        oss << "No tests found in '" << INPUT_FILE_NAME << "'";
        drawTextXY(-0.15, 0, GLUT_BITMAP_HELVETICA_12, oss.str());
        glColor3d(1, 1, 1);
        drawTextXY(-0.15, -textDelta, GLUT_BITMAP_HELVETICA_12, "Press Esc or q to quit");
        glFlush();
        return;
    }
    
    if (treeToDraw) {
        drawTree(treeToDraw, treeRectX1, treeRectY1, treeRectX2, treeRectY2, (treeRectY2 - treeRectY1) / treeToDraw->getHeight());

        double crossDX = 15.0 / glutGet(GLUT_WINDOW_WIDTH);
        double crossDY = 15.0 / glutGet(GLUT_WINDOW_HEIGHT);
        glColor4d(0, 0, 1, 0.5);
        glBegin(GL_LINES);
        glVertex2d(-crossDX, 0);
        glVertex2d(crossDX, 0);
        glVertex2d(0, -crossDY);
        glVertex2d(0, crossDY);
        glEnd();
    }

    if (currentlyParseException) {
        glColor3d(1, 1, 0);
        drawTextXY(-0.15, 0, GLUT_BITMAP_HELVETICA_12, "ParseException:");
        glColor3d(1, 1, 1);
        std::ostringstream oss;
        oss << parseException.getMessage() << " " << parseException.getErrorOffset();
        drawTextXY(-0.15, -textDelta, GLUT_BITMAP_HELVETICA_12, oss.str());
    }

    glColor3d(1, 1, 0);
    std::ostringstream oss;
    oss << "Current test [" << currentTestNumber + 1 << "/" << tests.size() << "]: " << tests[currentTestNumber];
    drawTextXYEvenIfItDoesNotStartOnWindow(-0.90 + currentTestNotePosDeltaX, 0.90, GLUT_BITMAP_HELVETICA_12, oss.str());

    if (showHelp) {
        glColor3d(1, 1, 1);
        drawTextXY(-0.90, 0.90 - textDelta * 2,  GLUT_BITMAP_HELVETICA_12, "Esc or q - quit");
        drawTextXY(-0.90, 0.90 - textDelta * 3,  GLUT_BITMAP_HELVETICA_12, "f - toggle fullscreen");
        drawTextXY(-0.90, 0.90 - textDelta * 4,  GLUT_BITMAP_HELVETICA_12, "F2 or PageDown - next test");
        drawTextXY(-0.90, 0.90 - textDelta * 5,  GLUT_BITMAP_HELVETICA_12, "F1 or PageUp - previous test");
        drawTextXY(-0.90, 0.90 - textDelta * 6,  GLUT_BITMAP_HELVETICA_12, "x/X - zoom in/out tree by X axis");
        drawTextXY(-0.90, 0.90 - textDelta * 7,  GLUT_BITMAP_HELVETICA_12, "y/Y - zoom in/out tree by Y axis");
        drawTextXY(-0.90, 0.90 - textDelta * 8,  GLUT_BITMAP_HELVETICA_12, "+/- - zoom in/out tree by both axes");
        drawTextXY(-0.90, 0.90 - textDelta * 9,  GLUT_BITMAP_HELVETICA_12, "Arrows - move tree");
        drawTextXY(-0.90, 0.90 - textDelta * 10, GLUT_BITMAP_HELVETICA_12, "h/H - show/hide this help");
    }
    
    glutSwapBuffers();
}

void readTests() {
    std::ifstream inputFile(INPUT_FILE_NAME);
    if (!inputFile.fail()) {
        while (!inputFile.eof()) {
            std::string s;
            getline(inputFile, s);
            if (s != "")
                tests.push_back(s);
        }
    }
    inputFile.close();
}

void rebuildTree() {
    if (tests.empty())
        return;

    if (treeToDraw)
        delete treeToDraw;
    try {
        treeToDraw = p.parse(tests[currentTestNumber]);
        currentlyParseException = false;
    } catch (ParseException const& pe) {
        treeToDraw = 0;
        currentlyParseException = true;
        parseException = pe;
    }
}

void resetTreeRect() {
    treeRectX1 = INITIAL_TREE_RECT_X1;
    treeRectY1 = INITIAL_TREE_RECT_Y1;
    treeRectX2 = INITIAL_TREE_RECT_X2;
    treeRectY2 = INITIAL_TREE_RECT_Y2;
}

void keyboardFunc(unsigned char key, int x, int y) {
    switch (key) {
        case 27:
        case 'q':
        case 'Q':
            haltProgram();
        case 'f':
        case 'F':
            fullscreenMode = !fullscreenMode;
            if (fullscreenMode) {
                glutFullScreen();
            } else {
                glutReshapeWindow(DEFAULT_WINDOW_SIZE_X, DEFAULT_WINDOW_SIZE_Y);
                glutPositionWindow(DEFAULT_WINDOW_POS_X, DEFAULT_WINDOW_POS_Y);
            }
            glutPostRedisplay();
            break;
        case ' ':
            resetTreeRect();
            glutPostRedisplay();
            break;
        case 'x':
            treeRectX1 *= 1.1;
            treeRectX2 *= 1.1;
            glutPostRedisplay();
            break;
        case 'X':
            treeRectX1 /= 1.1;
            treeRectX2 /= 1.1;
            glutPostRedisplay();
            break;
        case 'y':
            treeRectY1 *= 1.1;
            treeRectY2 *= 1.1;
            glutPostRedisplay();
            break;
        case 'Y':
            treeRectY1 /= 1.1;
            treeRectY2 /= 1.1;
            glutPostRedisplay();
            break;
        case '+':
            treeRectX1 *= 1.1;
            treeRectX2 *= 1.1;
            treeRectY1 *= 1.1;
            treeRectY2 *= 1.1;
            glutPostRedisplay();
            break;
        case '-':
            treeRectX1 /= 1.1;
            treeRectX2 /= 1.1;
            treeRectY1 /= 1.1;
            treeRectY2 /= 1.1;
            glutPostRedisplay();
            break;
        case 'H':
            if (showHelp) {
                showHelp = false;
                glutPostRedisplay();
            }
            break;
        case 'h':
            if (!showHelp) {
                showHelp = true;
                glutPostRedisplay();
            }
            break;
    }
}

void specialFunc(int key, int x, int y) {
    double dx = 30.0 / glutGet(GLUT_WINDOW_WIDTH);
    double dy = 30.0 / glutGet(GLUT_WINDOW_HEIGHT);
    switch (key) {
        case GLUT_KEY_PAGE_UP:
        case GLUT_KEY_F1:
            if (currentTestNumber > 0) {
                --currentTestNumber;
                rebuildTree();
                resetTreeRect();
                currentTestNotePosDeltaX = 0;
                glutPostRedisplay();
            }
            break;
        case GLUT_KEY_PAGE_DOWN:
        case GLUT_KEY_F2:
            if (currentTestNumber < tests.size() - 1) {
                ++currentTestNumber;
                rebuildTree();
                resetTreeRect();
                currentTestNotePosDeltaX = 0;
                glutPostRedisplay();
            }
            break;
        case GLUT_KEY_LEFT:
            treeRectX1 -= dx;
            treeRectX2 -= dx;
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            treeRectX1 += dx;
            treeRectX2 += dx;
            glutPostRedisplay();
            break;
        case GLUT_KEY_UP:
            treeRectY1 += dy;
            treeRectY2 += dy;
            glutPostRedisplay();
            break;
        case GLUT_KEY_DOWN:
            treeRectY1 -= dy;
            treeRectY2 -= dy;
            glutPostRedisplay();
            break;
        case GLUT_KEY_F3:
            currentTestNotePosDeltaX -= 2 * dx;
            glutPostRedisplay();
            break;
        case GLUT_KEY_F4:
            currentTestNotePosDeltaX += 2 * dx;
            glutPostRedisplay();
            break;
    }
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(DEFAULT_WINDOW_SIZE_X, DEFAULT_WINDOW_SIZE_Y);
    glutInitWindowPosition(DEFAULT_WINDOW_POS_X, DEFAULT_WINDOW_POS_Y);
    window = glutCreateWindow("Parse Tree Visualizer");
    
    glutDisplayFunc(displayFunc);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialFunc);
    
    glClearColor(0, 0, 0, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glLineWidth(1.0);

    readTests();
    currentTestNumber = 0;
    rebuildTree();

    glutMainLoop();

    return 0;
}
