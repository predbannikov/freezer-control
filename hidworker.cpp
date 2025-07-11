#include "hidworker.h"
#include <QThread>
#include <QtConcurrent/QtConcurrent>

HidWorker::HidWorker(uint16_t vid, uint16_t pid, QObject* parent)
    : QObject(parent), m_vid(vid), m_pid(pid)
{}

HidWorker::~HidWorker() {
    stop();
}

void HidWorker::start() {
    if (hid_init() != 0) {
        qDebug() << "hid_init failed";
        emit errorOccurred("hid_init failed");
        return;
    }
    m_handle = hid_open(m_vid, m_pid, nullptr);
    if (!m_handle) {
        qDebug() << "hid_open failed";
        emit errorOccurred("hid_open failed");
        hid_exit();
        return;
    }
    // НЕ блокировать hid_read
    hid_set_nonblocking(m_handle, 1);

    m_mutex.lock();
    m_running = true;
    m_mutex.unlock();

    // Запускаем цикл в этом же потоке
    QtConcurrent::run([this]{ loop(); });
}

void HidWorker::stop() {
    m_mutex.lock();
    m_running = false;
    m_wait.wakeAll();
    m_mutex.unlock();
    if (m_handle) {
        hid_close(m_handle);
        hid_exit();
        m_handle = nullptr;
    }
    emit finished();
}

void HidWorker::sendData(const QByteArray &data) {
    QMutexLocker lock(&m_mutex);
    m_outQueue.append(data);
    // qDebug() << Q_FUNC_INFO << data.toHex();
    m_wait.wakeOne();
}

void HidWorker::loop() {

    while (true) {
        m_mutex.lock();
        if (!m_running) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();

        if (!m_handle) {
            m_handle = hid_open(m_vid, m_pid, nullptr);
            if (m_handle) {
                hid_set_nonblocking(m_handle, 1);
                qDebug() << "Device reconnected!";
                emit errorOccurred("Device reconnected!");
                attepmtReconect = 0;
            } else {
                emit errorOccurred("Device not found, reconnecting...");
                qDebug() << "Device not found, reconnecting...";
                if (++attepmtReconect > 15) {
                    QString deviceId = "USB\\VID_3210&PID_0098\\xxxxxxxx"; // подбери свой!
                    int devconResult = QProcess::execute("devcon", {"restart", deviceId});
                    qDebug() << "devcon restart result:" << devconResult;
                    QThread::msleep(2000); // Дать системе время на инициализацию
                    attepmtReconect = 0;
                }
                QThread::msleep(1000);
                continue;
            }
        }

        // есть что слать?
        m_mutex.lock();
        bool hasOut = !m_outQueue.isEmpty();
        QByteArray packet;
        if (hasOut)
            packet = m_outQueue.takeFirst();
        m_mutex.unlock();

        if (hasOut) {
            QByteArray buf;
            buf.resize(1 + packet.size());
            buf[0] = 0;
            memcpy(buf.data()+1, packet.data(), packet.size());

            int w = hid_write(m_handle, reinterpret_cast<unsigned char*>(buf.data()), buf.size());
            if (w < 0) {
                emit errorOccurred(QString("Write error: %1. Lost device?").arg(hid_error(m_handle)));
                hid_close(m_handle);
                hid_exit();
                m_handle = nullptr;
                QThread::msleep(1000);
                continue; // сразу пробовать переподключиться
            }
        } else {
            // читаем входящие (IN endpoint = 8 байт)
            unsigned char inBuf[8] = {0};
            int r = hid_read_timeout(m_handle, inBuf, sizeof(inBuf), 100);
            if (r > 0) {
                QByteArray arrived(reinterpret_cast<char*>(inBuf), r);
                emit dataReceived(arrived);
            } else if (r < 0) {
                emit errorOccurred(QString("Read error: %1. Lost device?").arg(hid_error(m_handle)));
                hid_close(m_handle);
                hid_exit();
                m_handle = nullptr;
                QThread::msleep(1000);
                continue; // переподключение
            }
            QThread::msleep(10);
        }
    }
    // При завершении clean-up
    if (m_handle) {
        hid_close(m_handle);
        hid_exit();
        m_handle = nullptr;
    }
    emit finished();
}
