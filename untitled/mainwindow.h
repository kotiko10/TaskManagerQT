#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>

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

private:
    Ui::MainWindow *ui;
    QTableWidget *processTable;
    QPushButton *refreshButton;
    QPushButton *killButton;
    QTimer *refreshTimer;

    void fetchProcesses();
    void parseStatFile(const QString& statFileContent, int& pid, QString& comm, int& cpuUsage, int& memUsage);
};

#endif // MAINWINDOW_H
