#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>


#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

#include "hidworker.h"




namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    HidWorker* m_hidWorker;
    QThread*   m_hidThread;



    // void connectToHID();
    void createPlot();

public slots:
    void setTemperatur();
    void getTemperatur();

    void setPID_P();
    void getPID_P();

    void setPID_D();
    void getPID_D();

    void setCompressorOnTime();
    void getCompressorOnTime();

    void setCycleTime();
    void getCycleTime();
    void getSetPoint();

private slots:
    void onHidData(const QByteArray &data);
    void on_pushButton_2_clicked();
    void on_btnTest_clicked();
    void addDataPoint(double curTemp);


    void on_btnSetPID_P_clicked();

    void on_btnSetPID_D_clicked();

    void on_btnSetTimeBaseWork_clicked();

private:
    QwtPlot *plot;
    QwtPlotCurve *curve;
    QVector<double> timeData;
    QVector<double> tempData;
    QTimer *timer;
    double elapsedTime = 0;

signals:
    void sendToHid(const QByteArray &data);

protected:
    void closeEvent(QCloseEvent *event) override;

};



#endif // MAINWINDOW_H
