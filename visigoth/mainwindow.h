#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "glgraphwidget.h"

#include <QMainWindow>
#include <QGraphicsView>

class Algorithm;
class QDockWidget;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void populate();

public slots:
    void exportTo();
    void toggleShowControl(bool enabled);

private slots:
    void onAlgorithmChanged(Algorithm *newAlgo);

private:
    Ui::MainWindow *ui;
    GLGraphWidget *view;
    QDockWidget *algoCtl;
};

#endif // MAINWINDOW_H
