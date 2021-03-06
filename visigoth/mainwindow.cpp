#include "algorithm.h"
#include "graphscene.h"
#include "glgraphwidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_helpWidget.h"
#include "ui_statistics.h"
#include "graphscene.h"
#include "notify.h"
#include "statistics.h"

#include <QDesktopWidget>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QPrinter>
#include <QComboBox>
#include <QStringList>
#include <QObject>
#include <QAction>
#include <QMessageBox>
#include <QColorDialog>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    statsUi(new Ui::Statistics),
    algoCtl(0),
    helpWidget(0),
    focusedNode(0)
{
    qsrand(23);

    QCoreApplication::setOrganizationName("Visigoth");
    QCoreApplication::setApplicationName("Visigoth");

    ui->setupUi(this);

    connect(ui->exportToAct, SIGNAL(triggered()), this, SLOT(exportTo()));
    connect(ui->helpAct, SIGNAL(toggled(bool)), this, SLOT(toggleHelp(bool)));

    view = new GLGraphWidget(this);
    scene = new GraphScene(this);
    view->setScene(scene);

    statsUi->setupUi(ui->statsWidget);

    Notify::init(ui->statusBar);

    setCentralWidget(view);

    connect(ui->newNodeAct, SIGNAL(triggered()), scene, SLOT(addVertex()));
    connect(ui->randomizeAct, SIGNAL(triggered()), scene, SLOT(randomizePlacement()));
    connect(ui->generateAct, SIGNAL(triggered()), scene, SLOT(repopulate()));

    connect(ui->menuCustomizeGraph, SIGNAL(triggered(QAction*)), this, SLOT(customizeColour(QAction*)));

    connect(ui->mode3DAct, SIGNAL(toggled(bool)), view, SLOT(set3DMode(bool)));
    connect(ui->aboutAct, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->aboutQtAct, SIGNAL(triggered()), this, SLOT(showAboutQt()));
    connect(scene, SIGNAL(repopulated()), this, SLOT(onGenerate()));
    connect(view, SIGNAL(algorithmChanged(Algorithm*)), this, SLOT(onAlgorithmChanged(Algorithm*)));
    connect(view, SIGNAL(onSelectNode(Node*)), this, SLOT(onFocusedNodeChanged(Node*)));

    ui->chooserCombo->addItems(scene->algorithms());
    connect(ui->chooserCombo, SIGNAL(currentIndexChanged(const QString &)), scene, SLOT(chooseAlgorithm(const QString &)));

    scene->chooseAlgorithm(ui->chooserCombo->currentText());

    scene->repopulate();

    Notify::normal("All ready");
}

MainWindow::~MainWindow() {
    delete view;
    delete ui;
}

/* Asks the user for a color.  If the user cancels the action,
return false and leave NEWCOLOR unchanged.  Otherwise, return
true and update NEWCOLOUR to match the picked colour. */
bool MainWindow::pickColour(QColor &newColour) {
    const QColor& initial = QColor::fromRgbF(0.0, 0.0, 1.0, 0.5);
    QColor c = QColorDialog::getColor(initial, this, "Select Colour", 0);
    if (!c.isValid()) {
        return false;
    }

    newColour = c;
    return true;
}

void MainWindow::customizeColour(QAction *action) {
    QColor colour;
    if (action == ui->edgeColourAct) {
        if (pickColour(colour)) {
            scene->customizeEdgesColour(colour);
        }
    } else if (action == ui->nodeColourAct) {
        if (pickColour(colour)) {
            scene->customizeNodesColour(colour);
        }
    } else if (action == ui->backgroundColourAct) {
        if (pickColour(colour)) {
            scene->customizeBackgroundColour(colour);
        }
    } else if (action == ui->resetHighlightingAct) {
        view->resetHighlighting();
    }
}

void MainWindow::toggleHelp(bool enabled) {
    if (!helpWidget) {
        helpDock = new QDockWidget(this);
        helpWidget = new QWidget(this);
        Ui::helpWidget *helpWid = new Ui::helpWidget();
        helpWid->setupUi(helpWidget);
        helpWid->text->setSource(QUrl::fromLocalFile(":/resource/helpFile.html"));
        helpWid->text->isReadOnly();
        helpWid->text->setFrameShape(QFrame::NoFrame);
        helpWid->text->viewport()->setAutoFillBackground(false);
        helpDock->setWidget(helpWidget);
        helpDock->setWindowTitle("Help Manual");
        helpDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
        addDockWidget(Qt::LeftDockWidgetArea, helpDock);
    } else {
        helpDock->setVisible(enabled);
    }

}

void MainWindow::exportTo() {
    //first commented version took a screenshot of the whole screen, compared to
    //the 2nd version which now takes only the Widget;

    //QPixmap pixmap = QPixmap::grabWindow(QApplication::desktop()->winId());

    int vWidth = view->width()-15;

    QPixmap pixmap = QPixmap::grabWidget(view, 0, 0, vWidth, -1);

    QString format = "png";
    QString initialPath = QDir::currentPath() + "/untitled." + format;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                    initialPath,
                                    tr("PNG (*.png);;JPG (*.jpg);;All Files (*)"));
    //tr("%1 Files (*.%2);;All Files (*);;JPG (*.jpg)")
    //.arg(format.toUpper())
    //.arg(format));
    if (!fileName.isEmpty()) {
        pixmap.save(fileName, format.toAscii());
    }
}

void MainWindow::onAlgorithmChanged(Algorithm *newAlgo) {
    QWidget *ctl = newAlgo->controlWidget(this);
    if (algoCtl) {
        removeDockWidget(algoCtl);
        delete algoCtl->widget();
        delete algoCtl;
        algoCtl = 0;
    }
    if (ctl) {
        QDockWidget *dock = new QDockWidget(this);
        dock->setWidget(ctl);
        dock->setWindowTitle("Algorithm Control");
        algoCtl = dock;
        addDockWidget(Qt::RightDockWidgetArea, algoCtl);
    }

    ui->newNodeAct->setEnabled(newAlgo->canAddVertex());
}

void MainWindow::onGenerate() {
    focusedNode = 0;
    Statistics *stats = scene->getStatistics();
    statsUi->lengthLabel->setText(QString::number(stats->lengthAvg()));
    statsUi->degreeLabel->setText(QString::number(stats->degreeAvg()));
    statsUi->clusteringLabel->setText(QString::number(stats->clusteringAvg()));
    statsUi->exponentLabel->setText(QString::number(stats->powerLawExponent()));
}

void MainWindow::onFocusedNodeChanged(Node *node) {
    if (node == focusedNode)
        return;
    focusedNode = node;

    Statistics *stats = scene->getStatistics();
    statsUi->coeffLabel->setText(QString::number(stats->clusteringCoeff(focusedNode)));
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "Visigoth", "<h3>Visigoth Graph Visualisations</h3> "
                                         "<p>Visigoth is a tool to generate, analyse and visualise Small World "
                                         "Networks.  It makes these kinds of networks accessible to anyone new "
                                         "to their mathematical properties and assists in discovering the "
                                         "various properties they exhibit by presenting the particular networks "
                                         "generated by some of the currently published algorithms.</p>"
                                         "<h3>Credits</h3>"
                                         "<ul>"
                                         "<li>Andreea-Ingrid Funie &lt;<a href='mailto://aif109@doc.ic.ac.uk'>aif109@doc.ic.ac.uk</a>></li>"
                                         "<li>Alexandru Scvortov &lt;<a href='mailto://as10109@doc.ic.ac.uk'>as10109@doc.ic.ac.uk</a>></li>"
                                         "<li>Francesco Mazzoli &lt;<a href='mailto://fm2209@doc.ic.ac.uk'>fm2209@doc.ic.ac.uk</a>></li>"
                                         "<li>Marc-David Haubenstock &lt;<a href='mailto://mh808@doc.ic.ac.uk'>mh808@doc.ic.ac.uk</a>></li>"
                                         "<li>Maximilian Staudt &lt;<a href='mailto://ms9109@doc.ic.ac.uk'>ms9109@doc.ic.ac.uk</a>></li>"
                                         "</ul>");
}

void MainWindow::showAboutQt() {
    QMessageBox::aboutQt(this);
}
