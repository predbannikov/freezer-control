#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringList>
#include <QByteArray>
#include <QPen>
#include <QDebug>


#include <hidapi.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << "MainWindow создан";

    m_hidWorker = new HidWorker(0x3210, 0x0098);
    m_hidThread = new QThread(this);

    m_hidWorker->moveToThread(m_hidThread);

    connect(m_hidThread, &QThread::started, m_hidWorker, &HidWorker::start);
    connect(m_hidWorker, &HidWorker::finished,   m_hidThread, &QThread::quit);
    connect(m_hidWorker, &HidWorker::finished,   m_hidWorker, &HidWorker::deleteLater);
    connect(m_hidThread, &QThread::finished,     m_hidThread, &QThread::deleteLater);

    // Сигнал из GUI на отправку:
    connect(this, &MainWindow::sendToHid, m_hidWorker, &HidWorker::sendData);
    // Сигнал от worker’а о новых данных:
    connect(m_hidWorker, &HidWorker::dataReceived, this, &MainWindow::onHidData);

    m_hidThread->start();

    createPlot();


}

#define COUNT_POINTS 1800

void MainWindow::createPlot()
{
    plot = new QwtPlot(this);
    curve = new QwtPlotCurve("Температура");
    timer = new QTimer(this);
    plot->setTitle("Температура во времени");
    plot->setCanvasBackground(Qt::white);
    plot->setAxisTitle(QwtPlot::xBottom, "Время, сек");
    plot->setAxisTitle(QwtPlot::yLeft, "Температура, °C");
    plot->setAxisScale(QwtPlot::xBottom, 0, COUNT_POINTS);    // 60 секунд
    plot->setAxisScale(QwtPlot::yLeft, -3, 0);     // Диапазон температур

    curve->attach(plot);
    curve->setPen(QPen(Qt::red, 2));

    // QWidget *central = new QWidget(this);
    // QVBoxLayout *layout = new QVBoxLayout(central);
    // layout->addWidget(plot);
    // setCentralWidget(central);
    ui->tabGraphics->addTab(plot, "fsdfa");

    connect(timer, &QTimer::timeout, [this](){ getTemperatur(); });
    timer->start(1000);  // Каждую секунду

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onHidData(const QByteArray &data)
{
    // Преобразуем float в 4 байта
    union {
        float f;
        uint32_t u;
        uint8_t b[4];
    } converter;

    float value = 0.0;
    // По байтам (обычно little-endian)
    // qDebug() << data[0] << data[1];
    converter.b[3] = data[3];
    converter.b[2] = data[2];
    converter.b[1] = data[1];
    converter.b[0] = data[0];
    uint32_t command = converter.u;
    switch (command) {
    case 0x20:      // receive temperature
        converter.b[0] = data[4];
        converter.b[1] = data[5];
        converter.b[2] = data[6];
        converter.b[3] = data[7];
        addDataPoint(converter.f);
        break;
    case 0x21:
        // value = converter.f;
        converter.b[0] = data[4];
        converter.b[1] = data[5];
        converter.b[2] = data[6];
        converter.b[3] = data[7];
        ui->lblPID_P->setText(tr("pid_P=%1").arg(converter.f));
        qDebug() << "receive PID_P" << converter.f;
        break;
    case 0x22:
        // value = converter.f;
        converter.b[0] = data[4];
        converter.b[1] = data[5];
        converter.b[2] = data[6];
        converter.b[3] = data[7];
        ui->lblPID_D->setText(tr("pid_D=%1").arg(converter.f));
        qDebug() << "receive PID_D" << converter.f;
        break;
    case 0x23:
        // value = converter.u;
        converter.b[0] = data[4];
        converter.b[1] = data[5];
        converter.b[2] = data[6];
        converter.b[3] = data[7];
        ui->lblCompressionOnTime->setText(tr("compressionOnTime=%1").arg(converter.u));
        qDebug() << "receive compressorOnTime" << converter.u;
        break;
    case 0x24:
        // value = converter.u;
        converter.b[0] = data[4];
        converter.b[1] = data[5];
        converter.b[2] = data[6];
        converter.b[3] = data[7];
        qDebug() << "receive cycleTime" << converter.u;
        break;
    case 0x25:
        // value = converter.u;
        converter.b[0] = data[4];
        converter.b[1] = data[5];
        converter.b[2] = data[6];
        converter.b[3] = data[7];
        ui->lblSetPoint->setText(tr("setpoint=%1").arg(converter.f));
        qDebug() << "receive cycleTime" << converter.f;
        break;
    }


}

void MainWindow::on_pushButton_2_clicked()
{
    getPID_P();
    getPID_D();
    getCompressorOnTime();
    getCycleTime();
}

void MainWindow::on_btnTest_clicked()
{
    setTemperatur();
}

void MainWindow::addDataPoint(double curTemp)
{
    // Для примера — случайная температура от 23 до 27 °C
    // qDebug() << "add " << curTemp;
    elapsedTime += 1.0;

    timeData.append(elapsedTime);
    tempData.append(curTemp);

    // Для красивого отображения — показываем только последние 60 точек
    if (timeData.size() > COUNT_POINTS) {
        timeData.removeFirst();
        tempData.removeFirst();
        plot->setAxisScale(QwtPlot::xBottom, timeData.first(), timeData.last());
    }

    // Автоматическое масштабирование по Y
    auto minMax = std::minmax_element(tempData.begin(), tempData.end());
    double minY = *minMax.first;
    double maxY = *minMax.second;

    // Немного отступа сверху/снизу для красоты
    double margin = (maxY - minY) * 0.1;
    if (margin < 0.5) margin = 0.5; // Не слишком тонкая ось
    plot->setAxisScale(QwtPlot::yLeft, minY - margin, maxY + margin);


    curve->setSamples(timeData.data(), tempData.data(), timeData.size());
    plot->replot();
}

void MainWindow::setTemperatur()
{
    float value = 7.456f;
    uint8_t command = 0x10;

    QByteArray buf;
    buf.append(char(command));

    union {
        float f;
        uint8_t b[4];
    } converter;
    converter.f = value;

    // По байтам (обычно little-endian)
    buf.append(char(converter.b[0]));
    buf.append(char(converter.b[1]));
    buf.append(char(converter.b[2]));
    buf.append(char(converter.b[3]));

    sendToHid(buf);
}

void MainWindow::getTemperatur()
{
    uint8_t command = 0x20;
    QByteArray buf;
    buf.append(char(command));
    sendToHid(buf);
}

void MainWindow::setPID_P()
{
    float value = ui->doubleSpinPID_P->value();
    uint8_t command = 0x11;

    QByteArray buf;
    buf.append(char(command));

    union {
        float f;
        uint8_t b[4];
    } converter;
    converter.f = value;

    // По байтам (обычно little-endian)
    buf.append(char(converter.b[0]));
    buf.append(char(converter.b[1]));
    buf.append(char(converter.b[2]));
    buf.append(char(converter.b[3]));

    // qDebug() << Q_FUNC_INFO << buf.toHex();
    sendToHid(buf);
}

void MainWindow::getPID_P()
{
    uint8_t command = 0x21;
    QByteArray buf;
    buf.append(char(command));
    sendToHid(buf);
}

void MainWindow::setPID_D()
{
    float value = ui->doubleSpinPID_D->value();
    uint8_t command = 0x12;

    QByteArray buf;
    buf.append(char(command));

    union {
        float f;
        uint8_t b[4];
    } converter;
    converter.f = value;

    // По байтам (обычно little-endian)
    buf.append(char(converter.b[0]));
    buf.append(char(converter.b[1]));
    buf.append(char(converter.b[2]));
    buf.append(char(converter.b[3]));

    // qDebug() << Q_FUNC_INFO << buf.toHex();
    sendToHid(buf);
}

void MainWindow::getPID_D()
{
    uint8_t command = 0x22;
    QByteArray buf;
    buf.append(char(command));
    sendToHid(buf);
}

void MainWindow::setCompressorOnTime()
{
    uint32_t value = ui->spinTimeBaseWork->value();
    uint8_t command = 0x13;

    QByteArray buf;
    buf.append(char(command));

    union {
        uint32_t u;
        uint8_t b[4];
    } converter;
    converter.u = value;

    // По байтам (обычно little-endian)
    buf.append(char(converter.b[0]));
    buf.append(char(converter.b[1]));
    buf.append(char(converter.b[2]));
    buf.append(char(converter.b[3]));

    // qDebug() << Q_FUNC_INFO << buf.toHex();
    sendToHid(buf);
}

void MainWindow::getCompressorOnTime()
{
    uint8_t command = 0x23;
    QByteArray buf;
    buf.append(char(command));
    sendToHid(buf);
}

void MainWindow::setCycleTime()
{
    uint32_t value = ui->spinTimeCycle->value();
    uint8_t command = 0x14;

    QByteArray buf;
    buf.append(char(command));

    union {
        uint32_t u;
        uint8_t b[4];
    } converter;
    converter.u = value;

    // По байтам (обычно little-endian)
    buf.append(char(converter.b[0]));
    buf.append(char(converter.b[1]));
    buf.append(char(converter.b[2]));
    buf.append(char(converter.b[3]));

    // qDebug() << Q_FUNC_INFO << buf.toHex();
    sendToHid(buf);
}

void MainWindow::getCycleTime()
{
    uint8_t command = 0x24;
    QByteArray buf;
    buf.append(char(command));
    sendToHid(buf);
}

void MainWindow::getSetPoint()
{
    uint8_t command = 0x25;
    QByteArray buf;
    buf.append(char(command));
    sendToHid(buf);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Сначала даём знать worker-у, что пора остановиться
    if (m_hidWorker) {
        m_hidWorker->stop();         // в stop() у тебя m_running = false и hid_close
    }

    // Просим поток выйти из цикла и дожидаемся его окончания
    if (m_hidThread && m_hidThread->isRunning()) {
        m_hidThread->quit();         // пошлёт сигнал QThread::finished()
        m_hidThread->wait(2000);     // ждём до 2 секунд (или Q_UINT64_C(ULONG_MAX) для бесконечно)
    }

    // Теперь можно спокойно закрываться
    QMainWindow::closeEvent(event);
}


void MainWindow::on_btnSetPID_P_clicked()
{
    setPID_P();
}


void MainWindow::on_btnSetPID_D_clicked()
{
    setPID_D();
}


void MainWindow::on_btnSetTimeBaseWork_clicked()
{
    setCompressorOnTime();
}

