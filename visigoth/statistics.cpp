#include "statistics.h"
#include "graphscene.h"

Statistics::Statistics(GraphScene *scene): graph(scene)
{ }

double Statistics::averageDegree() {
    return (2 * graph->edges().count()) / graph->nodes().count();
}

double Statistics::averageLength() {
    double allLengths = 0;

    foreach (Node *n, graph->nodes()) {
        allLengths += lengthSum(n);
        foreach (Node *m, graph->nodes()) {
            m->setDistance(0);
            m->setVisited(false);
        }
    }

    return allLengths / (double)(nodes.count()*(nodes.count() - 1));
}

double Statistics::clusteringAvg() {
    double clusterCumulative = 0.0;

    foreach (Node *n, graph->nodes()) {
        clusterCumulative += clusteringCoeff(n);
    }

    return clusterCumulative / (double)nodes.count();
}

double Statistics::clusteringCoeff(Node *node) {
    QList<Edge*> edges = node->edges();
    int k = edges.count();
    int intersection = 0;

    while(!edges.empty()) {
        Edge *e = edges.takeFirst();
        Node *src = e->sourceNode();
        Node *dest = e->destNode();

        QVector<Node*> nNeigh = buildNeighbourVector(node);

        if(src->tag() == node->tag()) {
            QVector<Node*> dNeigh = buildNeighbourVector(dest);
            intersection += intersectionCount(nNeigh, dNeigh);
        } else {
            QVector<Node*> dNeigh = buildNeighbourVector(dest);
            intersection += intersectionCount(nNeigh, dNeigh);
        }
    }

    return k > 1 ? (2 * intersection) / (double)(k*(k-1)) : 0;
}

double Statistics::clusteringDegree(int degree) {
    QList<Node*> nodeList = graph->getDegreeList(degree);
    int degreeCount = nodeList.count();
    int clusterCumulative = 0;

    foreach (Node *n, nodeList) {
        clusterCumulative += clusteringCoeff(n);
    }

    return clusterCumulative / degreeCount;
}


// private:

QVector<Node*> Statistics::buildNeighbourVector(Node *n) {
    QList<Edge*> eList = n->edges();
    QVector<Node*> retVec(eList.count());

    while(!eList.empty()) {
        Edge *e = eList.takeFirst();

        if(e->sourceNode()->tag() == n->tag()) {
            retVec << e->destNode();
        } else {
            retVec << e->sourceNode();
        }
    }

    return retVec;
}

double Statistics::lengthSum(Node *s) {
    QList<Node*> queue;
    queue.append(s);
    double retLength = 0;
    s->setVisited(true);

    // Find the distances to all other nodes using breadth first search
    while(!queue.empty()) {
        Node *parent = queue.first();
        QList<Edge*> edges = parent->edges();

        foreach(Edge *e, edges) {
            Node *n;

            if(e->sourceNode()->tag() == parent->tag()) {
                n = e->destNode();
            } else {
                n = e->sourceNode();
            }

            if(!n->getVisited()) {
                n->setVisited(true);
                n->setDistance(parent->getDistance() + 1);
                queue.append(n);
            }
        }

        queue.removeFirst();
        retLength += parent->getDistance();
    }

    return retLength;
}

int Statistics::intersectionCount(QVector<Node*> vec1, QVector<Node*> vec2) {
    QVector<Node*> retVec;
    QVector<Node*> *shorterVec;
    QVector<Node*> *longerVec;
    int length;

    if (vec1.count() > vec2.count()) {
        length = vec1.count();
        shorterVec = &vec2;
        longerVec = &vec1;
    } else {
       length = vec2.count();
       shorterVec = &vec1;
       longerVec = &vec2;
    }

    for (int i(0); i < length; ++i) {
        Node* tempPointer = longerVec->at(i);
        if (shorterVec->contains(tempPointer)) {
            retVec << tempPointer;
        }
    }

    return retVec.count();
}
