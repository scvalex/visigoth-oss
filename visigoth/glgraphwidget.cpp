#include <QMouseEvent>
#include <QKeyEvent>
#include <cmath>
#include <QInputDialog>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "glgraphwidget.h"
#include "glancillary.h"        // gla*()

#include "edge.h"
#include "node.h"
#include "treecode.h"



/****************************
 * GraphWidget imitation code (public)
 ***************************/

GLGraphWidget::GLGraphWidget(QWidget *parent) :
    QGLWidget(parent),
    zoom(1.0),
    mouseMode(MOUSE_IDLE),
    helping(true),
    isPlaying(true),
    isRunning(false),
    timerId(0)
{
    setFocusPolicy(Qt::StrongFocus);

    myScene = new GraphScene(this);
    myScene->setBackgroundBrush(Qt::black);
    myScene->setItemIndexMethod(QGraphicsScene::NoIndex);
}

void GLGraphWidget::populate()
{
    myScene->repopulate();
    myScene->randomizePlacement();
}

void GLGraphWidget::itemMoved()
{
    isRunning = true;
    setAnimationRunning();
}



/****************************
 * GraphWidget imitation code (protected)
 ***************************/

void GLGraphWidget::setAnimationRunning()
{
    if (isPlaying && isRunning && !timerId)
        timerId = startTimer(1000 / 25);
    else if ((!isPlaying || !isRunning) && timerId) {
        killTimer(timerId);
        timerId = 0;
    }
}

void GLGraphWidget::playPause()
{
    isPlaying = !isPlaying;
    setAnimationRunning();

    this->repaint();
}

void GLGraphWidget::scaleView(qreal scaleFactor)
{
    zoom *= scaleFactor;
    this->initProjection();
}

void GLGraphWidget::fitToScreen()
{
    float aspectWidget = (float)width()/(float)height();
    float aspectGraph = (float)myScene->width()/(float)myScene->height();

    if (aspectGraph >= aspectWidget)
        scaleView((GLfloat)width() / (GLfloat)myScene->width() / zoom);
    else
        scaleView((GLfloat)height() / (GLfloat)myScene->height() / zoom);
}

void GLGraphWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, event->delta() / 240.0));

    this->repaint();
}

void GLGraphWidget::mousePressEvent(QMouseEvent *event)
{
    if (mouseMode != MOUSE_IDLE)
        return;

    if (event->button() == Qt::LeftButton)
    {
        switch(0)  // Should get modifier key status here
        {
            case 0:  // When no modifiers are pressed
                mouseMode = MOUSE_TRANSLATING;
                break;
            //case GLUT_ACTIVE_SHIFT:
                mouseMode = MOUSE_ROTATING;
                break;
            //case GLUT_ACTIVE_SHIFT | GLUT_ACTIVE_CTRL:
                mouseMode = MOUSE_TRANSLATING;
                break;
            //case GLUT_ACTIVE_CTRL:
                mouseMode = MOUSE_TRANSLATING2;
                break;
        }

        mouseX = event->x();
        mouseY = event->y();
    }
}

void GLGraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
    (void) event;

    mouseMode = MOUSE_IDLE;
}

void GLGraphWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx, dy;

    if (mouseMode == MOUSE_IDLE)
        return;

    dx = event->x() - mouseX;
    dy = event->y() - mouseY;
    mouseX = event->x();
    mouseY = event->y();

    switch(mouseMode)
    {
        case MOUSE_TRANSLATING:
            //glaCameraTranslatef(cameramat, (0.1) * dx, (-0.1) * dy, 0.0);
            // Modified for 2D projection and zoom
            glaCameraTranslatef(cameramat, (GLfloat)dx/zoom, (GLfloat)dy/zoom, 0.0);
            break;
        case MOUSE_TRANSLATING2:
            //glaCameraRotatef(cameramat, dx, 0.0, 1.0, 0.0);
            //glaCameraTranslatef(cameramat, 0.0, 0.0, (-0.1) * dy);
            break;
        case MOUSE_ROTATING:
            //glaCameraRotatef(cameramat, dx, 0.0, 1.0, 0.0);
            //glaCameraRotatef(cameramat, dy, 1.0, 0.0, 0.0);
            break;
        case MOUSE_DRAGGING:
        case MOUSE_IDLE:
            break;
    }

    this->repaint();
}

void GLGraphWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_H:
        helping = !helping;
        this->repaint();
        break;
    case Qt::Key_G:
        populate();
        break;
    case Qt::Key_Escape:
        helping = false;
        this->repaint();
        break;
    case Qt::Key_Equal:
    case Qt::Key_Plus:
        scaleView(1.2);
        break;
    case Qt::Key_Minus:
        scaleView(1.0/1.2);
        break;
    case Qt::Key_R:
        myScene->randomizePlacement();
        break;
    case Qt::Key_Space:
        playPause();
        break;
    case Qt::Key_0:
        fitToScreen();
        break;
    case Qt::Key_A:
        myScene->addVertex();
        break;
    case Qt::Key_I:
        getUserInput();
        break;
    case Qt::Key_N:
        myScene->nextAlgorithm();
        myScene->randomizePlacement();
        break;
    case Qt::Key_Left:
        glaCameraTranslatef(cameramat, (-20.0)/zoom, 0.0, 0.0);
        break;
    case Qt::Key_Up:
        glaCameraTranslatef(cameramat, 0.0, (-20.0)/zoom, 0.0);
        break;
    case Qt::Key_Right:
        glaCameraTranslatef(cameramat, 20.0/zoom, 0.0, 0.0);
        break;
    case Qt::Key_Down:
        glaCameraTranslatef(cameramat, 0.0, 20.0/zoom, 0.0);
        break;
    default:
        QGLWidget::keyPressEvent(event);
    }

    this->repaint();
}

void GLGraphWidget::timerEvent(QTimerEvent *)
{
    QPointF topLeft;
    QPointF bottomRight;

    TreeCode treeCode(myScene->sceneRect());

    foreach (Node* node, myScene->nodes()) {
        QPointF pos = node->calculatePosition();

        if (pos.x() < topLeft.x())
            topLeft.setX(pos.x());
        if (pos.y() < topLeft.y())
            topLeft.setY(pos.y());
        if (pos.x() > bottomRight.x())
            bottomRight.setX(pos.x());
        if (pos.y() > bottomRight.y())
            bottomRight.setY(pos.y());
    }

    // Resize the scene to fit all the nodes
    QRectF sceneRect = myScene->sceneRect();
    sceneRect.setLeft(topLeft.x() - 10);
    sceneRect.setTop(topLeft.y() - 10);
    sceneRect.setRight(bottomRight.x() + 10);
    sceneRect.setBottom(bottomRight.y() + 10);

    isRunning = false;
    foreach (Node *node, myScene->nodes()) {
        if (node->advance())
            isRunning = true;
    }
    setAnimationRunning();

    this->repaint();
}





/****************************
 * GL related QT event handlers (protected)
 ***************************/

void GLGraphWidget::initializeGL()
{
    glaInit();

    // Init camera matrix
    glMatrixMode(GL_MODELVIEW);
    // Warning: Do not set the camera far away when using small
    //     zNear, zFar values. Darkness awaits.
    //gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, cameramat);
    glLoadIdentity();
}

void GLGraphWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the camera
    glLoadMatrixf(cameramat);

    // Draw the old example objects
    glaDrawExample();

    // Draw the graph
    drawGraphGL();

    glFlush();
}

void GLGraphWidget::resizeGL(int w, int h)
{
    // Set up the Viewport transformation
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

    this->initProjection();
}



/****************************
 * GL graph drawing and projection setup (private)
 ***************************/

void GLGraphWidget::drawGraphGL()
{
    QPointF p;

    // Attention, references abound!
    QVector<Node*> &nodeVector = myScene->nodes();
    QList<Edge*> &edgeList= myScene->edges();

    // Draw edges
    glColor4f(0.0, 0.0, 1.0, 0.5);
    foreach (Edge* edge, edgeList)
    {
        glBegin(GL_LINE_STRIP);
            p = edge->sourceNode()->pos();
            glVertex3f((GLfloat)p.x(), (GLfloat)p.y(), 0.0);
            p = edge->destNode()->pos();
            glVertex3f((GLfloat)p.x(), (GLfloat)p.y(), 0.0);
        glEnd();
    }

    // Draw nodes
    glPointSize(5.0);
    glBegin(GL_POINTS);
        foreach (Node* node, nodeVector)
        {
            //glLoadName(i);    // Load point number into depth buffer for selection
            //glColor4f(n->color, 1.0, 0.3, 0.7);
            glColor4f(0.0, 1.0, 0.3, 0.7);
            p = node->pos();
            glVertex3f((GLfloat)p.x(), (GLfloat)p.y(), 0.0);
        }
    glEnd();
}

void GLGraphWidget::initProjection()
{
    // Set up the Projection transformation
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Zooming
    glScalef(zoom, zoom, 1.0/zoom);

    // Flat projection
    gluOrtho2D(0.0, (GLfloat)width(), (GLfloat)height(), 0.0);

    // Switch to Model/view transformation for drawing objects
    glMatrixMode(GL_MODELVIEW);
}

void GLGraphWidget::getUserInput() {
    bool ok;
    QString text = QInputDialog::getText(this,
            "", "Enter the Number of Nodes:", QLineEdit::Normal,
            QString::null, &ok, 0);
    if (ok && !text.isEmpty()) {
        int nodesNumber = text.toInt();
        myScene->modifyNodesNumber(nodesNumber);
        myScene->clear();
        Node::reset();
        populate();
    } else {
        //don't do anything
    }
}

