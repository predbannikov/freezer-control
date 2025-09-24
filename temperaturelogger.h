#ifndef TEMPERATURELOGGER_H
#define TEMPERATURELOGGER_H

#include <QObject>
#include <QString>

class QTimer;

class TemperatureLogger : public QObject
{
    Q_OBJECT
public:
    explicit TemperatureLogger(QObject* parent = nullptr);

    void setTemperatureProvider(double provider);

    void setLogFilePath(const QString& path);
    QString logFilePath() const;

    void setMaxBytes(qint64 bytes);   // порог ротации (байт), по умолчанию 1 МБ
    qint64 maxBytes() const;

    void setIntervalMs(int intervalMs); // период, по умолчанию 15000 мс
    int intervalMs() const;

    // Сколько файлов после ротации хранить (по дате модификации, последние N)
    void setMaxRotatedFiles(int count); // по умолчанию 10
    int maxRotatedFiles() const;

public slots:
    void start();
    void stop();

private slots:
    void onTick();

private:
    void appendLine(const QString& line);
    void rotateIfNeeded();
    void rotateLogFile();          // переименовать текущий лог и создать новый
    void pruneOldRotatedFiles();   // удалить старые, оставить maxRotatedFiles

    static QString nowTimestamp(); // "YYYY-MM-DD HH:MM:SS"
    static QString tsForFilename(); // "YYYY-MM-DD_HH-mm-ss"
    static QString formatCsvLine(const QString& timestamp, double temperature);

private:
    QTimer* timer_{nullptr};
    double temperatureProvider_;
    QString logPath_ = QStringLiteral("temperature_log.csv");
    qint64  maxBytes_ = 1 * 1024 * 1024; // 1 МБ
    int     intervalMs_ = 15000;         // 15 сек
    int     keepFiles_ = 50;             // сколько ротаций хранить
    bool    running_ = false;
};

#endif // TEMPERATURELOGGER_H
