#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QTabWidget>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateProcesses();
    void killProcess();
    void updateGPUInfo();

private:
    Ui::MainWindow *ui;
    QTableWidget *processTable;
    QPushButton *refreshButton;
    QPushButton *killButton;
    QTimer *refreshTimer;

    QTabWidget *tabWidget;
    QWidget *cpuTab;
    QWidget *gpuTab;
    QLabel *gpuInfoLabel;

    void fetchProcesses();
    void parseStatFile(const QString& statFileContent, int& pid, QString& comm, int& cpuUsage, int& memUsage);
    uid_t getProcessUID(int pid);
};

#endif // MAINWINDOW_H
