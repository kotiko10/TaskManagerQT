#include "mainwindow.h"
#include "ui_mainwindow.h"


// for logic purposes

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

//for QT purposes
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QProcess>
#include <QTimer>
#include <QStringList>
#include <QRegularExpression>
#include <QMessageBox>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    tabWidget = new QTabWidget(this);

    // CPU Tab
    cpuTab = new QWidget(this);
    processTable = new QTableWidget(this);
    processTable->setColumnCount(4);
    processTable->setHorizontalHeaderLabels(QStringList() << "PID" << "Name" << "CPU Usage (%)" << "Memory Usage (KB)");
    processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    refreshButton = new QPushButton("Refresh", this);
    killButton = new QPushButton("Kill Process", this);

    QVBoxLayout *cpuLayout = new QVBoxLayout;
    cpuLayout->addWidget(processTable);
    cpuLayout->addWidget(refreshButton);
    cpuLayout->addWidget(killButton);
    cpuTab->setLayout(cpuLayout);

    tabWidget->addTab(cpuTab, "Processes");

    // GPU Tab
    gpuTab = new QWidget(this);
    gpuInfoLabel = new QLabel(this);

    QVBoxLayout *gpuLayout = new QVBoxLayout;
    gpuLayout->addWidget(gpuInfoLabel);
    gpuTab->setLayout(gpuLayout);

    tabWidget->addTab(gpuTab, "GPU Info");

    setCentralWidget(tabWidget);

    // Connect signals and slots
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateProcesses);
    connect(killButton, &QPushButton::clicked, this, &MainWindow::killProcess);

    // Timer to auto-refresh every 5 seconds
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateProcesses);
    refreshTimer->start(5000);

    updateProcesses();
    updateGPUInfo();
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

uid_t MainWindow::getProcessUID(int pid)
{
    QFile statusFile("/proc/" + QString::number(pid) + "/status");
    if (statusFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&statusFile);
        QString line;
        while (in.readLineInto(&line)) {
            if (line.startsWith("Uid:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                return parts[1].toUInt();  // Return the UID
            }
        }
        statusFile.close();
    }
    return -1;  // Return an invalid UID on failure
}

void MainWindow::killProcess()
{
    int row = processTable->currentRow();
    if (row == -1) {
        QMessageBox::warning(this, "Error", "No process selected");
        return;
    }

    auto pidItem = processTable->item(row, 0);
    if (!pidItem) {
        QMessageBox::warning(this, "Error", "Invalid process selected");
        return;
    }

    int pid = pidItem->text().toInt();
    uid_t uid = getProcessUID(pid);
    if (uid == 0) {  // Check if the process is owned by root
        QMessageBox::warning(this, "Error", "Cannot kill system process");
        return;
    }

    if (kill(pid, SIGKILL) == -1) {
        QMessageBox::critical(this, "Error", "Failed to kill process");
    } else {
        QMessageBox::information(this, "Success", "Process killed successfully");
        updateProcesses();  // Refresh the process list
    }
}

void MainWindow::updateGPUInfo()
{
    QProcess process;
    process.start("nvidia-smi --query-gpu=name,memory.total,memory.used,temperature.gpu --format=csv,noheader");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    if (output.isEmpty()) {
        gpuInfoLabel->setText("No NVIDIA GPU detected or nvidia-smi not available.");
    } else {
        gpuInfoLabel->setText(output);
    }
}
