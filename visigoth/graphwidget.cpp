#include "edge.h"
#include "graphwidget.h"
#include "node.h"
#include "randomgenerator.h"
#include "francescogenerator.h"

#include <cmath>
#include <QGraphicsScene>
#include <QKeyEvent>

GraphWidget::GraphWidget(QWidget *parent) :
    QGraphicsView(parent),
    helping(true),
    helpText(),
    timerId(0)
{
    setMinimumSize(HELP_WIDTH + 10, HELP_HEIGHT + 10);
    scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(Qt::black);
    //FIXME: look into scene index
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(scene);

    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);

    helpText.setText("<h3>Welcome to Visigoth Graph Visualisations</h3>"
                     "<p>Drag nodes to move. Press <em>ESC</em> to close this help.</p>"
                     "<p>Keybindings:"
                     "<ul>"
                     "<li><em>h</em> - show this text</li>"
                     "<li><em>r</em> - generate a new graph</li>"
                     "<li>&lt;<em>spc</em>&gt; - randomize node placement</li>"
                     "<li>&lt;<em>esc</em>&gt; - return to graph view</li>"
                     "</ul>"
                     "</p>"
                     );
    helpText.setTextWidth(HELP_WIDTH - 10);
}

QVector<Node*> GraphWidget::nodes() const {
    return nodeVector;
}

void GraphWidget::populate() {
    generator = new FrancescoGenerator(this, 600);
    generator->populate(nodeVector, edges);

    foreach (Node *node, nodeVector) {
        scene->addItem(node);
    }

    foreach (Edge *edge, edges) {
        scene->addItem(edge);
    }

    randomizePlacement();
}

void GraphWidget::itemMoved() {
    if (!timerId)
        timerId = startTimer(1000 / 25);
}

void GraphWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_H:
        helping = !helping;
        viewport()->update();
        break;
    case Qt::Key_R:
        scene->clear();
        populate();
        break;
    case Qt::Key_Escape:
        helping = false;
        viewport()->update();
        break;
    case Qt::Key_Plus:
        scaleView(qreal(1.2));
        break;
    case Qt::Key_Minus:
        scaleView(1 / qreal(1.2));
        break;
    case Qt::Key_Space:
        randomizePlacement();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void GraphWidget::timerEvent(QTimerEvent *) {
    foreach (Node *node, nodeVector)
        node->calculateForces();

    bool itemsMoved = false;
    foreach (Node *node, nodeVector) {
        if (node->advance())
            itemsMoved = true;
    }

    if (!itemsMoved) {
        killTimer(timerId);
        timerId = 0;
    }
}

void GraphWidget::wheelEvent(QWheelEvent *event) {
    scaleView(pow((double)2, event->delta() / 240.0));
}

void GraphWidget::paintEvent(QPaintEvent *event) {
    QGraphicsView::paintEvent(event);

    if (helping) {
        QPainter painter(viewport());
        QRectF popup((width() - HELP_WIDTH) / 2, (height() - HELP_HEIGHT) / 2, HELP_WIDTH, HELP_HEIGHT);
        painter.setBrush(QBrush(QColor::fromRgb(140, 140, 255, 100)));
        painter.setPen(QPen(QColor::fromRgb(140, 140, 255, 150), 2));
        painter.drawRoundedRect(popup.adjusted(-2, -2, +2, +2), 4, 4);
        painter.setPen(QPen(QColor::fromRgb(0, 0, 20), 1));
        painter.drawStaticText(popup.left() + 5, popup.top() + 5, helpText);
    }
}

void GraphWidget::scaleView(qreal scaleFactor) {
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;
    scale(scaleFactor, scaleFactor);
}

void GraphWidget::randomizePlacement() {
    foreach (Node *node, nodeVector) {
        node->setPos(10 + qrand() % 1000, 10 + qrand() % 600);
    }
    foreach (Edge *edge, edges) {
        edge->adjust();
    }
}
