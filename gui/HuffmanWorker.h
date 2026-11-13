#pragma once

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>

enum class OperationMode
{
    Compress = 0,
    Decompress = 1
};

struct CodeTableEntry
{
    unsigned char byteValue = 0;
    int frequency = 0;
    QString code;
};

struct OperationResult
{
    bool success = false;
    OperationMode mode = OperationMode::Compress;
    QString inputPath;
    QString outputPath;
    qint64 inputBytes = 0;
    qint64 outputBytes = 0;
    qint64 elapsedNs = 0;
    QString errorMessage;
    QList<CodeTableEntry> codeTable;
};

Q_DECLARE_METATYPE(OperationMode)
Q_DECLARE_METATYPE(OperationResult)

QString formatByteSize(qint64 bytes);
QString formatDurationNs(qint64 nanoseconds);

class HuffmanWorker : public QObject
{
    Q_OBJECT

public:
    explicit HuffmanWorker(QObject* parent = nullptr);

public slots:
    void process(OperationMode mode, const QString& inputPath, const QString& outputPath);

signals:
    void finished(const OperationResult& result);
};
