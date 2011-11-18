#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <QObject>

class QWidget;

class Algorithm : public QObject {
    Q_OBJECT

public:
    Algorithm(QObject *parent = 0);
    virtual ~Algorithm();

    virtual void reset() = 0;
    virtual bool canAddVertex() = 0;
    virtual void addVertex() = 0;
    virtual QWidget* controlWidget(QWidget *parent = 0) = 0;
};

#endif // ALGORITHM_H
