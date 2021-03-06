#include "barabasialbert.h"
#include "ui_barabasialbert.h"

#include <QWidget>

BarabasiAlbert::BarabasiAlbert(GraphScene *graph):
    Algorithm(graph),
    graph(graph),
    ctlW(0),
    barabasiCtl(0),
    size(START_NODES),
    nodeDegree(START_DEGREE)
{
    updatePreference(graph->nodes(), 2 * graph->edges().size());
}

BarabasiAlbert::~BarabasiAlbert() {
    if (ctlW != 0) {
        delete ctlW;
        delete barabasiCtl;
    }
}

int BarabasiAlbert::getNumNodes() const {
    return size;
}

int BarabasiAlbert::getNodeDegree() const {
    return nodeDegree;
}

void BarabasiAlbert::reset() {
    preferences.clear();
    cumulativePreferences.clear();
    for (int i(0); i < size; ++i) {
        addVertex(false);
    }
    updateUI();
}

QWidget* BarabasiAlbert::controlWidget(QWidget *parent) {
    if (ctlW == 0) {
        ctlW = new QWidget(parent);
        barabasiCtl = new Ui::BarabasiControl();
        barabasiCtl->setupUi(ctlW);

        updateUI();

        connect(barabasiCtl->sizeEdit, SIGNAL(valueChanged(int)), this, SLOT(onSizeChanged(int)));
        connect(barabasiCtl->degreeEdit, SIGNAL(valueChanged(int)), this, SLOT(onDegreeChanged(int)));
    }
    return ctlW;
}

bool BarabasiAlbert::canAddVertex() {
    return true;
}

void BarabasiAlbert::addVertex() {
    addVertex(true);
}

void BarabasiAlbert::addVertex(bool saveSize) {
    addVertex(nodeDegree);
    if (saveSize) {
        ++size;
    }
    updateUI();
}

// Add vertex using preferential attachment without clustering.
void BarabasiAlbert::addVertex(int edgesToAdd) {
    Node *vPref;
    int numNodes = graph->nodes().size();
    int numEdges = graph->edges().size();
    QList<Node*> usedNodes;
    QVector<Node*> nodes = graph->nodes(); /* important: doesn't contain the new node */
    Node *vertex = graph->newNode();

    // saftey check to ensure that the method
    // does not get stuck looping

    if (edgesToAdd > numNodes) {
        edgesToAdd = numNodes;
    }

    while (edgesToAdd > 0) {
        vPref = getPreference(nodes, genRandom());
        int cutOff;
        for (cutOff = 0; cutOff < 100 && !graph->newEdge(vertex, vPref); ++cutOff) {
            vPref = getPreference(nodes, genRandom());
        }
        if (cutOff == 100)
            break;

        --edgesToAdd;

        usedNodes << vPref;

        if (usedNodes.size() >= numNodes)
            break;
    }

    updatePreference(graph->nodes(), 2 * numEdges);
}


void BarabasiAlbert::updatePreference(const QVector<Node*> &nodes, int totalDegree) {
    int prefCumulative = 0;

    if (nodes.size() == 1) {
        preferences[nodes.first()->tag()] = 100;
        cumulativePreferences[nodes.first()->tag()] = 100;
        return;
    }

    foreach (Node *node, nodes) {
        double tempLength = (double)node->edges().length();
        double tempPref = (tempLength / (double) totalDegree) * 100;
        preferences[node->tag()] = tempPref;
        cumulativePreferences[node->tag()] = prefCumulative;
        prefCumulative += tempPref;
    }
}

// Return the preferred node, using binary search.
Node* BarabasiAlbert::getPreference(const QVector<Node*> &nodes, double genPref) {
    const float E = 0.0001;
    int l;
    for (l = 1; l < nodes.size(); l <<= 1)
        ;
    int i(0);
    for (; l > 0; l >>= 1) {
        if (l + i < nodes.size()) {
            if (cumulativePreferences[l + i] <= genPref + E)
                i += l;
        }
    }
    return nodes[i];
}

// Generate random double with 4 precision.
double BarabasiAlbert::genRandom(){
    double main = qrand() % 100;
    return main + (( qrand() % 10000 ) / 10000 );
}

void BarabasiAlbert::onSizeChanged(int newSize) {
    if (newSize == size)
        return;

    size = newSize;
    updateUI();
    graph->repopulate();
}

void BarabasiAlbert::onDegreeChanged(int newDegree) {
    if (newDegree == nodeDegree)
        return;

    nodeDegree = newDegree;
    updateUI();
    graph->repopulate();
}

void BarabasiAlbert::updateUI() {
    if (!barabasiCtl)
        return;

    if (barabasiCtl->sizeEdit->value() != size) {
        barabasiCtl->sizeEdit->setValue(size);
    }
    if (barabasiCtl->degreeEdit->value() != nodeDegree) {
        barabasiCtl->degreeEdit->setValue(nodeDegree);
    }
}
