#ifndef HIDWORKER_H
#define HIDWORKER_H

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <hidapi.h>


class HidWorker : public QObject {
    Q_OBJECT
public:
    explicit HidWorker(uint16_t vid, uint16_t pid, QObject* parent = nullptr);
    ~HidWorker();

public slots:
    void start();              // открыть и запустить цикл
    void stop();               // остановить цикл и закрыть устройство
    void sendData(const QByteArray &data);  // слот для отправки в устройство

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &msg);
    void finished();

private:
    void loop();  // основной цикл чтения

    QMutex      m_mutex;
    QWaitCondition m_wait;
    bool        m_running = false;

    hid_device* m_handle = nullptr;
    uint16_t    m_vid;
    uint16_t    m_pid;

    QList<QByteArray> m_outQueue;
};

#endif // HIDWORKER_H
