#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QtCore>

#include "Assert.h"
#include "Directory.h"
#include "Map.h"
#include "MazeFileType.h"
#include "MazeFiles.h"
#include "Model.h"
#include "MouseAlgos.h"
#include "Param.h"
#include "SimUtilities.h"
#include "State.h"
#include "Time.h"

namespace mms {

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        m_map(new Map),
        m_maze(nullptr),
        m_truth(nullptr),
        m_mouse(nullptr),
        m_mouseGraphic(nullptr),
        m_view(nullptr),
        m_controller(nullptr),
        m_mouseAlgoThread(nullptr) {

    // Initialize the UI
    ui->setupUi(this);

    // Add map to the UI
    ui->mapLayout->addWidget(m_map);

    // Resize the window
    resize(P()->defaultWindowWidth(), P()->defaultWindowHeight());

    // TODO: MACK - settings somewhere
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    ui->buildTextEdit->document()->setDefaultFont(font);
    ui->runTextEdit->document()->setDefaultFont(font);
    ui->buildTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Load saved algos
    for (const QString& algoName : MouseAlgos::algoNames()) {
        ui->selectAlgorithmComboBox->addItem(algoName);
    }

    // Connect the build button
    connect(
        ui->buildButton,
        &QPushButton::clicked,
        this,
        [&](){
            // TODO: MACK - helper for this (ensure all fields exist)
            const QString& algoName = ui->selectAlgorithmComboBox->currentText();
            ASSERT_TR(MouseAlgos::algoNames().contains(algoName));
            build(algoName);
        }
    );

    // Connect the run button
    connect(
        ui->runButton,
        &QPushButton::clicked,
        this,
        [&](){
            // TODO: MACK - helper for this (ensure all fields exist)
            const QString& algoName = ui->selectAlgorithmComboBox->currentText();
            ASSERT_TR(MouseAlgos::algoNames().contains(algoName));
            spawnMouseAlgo(algoName);
        }
    );

    // TODO: MACK - connect the import button (add more functionality here)
    connect(
        ui->importButton,
        &QPushButton::clicked,
        this,
        [&](){
            QString dir = QFileDialog::getExistingDirectory(
                this,
                tr("Open Directory"),
                "/home",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );
        }
    );

    // TODO: MACK - connect the import mazes button
    connect(
        ui->importMazesButton,
        &QPushButton::clicked,
        this,
        [&](){
            QStringList suffixes;
            for (const QString& suffix : MAZE_FILE_TYPE_TO_SUFFIX.values()) {
                suffixes.append(QString("*.") + suffix);
            }
            QStringList paths = QFileDialog::getOpenFileNames(
                this,
                tr("Select one or more maze files to import"),
                "/home",
                QString("Mazes Files (") + suffixes.join(" ") + ")"
            );
            for (const QString& path : paths) {
                MazeFiles::addMazeFile(path);
            }
            refreshMazeFiles();
        }
    );

    // TODO: MACK - set some maze files table properties
    ui->mazeFilesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->mazeFilesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->mazeFilesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->mazeFilesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents); 
    refreshMazeFiles();

    connect(
        ui->mazeFilesTable,
        &QTableWidget::itemSelectionChanged,
        this,
        [&](){
            QString path = ui->mazeFilesTable->selectedItems().at(1)->text();
            Maze* maze = Maze::fromFile(path);
            if (maze == nullptr) {
                return;
            }

            Maze* prev_maze = m_maze;
            MazeView* prev_truth = m_truth;
            m_maze = maze;
            m_truth = new MazeView(m_maze);
            m_map->setMaze(m_maze);
            m_map->setView(m_truth);
            m_map->setMouseGraphic(nullptr);

            if (m_controller != nullptr) {
                connect(m_mouseAlgoThread, &QThread::finished, this, [=](){
                    qDebug() << "THREAD FINISHED"; 
                    m_controller->deleteLater();
                    m_controller = nullptr;
                    Model::get()->setMaze(m_maze);
                    delete prev_maze;
                    delete prev_truth;
                    // TODO: MACK - the deletes below are causing problems :/
                    delete m_mouseAlgoThread;
                    delete m_mouseGraphic;
                    delete m_view;
                    delete m_mouse;
                });
                m_mouseAlgoThread->quit();
            }
            else {
                Model::get()->setMaze(m_maze);
                delete prev_maze;
                delete prev_truth;
            }
        }
    );
}

MainWindow::~MainWindow() {
    // TODO: MACK - other cleanup here
    delete m_maze;
    delete m_map;
    delete ui;
}

bool MainWindow::eventFilter(QObject *object, QEvent *e) {
    if (e->type() == QEvent::KeyPress) {
        keyPress(static_cast<QKeyEvent*>(e)->key());
        return true;
    }
    else if (e->type() == QEvent::KeyRelease) {
        keyRelease(static_cast<QKeyEvent*>(e)->key());
        return true;
    }
    // Let the event propogate to other widgets
    return false;
}

void MainWindow::keyPress(int key) {

    // NOTE: If you're adding or removing anything from this function, make
    // sure to update wiki/Keys.md

    if (key == Qt::Key_P) {
        // Toggle pause (only in discrete mode)
        if (m_controller != nullptr) {
            if (m_controller->getInterfaceType(false) == InterfaceType::DISCRETE) {
                S()->setPaused(!S()->paused());
            }
            else {
                qWarning().noquote().nospace()
                    << "Pausing the simulator is only allowed in "
                    << INTERFACE_TYPE_TO_STRING.value(InterfaceType::DISCRETE)
                    << " mode.";
            }
        }
    }
    else if (key == Qt::Key_F) {
        // Faster (only in discrete mode)
        if (m_controller != nullptr) {
            if (m_controller->getInterfaceType(false) == InterfaceType::DISCRETE) {
                S()->setSimSpeed(S()->simSpeed() * 1.5);
            }
            else {
                qWarning().noquote().nospace()
                    << "Increasing the simulator speed is only allowed in "
                    << INTERFACE_TYPE_TO_STRING.value(InterfaceType::DISCRETE)
                    << " mode.";
            }
        }
    }
    else if (key == Qt::Key_S) {
        // Slower (only in discrete mode)
        if (m_controller != nullptr) {
            if (m_controller->getInterfaceType(false) == InterfaceType::DISCRETE) {
                S()->setSimSpeed(S()->simSpeed() / 1.5);
            }
            else {
                qWarning().noquote().nospace()
                    << "Decreasing the simulator speed is only allowed in "
                    << INTERFACE_TYPE_TO_STRING.value(InterfaceType::DISCRETE)
                    << " mode.";
            }
        }
    }
    else if (key == Qt::Key_L) {
        // Cycle through the available layouts
        S()->setLayoutType(LAYOUT_TYPE_CYCLE.value(S()->layoutType()));
    }
    else if (key == Qt::Key_R) {
        // Toggle rotate zoomed map
        S()->setRotateZoomedMap(!S()->rotateZoomedMap());
    }
    else if (key == Qt::Key_I) {
        // Zoom in
        S()->setZoomedMapScale(S()->zoomedMapScale() * 1.5);
    }
    else if (key == Qt::Key_O) {
        // Zoom out
        S()->setZoomedMapScale(S()->zoomedMapScale() / 1.5);
    }
    else if (key == Qt::Key_T) {
        // Toggle wall truth visibility
        S()->setWallTruthVisible(!S()->wallTruthVisible());
        // TODO: MACK - update both MazeViews?
        m_view->getMazeGraphic()->updateWalls();
    }
    else if (key == Qt::Key_C) {
        // Toggle tile colors
        S()->setTileColorsVisible(!S()->tileColorsVisible());
        // TODO: MACK - update both MazeViews?
        m_view->getMazeGraphic()->updateColor();
    }
    else if (key == Qt::Key_G) {
        // Toggle tile fog
        S()->setTileFogVisible(!S()->tileFogVisible());
        // TODO: MACK - update both MazeViews?
        m_view->getMazeGraphic()->updateFog();
    }
    else if (key == Qt::Key_X) {
        // Toggle tile text
        S()->setTileTextVisible(!S()->tileTextVisible());
        // TODO: MACK - update both MazeViews?
        m_view->getMazeGraphic()->updateText();
    }
    else if (key == Qt::Key_D) {
        // Toggle tile distance visibility
        S()->setTileDistanceVisible(!S()->tileDistanceVisible());
        // TODO: MACK - update both MazeViews?
        m_view->getMazeGraphic()->updateText();
    }
    else if (key == Qt::Key_Q) {
        // Quit
        SimUtilities::quit();
    }
    else if (
        key == Qt::Key_0 || key == Qt::Key_1 ||
        key == Qt::Key_2 || key == Qt::Key_3 ||
        key == Qt::Key_4 || key == Qt::Key_5 ||
        key == Qt::Key_6 || key == Qt::Key_7 ||
        key == Qt::Key_8 || key == Qt::Key_9
    ) {
        // Press an input button
        int inputButton = key - Qt::Key_0;
        if (!S()->inputButtonWasPressed(inputButton)) {
            S()->setInputButtonWasPressed(inputButton, true);
            qInfo().noquote().nospace()
                << "Input button " << inputButton << " was pressed.";
        }
        else {
            qWarning().noquote().nospace()
                << "Input button " << inputButton << " has not yet been"
                << " acknowledged as pressed; pressing it has no effect.";
        }
    }
    else if (
        key == Qt::Key_Up || key == Qt::Key_Down ||
        key == Qt::Key_Left || key == Qt::Key_Right
    ) {
        S()->setArrowKeyIsPressed(key, true);
    }
}

void MainWindow::keyRelease(int key) {
    if (
        key == Qt::Key_Up || key == Qt::Key_Down ||
        key == Qt::Key_Left || key == Qt::Key_Right
    ) {
        S()->setArrowKeyIsPressed(key, false);
    }
}

void MainWindow::refreshMazeFiles() {
    QStringList mazeFiles = MazeFiles::getMazeFiles();
    ui->mazeFilesTable->setRowCount(mazeFiles.size());
    ui->mazeFilesTable->setColumnCount(2);
    for (int i = 0; i < mazeFiles.size(); i += 1) {
        QString path = mazeFiles.at(i);
        ui->mazeFilesTable->setItem(i, 0, new QTableWidgetItem(QFileInfo(path).fileName()));
        ui->mazeFilesTable->setItem(i, 1, new QTableWidgetItem(path));
    }
}

void MainWindow::mazeBuild(const QString& mazeAlgoName) {

    // TODO: MACK - check settings actually contains these values and contains
    // and path validity here
    /*
    QString dirPath = MazeAlgos::getDirPath(algoName);
    QString buildCommand = MazeAlgos::getBuildCommand(algoName);

    // TODO: MACK - ensure we're cleaning this up properly
    // First, instantiate a new process
    m_mazeBuildProcess = new QProcess(this);
    m_mazeBuildProcess->setWorkingDirectory(dirPath);

    // Listen for build errors
    connect(
        m_mazeBuildProcess,
        &QProcess::readyReadStandardError,
        this,
        [&](){
            QString errors = m_mazeBuildProcess->readAllStandardError();
            ui->buildTextEdit->appendPlainText(errors);
        }
    );

    // Re-enable build button when build finishes
    connect(
        m_mazeBuildProcess,
        static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished
        ),
        this,
        [&](int exitCode, QProcess::ExitStatus exitStatus){
            ui->buildButton->setEnabled(true);
        }
    );
            
    // Disable the build button before build starts
    ui->buildButton->setEnabled(false);

    // Now, start the build
    QStringList args = buildCommand.split(' ', QString::SkipEmptyParts);
    // TODO: MACK - check sanity of build command
    ASSERT_LT(0, args.size());
    QString command = args.at(0);
    args.removeFirst();
    m_mazeBuildProcess->start(command, args);
    */
}

void MainWindow::mazeRun(const QString& mazeAlgoName) {
}

void MainWindow::build(const QString& algoName) {

    // TODO: MACK - check settings actually contains these values and contains
    // and path validity here
    QString dirPath = MouseAlgos::getDirPath(algoName);
    QString buildCommand = MouseAlgos::getBuildCommand(algoName);

    // TODO: MACK - ensure we're cleaning this up properly
    // First, instantiate a new process
    m_buildProcess = new QProcess(this);
    m_buildProcess->setWorkingDirectory(dirPath);

    // Listen for build errors
    connect(
        m_buildProcess,
        &QProcess::readyReadStandardError,
        this,
        [&](){
            QString errors = m_buildProcess->readAllStandardError();
            ui->buildTextEdit->appendPlainText(errors);
        }
    );

    // Re-enable build button when build finishes
    connect(
        m_buildProcess,
        static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished
        ),
        this,
        [&](int exitCode, QProcess::ExitStatus exitStatus){
            ui->buildButton->setEnabled(true);
        }
    );
            
    // Disable the build button before build starts
    ui->buildButton->setEnabled(false);

    // Now, start the build
    QStringList args = buildCommand.split(' ', QString::SkipEmptyParts);
    // TODO: MACK - check sanity of build command
    ASSERT_LT(0, args.size());
    QString command = args.at(0);
    args.removeFirst();
    m_buildProcess->start(command, args);
}

void MainWindow::spawnMouseAlgo(const QString& algoName) {

    // Generate the mouse, lens, and controller
    m_mouse = new Mouse(m_maze);
    m_mouseGraphic = new MouseGraphic(m_mouse);
    m_view = new MazeViewMutable(m_maze);
    m_controller = new Controller(m_maze, m_mouse, m_view);

    // Configures the window to listen for build and run stdout,
    // as forwarded by the controller, and adds a map to the UI

    /////////////////////////////////

    // Add a map to the UI
    // TODO: MACK - minimum size, size policy
    m_map->setView(m_view);
    m_map->setMouseGraphic(m_mouseGraphic);

    // Listen for mouse algo stdout
    connect(
        m_controller,
        &Controller::algoStdout,
        ui->runTextEdit,
        &QPlainTextEdit::appendPlainText
    );

    // TODO: MACK - group the position stuff together

    /*
    // Add run stats info to the UI
    QVector<QPair<QString, QVariant>> runStats = getRunStats();
    for (int i = 0; i < runStats.size(); i += 1) {
        QString label = runStats.at(i).first;
        QLabel* labelHolder = new QLabel(label + ":");
        QLabel* valueHolder = new QLabel();
        ui->runStatsLayout->addWidget(labelHolder, i, 0);
        ui->runStatsLayout->addWidget(valueHolder, i, 1);
        m_runStats.insert(label, valueHolder);
    }

    // Add run stats info to the UI
    QVector<QPair<QString, QVariant>> mazeInfo = getMazeInfo();
    for (int i = 0; i < mazeInfo.size(); i += 1) {
        QString label = mazeInfo.at(i).first;
        QLabel* labelHolder = new QLabel(label + ":");
        QLabel* valueHolder = new QLabel();
        ui->mazeInfoLayout->addWidget(labelHolder, i, 0);
        ui->mazeInfoLayout->addWidget(valueHolder, i, 1);
        m_mazeInfo.insert(label, valueHolder);
    }

    // Periodically update the header
    connect(
        &m_headerRefreshTimer,
        &QTimer::timeout,
        this,
        [=](){
            // TODO: MACK - only update if tab is visible
            QVector<QPair<QString, QVariant>> runStats = getRunStats();
            for (const auto& pair : runStats) {
                QString text = pair.second.toString();
                if (pair.second.type() == QVariant::Double) {
                    text = QString::number(pair.second.toDouble(), 'f', 3);
                }
                m_runStats.value(pair.first)->setText(text);
            }
            QVector<QPair<QString, QVariant>> mazeInfo = getMazeInfo();
            for (const auto& pair : mazeInfo) {
                QString text = pair.second.toString();
                if (pair.second.type() == QVariant::Double) {
                    text = QString::number(pair.second.toDouble(), 'f', 3);
                }
                m_mazeInfo.value(pair.first)->setText(text);
            }
        }
    );
    m_headerRefreshTimer.start(50);
    */
    /////////////////////////////////

    // The thread on which the controller will execute
    m_mouseAlgoThread = new QThread();

    // We need to actually spawn the QProcess (i.e., m_process = new
    // QProcess()) in the separate thread, hence why this is async. Note that
    // we need the separate thread because, while it's performing an
    // algorithm-requested action, the Controller could block the GUI loop from
    // executing.
    connect(m_mouseAlgoThread, &QThread::started, m_controller, [=](){
        // We need to add the mouse to the world *after* the the controller is
        // initialized (thus ensuring that tile fog is cleared automatically),
        // but *before* we actually start the algorithm (lest the mouse
        // position/orientation not be updated properly during the beginning of
        // the mouse algo's execution)
        m_controller->init();
        Model::get()->addMouse("", m_mouse);
        m_controller->start(algoName);
    });
    m_controller->moveToThread(m_mouseAlgoThread);
	m_mouseAlgoThread->start();
}

QVector<QPair<QString, QVariant>> MainWindow::getRunStats() const {

    MouseStats stats;
    if (Model::get()->containsMouse("")) {
        stats = Model::get()->getMouseStats("");
    }

    return {
        {"Run ID", S()->runId()}, // TODO: MACK - run directory
        {"Random Seed", P()->randomSeed()},
        // {"Mouse Algo", P()->mouseAlgorithm()}, // TODO: MACK
        {"Tiles Traversed",
            QString::number(stats.traversedTileLocations.size()) + " / " +
            QString::number(m_maze->getWidth() * m_maze->getHeight())
        },
        {"Closest Distance to Center", stats.closestDistanceToCenter},
        {"Current X (m)", m_mouse->getCurrentTranslation().getX().getMeters()},
        {"Current Y (m)", m_mouse->getCurrentTranslation().getY().getMeters()},
        {"Current Rotation (deg)", m_mouse->getCurrentRotation().getDegreesZeroTo360()},
        {"Current X tile", m_mouse->getCurrentDiscretizedTranslation().first},
        {"Current Y tile", m_mouse->getCurrentDiscretizedTranslation().second},
        {"Current Direction",
            DIRECTION_TO_STRING.value(m_mouse->getCurrentDiscretizedRotation())
        },
        {"Elapsed Real Time", SimUtilities::formatDuration(Time::get()->elapsedRealTime())},
        {"Elapsed Sim Time", SimUtilities::formatDuration(Time::get()->elapsedSimTime())},
        {"Time Since Origin Departure",
            stats.timeOfOriginDeparture.getSeconds() < 0
            ? "NONE"
            : SimUtilities::formatDuration(
                Time::get()->elapsedSimTime() - stats.timeOfOriginDeparture)
        },
        {"Best Time to Center",
            stats.bestTimeToCenter.getSeconds() < 0
            ? "NONE"
            : SimUtilities::formatDuration(stats.bestTimeToCenter)
        },
        {"Crashed", (S()->crashed() ? "TRUE" : "FALSE")},
    };
}

QVector<QPair<QString, QVariant>> MainWindow::getAlgoOptions() const {
    return {
        // Mouse Info
        // TODO: MACK - get this from the controller
        // {"Mouse Algo", P()->mouseAlgorithm()},
        /*
        {"Mouse File", (
            m_controller == nullptr
            ? "NONE"
            : m_controller->getStaticOptions().mouseFile
        }),
        */
        // TODO: MACK - interface type not finalized
        {"Interface Type",
            m_controller == nullptr
            ? "NONE"
            : INTERFACE_TYPE_TO_STRING.value(m_controller->getInterfaceType(false))
        },
        /*
        QString("Initial Direction:           ") + (m_controller == nullptr ? "NONE" :
            m_controller->getStaticOptions().initialDirection),
        QString("Tile Text Num Rows:          ") + (m_controller == nullptr ? "NONE" :
            QString::number(m_controller->getStaticOptions().tileTextNumberOfRows)),
        QString("Tile Text Num Cols:          ") + (m_controller == nullptr ? "NONE" :
            QString::number(m_controller->getStaticOptions().tileTextNumberOfCols)),
        */
        {"Allow Omniscience",
            m_controller == nullptr
            ? "NONE"
            : m_controller->getDynamicOptions().allowOmniscience ? "TRUE" : "FALSE"
        },
        {"Auto Clear Fog",
            m_controller == nullptr
            ? "NONE"
            : m_controller->getDynamicOptions().automaticallyClearFog ? "TRUE" : "FALSE"
        },
        {"Declare Both Wall Halves",
            m_controller == nullptr
            ? "NONE"
            : m_controller->getDynamicOptions().declareBothWallHalves ? "TRUE" : "FALSE"
        },
        {"Auto Set Tile Text",
            m_controller == nullptr
            ? "NONE"
            : m_controller->getDynamicOptions().setTileTextWhenDistanceDeclared ? "TRUE" : "FALSE"
        },
        {"Auto Set Tile Base Color",
            m_controller == nullptr
            ? "NONE"
            : m_controller->getDynamicOptions().setTileBaseColorWhenDistanceDeclaredCorrectly ? "TRUE" : "FALSE"
        }
        // TODO: MACK
        /*
        QString("Wheel Speed Fraction:        ") +
            (m_controller == nullptr ? "NONE" :
            (STRING_TO_INTERFACE_TYPE.value(m_controller->getStaticOptions().interfaceType) != InterfaceType::DISCRETE ? "N/A" :
            QString::number(m_controller->getStaticOptions().wheelSpeedFraction))),
        QString("Declare Wall On Read:        ") +
            (m_controller == nullptr ? "NONE" :
            (STRING_TO_INTERFACE_TYPE.value(m_controller->getStaticOptions().interfaceType) != InterfaceType::DISCRETE ? "N/A" :
            (m_controller->getDynamicOptions().declareWallOnRead ? "TRUE" : "FALSE"))),
        QString("Use Tile Edge Movements:     ") +
            (m_controller == nullptr ? "NONE" :
            (STRING_TO_INTERFACE_TYPE.value(m_controller->getStaticOptions().interfaceType) != InterfaceType::DISCRETE ? "N/A" :
            (m_controller->getDynamicOptions().useTileEdgeMovements ? "TRUE" : "FALSE"))),
        */
    };
}

QVector<QPair<QString, QVariant>> MainWindow::getMazeInfo() const {
    return {
        // TODO: MACK
        /*
        {
            (P()->useMazeFile() ? "Maze File" : "Maze Algo"),
            (P()->useMazeFile() ? P()->mazeFile() : P()->mazeAlgorithm()),
        },
        */
        {"Maze Width", m_maze->getWidth()},
        {"Maze Height", m_maze->getHeight()},
        {"Maze Is Official", m_maze->isOfficialMaze() ? "TRUE" : "FALSE"},
    };
}

QVector<QPair<QString, QVariant>> MainWindow::getOptions() const {
    return {
        // Sim state
        {"Layout Type (l)", LAYOUT_TYPE_TO_STRING.value(S()->layoutType())},
        {"Rotate Zoomed Map (r)", (S()->rotateZoomedMap() ? "TRUE" : "FALSE")},
        {"Zoomed Map Scale (i, o)", QString::number(S()->zoomedMapScale())},
        {"Wall Truth Visible (t)", (S()->wallTruthVisible() ? "TRUE" : "FALSE")},
        {"Tile Colors Visible (c)", (S()->tileColorsVisible() ? "TRUE" : "FALSE")},
        {"Tile Fog Visible (g)", (S()->tileFogVisible() ? "TRUE" : "FALSE")},
        {"Tile Text Visible (x)", (S()->tileTextVisible() ? "TRUE" : "FALSE")},
        {"Tile Distance Visible (d)", (S()->tileDistanceVisible() ? "TRUE" : "FALSE")},
        {"Paused (p)", (S()->paused() ? "TRUE" : "FALSE")},
        {"Sim Speed (f, s)", QString::number(S()->simSpeed())},
    };
}

} // namespace mms
