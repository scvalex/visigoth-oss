#include "graphscene.h"
#include "barabasialbert.h"



Barabasialbert::Barabasialbert(GraphScene *graph) :
    graph(graph)
{
    updatePreference(graph->nodes(), 2 * graph->edges().size());
}

void Barabasialbert::init(int size) {
    for (int i(0); i < size; ++i) {
        addVertex();
    }
}

void Barabasialbert::addVertex() {
    addVertex((qrand() % 3 ) + 1, qrand() % 100);
}

// Add vertex using preferential attachment without clustering.
void Barabasialbert::addVertex(int edgesToAdd, double p) {
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

        if (usedNodes.count() >= numNodes)
            break;
    }

    updatePreference(graph->nodes(), 2 * numEdges);
}


void Barabasialbert::updatePreference(const QVector<Node*> &nodes, int totalDegree) {
    int prefCumulative = 0;

    if (nodes.count() == 1) {
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
Node* Barabasialbert::getPreference(const QVector<Node*> &nodes, double genPref) {
    const float E = 0.01;
    int l;
    for (l = 1; l < nodes.count(); l <<= 1)
        ;
    int i(0);
    for (; l > 0; l >>= 1) {
        if (l + i < nodes.count()) {
            if (cumulativePreferences[l + i] <= genPref + E)
                i += l;
        }
    }
    return nodes[i];
}


// Generate random double with 2 precision.
double Barabasialbert::genRandom(){
    double main = qrand() % 100;
    return main + (( qrand() % 100 ) / 100 );
}

