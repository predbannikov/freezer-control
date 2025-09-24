#include "TemperatureLogger.h"
#include <QTimer>
#include <QFile>
#include <QSaveFile>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

TemperatureLogger::TemperatureLogger(QObject* parent)
    : QObject(parent),
    timer_(new QTimer(this))
{
    connect(timer_, &QTimer::timeout, this, &TemperatureLogger::onTick);
    timer_->setTimerType(Qt::CoarseTimer);
}

void TemperatureLogger::setTemperatureProvider(double provider) {
    temperatureProvider_ = std::move(provider);
}

void TemperatureLogger::setLogFilePath(const QString& path) {
    logPath_ = path;
}

QString TemperatureLogger::logFilePath() const { return logPath_; }

void TemperatureLogger::setMaxBytes(qint64 bytes) {
    maxBytes_ = bytes > 0 ? bytes : 1;
}

qint64 TemperatureLogger::maxBytes() const { return maxBytes_; }

void TemperatureLogger::setIntervalMs(int intervalMs) {
    intervalMs_ = intervalMs > 0 ? intervalMs : 1000;
    if (running_) timer_->start(intervalMs_);
}

int TemperatureLogger::intervalMs() const { return intervalMs_; }

void TemperatureLogger::setMaxRotatedFiles(int count) {
    keepFiles_ = (count >= 0) ? count : 0;
}
int TemperatureLogger::maxRotatedFiles() const { return keepFiles_; }

void TemperatureLogger::start() {
    if (!temperatureProvider_) {
        qWarning("TemperatureLogger: temperature provider is not set.");
        return;
    }
    if (running_) return;
    running_ = true;
    timer_->start(intervalMs_);
}

void TemperatureLogger::stop() {
    if (!running_) return;
    running_ = false;
    timer_->stop();
}

void TemperatureLogger::onTick() {
    const QString ts = nowTimestamp();
    const double t = temperatureProvider_ ? temperatureProvider_ : qQNaN();
    appendLine(formatCsvLine(ts, t));
    rotateIfNeeded();
}

void TemperatureLogger::appendLine(const QString& line) {
    QFile f(logPath_);
    // гарантируем директорию
    const QFileInfo fi(logPath_);
    if (!fi.absoluteDir().exists()) {
        QDir().mkpath(fi.absolutePath());
    }
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning("TemperatureLogger: cannot open log file for append.");
        return;
    }
    QByteArray bytes = line.toUtf8();
    bytes.append('\n');
    if (f.write(bytes) != bytes.size()) {
        qWarning("TemperatureLogger: failed to write log line.");
    }
    f.flush();
    f.close();
}

void TemperatureLogger::rotateIfNeeded() {
    QFile f(logPath_);
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly)) return;
    const qint64 sz = f.size();
    f.close();
    if (sz >= maxBytes_) {
        rotateLogFile();
        pruneOldRotatedFiles();
    }
}

void TemperatureLogger::rotateLogFile() {
    QFileInfo info(logPath_);
    QDir dir = info.absoluteDir();
    const QString baseName = info.completeBaseName(); // "temperature_log"
    const QString suffix   = info.suffix();           // "csv"

    QString rotatedName = QString("%1_%2.%3")
                              .arg(baseName, tsForFilename(), suffix);
    QString rotatedPath = dir.absoluteFilePath(rotatedName);

    // на всякий случай, если совпало имя — добавим счётчик
    int counter = 1;
    while (QFile::exists(rotatedPath)) {
        rotatedName = QString("%1_%2_%3.%4")
        .arg(baseName, tsForFilename())
            .arg(counter++)
            .arg(suffix);
        rotatedPath = dir.absoluteFilePath(rotatedName);
    }

    // Закрыт ли файл — мы открываем его на запись только построчно, так что сейчас закрыт.
    if (!QFile::rename(logPath_, rotatedPath)) {
        // fallback: копия+очистка
        if (!QFile::copy(logPath_, rotatedPath)) {
            qWarning("TemperatureLogger: rotation copy failed.");
            return;
        }
        QFile orig(logPath_);
        if (!orig.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("TemperatureLogger: cannot truncate original after copy.");
        }
        orig.close();
        return;
    }

    // создаём новый пустой лог
    QFile newLog(logPath_);
    if (!newLog.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning("TemperatureLogger: cannot create new log after rotation.");
    }
    newLog.close();
}

void TemperatureLogger::pruneOldRotatedFiles() {
    QFileInfo info(logPath_);
    QDir dir = info.absoluteDir();

    const QString baseName = info.completeBaseName(); // "temperature_log"
    const QString suffix   = info.suffix();           // "csv"

    // ищем только наши ротации: temperature_log_*.csv (не трогаем текущий logPath_)
    const QString pattern = QString("%1_*.%2").arg(baseName, suffix);
    QFileInfoList list = dir.entryInfoList({pattern}, QDir::Files, QDir::Time); // по времени, новые сначала

    if (keepFiles_ <= 0) {
        // удалить все ротации
        for (const QFileInfo& fi : list) {
            QFile::remove(fi.absoluteFilePath());
        }
        return;
    }

    for (int i = keepFiles_; i < list.size(); ++i) {
        QFile::remove(list[i].absoluteFilePath());
    }
}

QString TemperatureLogger::nowTimestamp() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}
QString TemperatureLogger::tsForFilename() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
}

QString TemperatureLogger::formatCsvLine(const QString& timestamp, double temperature) {
    return QString("%1,%2").arg(timestamp, QString::number(temperature, 'f', 2));
}
