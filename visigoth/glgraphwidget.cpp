#include <QMouseEvent>
#include <QKeyEvent>
#include <cmath>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "vtools.h"
#include "glgraphwidget.h"
#include "glancillary.h"        // gla*()
#include "edge.h"
#include "node.h"
#include "quadtree.h"


/****************************
 * GraphWidget imitation code (public)
 ***************************/

GLGraphWidget::GLGraphWidget(QWidget *parent) :
    QGLWidget(parent),
    myScene(0),
    mouseMode(MOUSE_IDLE),
    animTimerId(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    set3DMode(false);
}

void GLGraphWidget::setScene(GraphScene *newScene) {
    if (myScene != 0)
        return;
    myScene = newScene;
    connect(myScene, SIGNAL(algorithmChanged(Algorithm*)), this, SIGNAL(algorithmChanged(Algorithm*)));
    connect(myScene, SIGNAL(nodeMoved()), this, SLOT(onNodeMoved()));
    setAnimation(true);
    myScene->set3DMode(mode3d);
}

void GLGraphWidget::set3DMode(bool enabled) {
    mode3d = enabled;
    zoom = 1.0;
    initializeCamera();
    setupLighting();

    if (myScene)
        myScene->set3DMode(mode3d);
}

GLGraphWidget::~GLGraphWidget() {
    delete myScene;
}

bool GLGraphWidget::animationRunning() {
    return (animTimerId != 0);
}

void GLGraphWidget::setAnimation(bool enable) {
    if (enable && !animTimerId) {
        animTimerId = startTimer(1000 / 25);
    } else if (!enable && animTimerId) {
        killTimer(animTimerId);
        animTimerId = 0;
    }
}

void GLGraphWidget::onNodeMoved() {
    setAnimation(true);
}

void GLGraphWidget::scaleView(qreal scaleFactor) {
    zoom *= scaleFactor;
}

void GLGraphWidget::fitToScreen() {
    // TODO: Possibly reposition so the graph is inside the window. 3D?
    if (!mode3d) {
        zoom = (qreal)height() / myScene->graphCube().longestEdge();
    }
}

void GLGraphWidget::wheelEvent(QWheelEvent *event) {
    scaleView(pow((double)2, event->delta() / 240.0));

    this->repaint();
}

void GLGraphWidget::resetHighlighting() {
    foreach (Node *node, myScene->nodes()) {
        node->setHighlight(false);
    }
    foreach (Edge *edge, myScene->edges()) {
        edge->setHighlight(false);
    }
    repaint();
}

void GLGraphWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        Node *hitNode = selectGL(event->x(), event->y());
        if (!hitNode)
            return;

        hitNode->setHighlight(!hitNode->highlighted());
        foreach (Node* node, hitNode->neighbours()) {
            foreach (Edge* edge, hitNode->edges()) {
                if ((edge->sourceNode() == hitNode && edge->destNode() == node) ||
                    (edge->sourceNode() == node && edge->destNode() == hitNode))
                {
                    edge->setHighlight(hitNode->highlighted());
                }
            }
        }

        this->repaint();
    }
}

void GLGraphWidget::mousePressEvent(QMouseEvent *event) {
    if (mouseMode != MOUSE_IDLE)
        return;

    if (event->button() == Qt::LeftButton) {
        switch (event->modifiers()) {
            case 0:  // When no modifiers are pressed
            {
                Node *hitNode = selectGL(event->x(), event->y());
                if (hitNode) {
                    // Calculate stats
                    emit onSelectNode(hitNode);

                    // Set up dragging
                    draggedNode = hitNode;
                    draggedNode->setAllowAdvance(false);
                    mouseMode = MOUSE_DRAGGING;
                } else {
                    if (mode3d)
                        mouseMode = MOUSE_TRANSLATING_XY;
                    else
                        mouseMode = MOUSE_TRANSLATING_2D;
                }
                break;
            }
            case Qt::ShiftModifier:
                if (mode3d)
                    mouseMode = MOUSE_ROTATING;
                break;
            case Qt::ControlModifier:
                if (mode3d)
                    mouseMode = MOUSE_TRANSLATING_Z;
                break;
        }
    }

    mouseX = event->x();
    mouseY = event->y();
}

void GLGraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    (void) event;

    if (mouseMode == MOUSE_DRAGGING && draggedNode)
      draggedNode->setAllowAdvance(true);

    mouseMode = MOUSE_IDLE;
}

void GLGraphWidget::mouseMoveEvent(QMouseEvent *event) {
    int dx, dy;

    if (mouseMode == MOUSE_IDLE) {
        return;
    }

    dx = event->x() - mouseX;
    dy = event->y() - mouseY;
    mouseX = event->x();
    mouseY = event->y();

    switch(mouseMode) {
        case MOUSE_TRANSLATING_2D:
            glaCameraTranslatef(cameramat, (GLfloat)dx/zoom, (-1.0)*(GLfloat)dy/zoom, 0.0);
            break;
        case MOUSE_TRANSLATING_XY:
            glaCameraTranslatef(cameramat, (3.0) * dx, (-3.0) * dy, 0.0);
            break;
        case MOUSE_TRANSLATING_Z:
            glaCameraRotatef(cameramat, dx, 0.0, 1.0, 0.0);
            glaCameraTranslatef(cameramat, 0.0, 0.0, (-3.0) * dy);
            break;
        case MOUSE_ROTATING:
            glaCameraRotatef(cameramat, dx, 0.0, 1.0, 0.0);
            glaCameraRotatef(cameramat, dy, 1.0, 0.0, 0.0);
            break;
        case MOUSE_DRAGGING:
        {
            GLdouble newX, newY, newZ;
            GLdouble model[16], proj[16];
            VPointF p = draggedNode->pos();

            // Get matrices  as double instead of single precision floats
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
              glLoadMatrixf(cameramat);
              glGetDoublev(GL_MODELVIEW_MATRIX, model);
              glLoadMatrixf(projmat);
              glGetDoublev(GL_MODELVIEW_MATRIX, proj);
            glPopMatrix();

            gluProject(p.x, p.y, p.z, model, proj, viewmat,
                        &newX, &newY, &newZ);

            newX = (GLdouble)mouseX;
            newY = (GLdouble)(viewmat[3] - mouseY);

            gluUnProject(newX, newY, newZ,
                        model, proj, viewmat,
                        &newX, &newY, &newZ);

            draggedNode->setPos(VPointF(newX, newY, newZ));
            break;
        }
        default:
            break;
    }

    this->repaint();
}

void GLGraphWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_G:
        myScene->repopulate();
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
        setAnimation(!animationRunning());
        break;
    case Qt::Key_0:
        fitToScreen();
        break;
    case Qt::Key_A:
    case Qt::Key_N:
        myScene->addVertex();
        break;
    case Qt::Key_Right:
        glaCameraTranslatef(cameramat, (-20.0)/zoom, 0.0, 0.0);
        break;
    case Qt::Key_Down:
        glaCameraTranslatef(cameramat, 0.0, 20.0/zoom, 0.0);
        break;
    case Qt::Key_Left:
        glaCameraTranslatef(cameramat, 20.0/zoom, 0.0, 0.0);
        break;
    case Qt::Key_Up:
        glaCameraTranslatef(cameramat, 0.0, (-20.0)/zoom, 0.0);
        break;
    default:
        QGLWidget::keyPressEvent(event);
    }

    this->repaint();
}

void GLGraphWidget::timerEvent(QTimerEvent *) {
    bool somethingMoved = myScene->calculateForces();

    if (!somethingMoved) {
        // setAnimation(true) would recreate the timer though it is
        // already running (this is a timer event). So don't do it.
        setAnimation(false);
    }

    this->repaint();
}





/****************************
 * GL related QT event handlers (protected)
 ***************************/

void GLGraphWidget::initializeGL() {
    glaInit();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initializeCamera();
}

void GLGraphWidget::initializeCamera() {
    glPushMatrix();
        glLoadIdentity();
        if (mode3d) {
            // Warning: Do not set the camera far away when using small
            //     zNear, zFar values. Darkness awaits.
            gluLookAt(0.0, 0.0, 500.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
        }
        glGetFloatv(GL_MODELVIEW_MATRIX, cameramat);
    glPopMatrix();
}

void GLGraphWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw 2D background elements
    initOverlayProjection();
    drawBackground();

    // Draw the graph and the central box
    initGraphProjection();
    glaDrawExample();
    drawGraphGL();

    // Draw 2D overlay elements
    initOverlayProjection();
    drawOverlay();

    glFlush();
}

void GLGraphWidget::resizeGL(int w, int h) {
    // Set up the Viewport transformation
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

    // Save the viewport, e.g. for mouse interaction
    glGetIntegerv(GL_VIEWPORT, viewmat);
}



/****************************
 * GL graph drawing and projection setup (private)
 ***************************/

/*** 2D overlay/background drawing ***/

void GLGraphWidget::drawBackground() {
    QColor c = myScene->backgroundColour();
    glClearColor(c.redF(), c.greenF(), c.blueF(), 1.0);
}

void GLGraphWidget::drawOverlay() {
}


/*** Graph drawing ***/

inline void GLGraphWidget::drawSphere(GLfloat r, int lats, int longs) {
    for (int i = 0; i <= lats; ++i) {
        GLfloat lat0 = M_PI * (-0.5 + (GLfloat) (i - 1) / lats);
        GLfloat z0  = sin(lat0);
        GLfloat zr0 =  cos(lat0);

        GLfloat lat1 = M_PI * (-0.5 + (GLfloat) i / lats);
        GLfloat z1 = sin(lat1);
        GLfloat zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
            for(int j = 0; j <= longs; ++j) {
                GLfloat lng = 2 * M_PI * (GLfloat) (j - 1) / longs;
                GLfloat x = cos(lng);
                GLfloat y = sin(lng);

                glNormal3f(x * zr0, y * zr0, z0);
                glVertex3f(x * zr0 * r, y * zr0 * r, z0 * r);
                glNormal3f(x * zr1, y * zr1, z1);
                glVertex3f(x * zr1 * r, y * zr1 * r, z1 * r);
            }
        glEnd();
    }
}

inline void GLGraphWidget::drawCircle(GLfloat r, int longs) {
    glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < longs; ++i) {
            GLfloat lng = 2 * M_PI * (GLfloat) (i - 1) / longs;
            glVertex2f(sin(lng) * r, cos(lng) * r);
        }
    glEnd();
}

inline void GLGraphWidget::drawNode(Node *node) {
    const QColor c = node->highlighted() ? Qt::red : node->colour();
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());

    float radius = (log(node->edges().size()) / log(2)) + 1.0;
    VPointF p = node->pos();

    glPushMatrix();
        glTranslatef(p.x, p.y, p.z);

        if (mode3d) {
            drawSphere(radius, 10, 10);
        } else {
            drawCircle(radius, 10);
        }
    glPopMatrix();
}

void GLGraphWidget::drawGraphGL() {
    // Draw edges
    foreach (Edge* edge, myScene->edges()) {
        const QColor c = edge->highlighted() ? Qt::yellow : edge->colour();
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());

        glBegin(GL_LINE_STRIP);
            VPointF p = edge->sourceNode()->pos();
            glVertex3f((GLfloat)p.x, (GLfloat)p.y, (GLfloat)p.z);
            p = edge->destNode()->pos();
            glVertex3f((GLfloat)p.x, (GLfloat)p.y, (GLfloat)p.z);
        glEnd();
    }

    // Draw nodes
    foreach (Node* node, myScene->nodes()) {
        drawNode(node);
    }
}


/*** Projection setup ***/

void GLGraphWidget::initGraphProjection() {
    // Set up the Projection transformation
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Zooming, only in 2D
    if (!mode3d)
        glScalef(zoom, zoom, 1.0/zoom);

    if (mode3d)
        // Persective projection
        gluPerspective(90, (GLfloat)width()/(GLfloat)height(), 0.0001, 100000.0);
    else
        // Flat projection
        glOrtho((GLfloat)width() / -2, (GLfloat)width() / 2,
                (GLfloat)height() / -2, (GLfloat)height() / 2,
                -100, 100);

    // Save the projection matrix for later use, e.g. mouse interaction
    glGetFloatv(GL_PROJECTION_MATRIX, projmat);

    // Switch to Model/view transformation for drawing objects
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(cameramat);   // Load camera for 3D graph

}

void GLGraphWidget::initOverlayProjection() {
    // Set up the Projection transformation
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Flat projection, Qt-style y axis.
    gluOrtho2D(0, (GLfloat)width(), (GLfloat)height(), 0);

    // Switch to Model/view transformation for drawing objects
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();   // Reset camera for 2D overlay
}


/* Lighting */

void GLGraphWidget::setupLighting() {
    if (mode3d) {
        // Lightining
        GLfloat sunDirection[] = {0.0, 2.0, -1.0, 1.0};
        GLfloat sunIntensity[] = {0.7, 0.7, 0.7, 1.0};
        GLfloat ambientIntensity[] = {0.3, 0.3, 0.3, 1.0};

        // Set up ambient light
        glEnable(GL_LIGHTING);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientIntensity);

        // Set up sun light
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, sunDirection);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, sunIntensity);
    } else {
        glDisable(GL_LIGHTING);
    }
}


/*** Selection ***/

Node* GLGraphWidget::selectGL(int x, int y)
{
    Node* hitNode = NULL;
    GLuint namebuf[64] = {0};
    GLint hits;

    glSelectBuffer(64, namebuf);

    // Account for inverse Y coordinate
    y = viewmat[3] - y;

    initGraphProjection();

    // Restrict projection matrix to selection area
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPickMatrix(x, y, 1.0, 1.0, viewmat);
    glMultMatrixf(projmat);

    // Redraw points to fill selection buffer
    glMatrixMode(GL_MODELVIEW);

    QVector<Node*> &nodes = myScene->nodes();
    for (int i = nodes.size() - 1; i >= 0; --i) {
        glSelectBuffer(64, namebuf);
        glRenderMode(GL_SELECT);

        // Reset name stack
        glInitNames();
        glPushName(0);

        // Draw the node
        drawNode(nodes[i]);

        hits = glRenderMode(GL_RENDER);

        if (hits) {
            hitNode = nodes[i];
            break;
        }
    }

    glMatrixMode(GL_MODELVIEW);
    this->repaint();

    return hitNode;
}
