#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QProcess>
#include <QTimer>
#include <QStringList>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set up the UI components
    processTable = new QTableWidget(this);
    processTable->setColumnCount(4);
    processTable->setHorizontalHeaderLabels(QStringList() << "PID" << "Name" << "CPU Usage (%)" << "Memory Usage (KB)");
    processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    refreshButton = new QPushButton("Refresh", this);
    killButton = new QPushButton("Kill", this);


    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(processTable);
    layout->addWidget(refreshButton);
    layout->addWidget(killButton);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    // Connect signals and slots
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateProcesses);

    // Timer to auto-refresh every 5 seconds
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateProcesses);
    refreshTimer->start(5000);

    updateProcesses();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateProcesses()
{
    fetchProcesses();
}

void MainWindow::fetchProcesses()
{
    QDir procDir("/proc");
    QStringList procEntries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    processTable->setRowCount(0);

    foreach (const QString &entry, procEntries)
    {
        bool isNumber;
        int pid = entry.toInt(&isNumber);
        if (!isNumber) continue;

        QFile statFile(procDir.filePath(entry + "/stat"));
        if (!statFile.open(QIODevice::ReadOnly)) continue;

        QTextStream in(&statFile);
        QString statFileContent = in.readAll();
        statFile.close();

        int cpuUsage = 0, memUsage = 0;
        QString comm;
        parseStatFile(statFileContent, pid, comm, cpuUsage, memUsage);

        int row = processTable->rowCount();
        processTable->insertRow(row);
        processTable->setItem(row, 0, new QTableWidgetItem(QString::number(pid)));
        processTable->setItem(row, 1, new QTableWidgetItem(comm));
        processTable->setItem(row, 2, new QTableWidgetItem(QString::number(cpuUsage)));
        processTable->setItem(row, 3, new QTableWidgetItem(QString::number(memUsage)));
    }
}

void MainWindow::parseStatFile(const QString& statFileContent, int& pid, QString& comm, int& cpuUsage, int& memUsage)
{
    QStringList statFields = statFileContent.split(" ");

    pid = statFields.at(0).toInt();
    comm = statFields.at(1);
    comm.remove("(").remove(")");

    cpuUsage = statFields.at(13).toInt();  // utime (user mode CPU time)
    cpuUsage += statFields.at(14).toInt(); // stime (kernel mode CPU time)
    cpuUsage += statFields.at(15).toInt(); // cutime (user mode CPU time of children)
    cpuUsage += statFields.at(16).toInt(); // cstime (kernel mode CPU time of children)

    QFile statusFile("/proc/" + QString::number(pid) + "/status");
    if (statusFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&statusFile);
        QString line;
        while (in.readLineInto(&line))
        {
            if (line.startsWith("VmRSS:"))
            {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                memUsage = parts[1].toInt();  // Resident Set Size
                break;
            }
        }
        statusFile.close();
    }
}
