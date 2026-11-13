#include "HuffmanWorker.h"
#include "Compression.h"
#include "Decompression.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QLocale>
#include <QtMath>
#include <algorithm>
#include <string>

QString formatByteSize(qint64 bytes)
{
    const QLocale locale;
    if (bytes < 1024)
    {
        return QStringLiteral("%1 B").arg(locale.toString(bytes));
    }

    const double kb = bytes / 1024.0;
    if (kb < 1024.0)
    {
        return QStringLiteral("%1 KB (%2 B)")
            .arg(QString::number(kb, 'f', kb < 10.0 ? 2 : 1), locale.toString(bytes));
    }

    const double mb = kb / 1024.0;
    if (mb < 1024.0)
    {
        return QStringLiteral("%1 MB (%2 B)")
            .arg(QString::number(mb, 'f', mb < 10.0 ? 2 : 1), locale.toString(bytes));
    }

    const double gb = mb / 1024.0;
    return QStringLiteral("%1 GB (%2 B)")
        .arg(QString::number(gb, 'f', 2), locale.toString(bytes));
}

QString formatDurationNs(qint64 nanoseconds)
{
    if (nanoseconds < 1000000)
    {
        return QStringLiteral("< 1 ms");
    }
    if (nanoseconds < 1000000000)
    {
        const qint64 roundedMs = qMax<qint64>(1, qRound64(nanoseconds / 1000000.0));
        return QStringLiteral("%1 ms").arg(roundedMs);
    }
    return QStringLiteral("%1 s").arg(QString::number(nanoseconds / 1000000000.0, 'f', 2));
}

HuffmanWorker::HuffmanWorker(QObject* parent)
    : QObject(parent)
{
}

void HuffmanWorker::process(OperationMode mode, const QString& inputPath, const QString& outputPath)
{
    OperationResult result;
    result.mode = mode;
    result.inputPath = inputPath;
    result.outputPath = outputPath;

    const QFileInfo inputInfo(inputPath);
    result.inputBytes = inputInfo.exists() ? inputInfo.size() : 0;

    int frequencies[256] = {};
    std::string codes[256];
    std::string errorStr;

    QElapsedTimer timer;
    timer.start();

    const std::string utf8In = inputPath.toUtf8().toStdString();
    const std::string utf8Out = outputPath.toUtf8().toStdString();

    if (mode == OperationMode::Compress)
    {
        result.success = compressFile(utf8In, utf8Out, &errorStr, frequencies, codes);
    }
    else
    {
        result.success = decompressFile(utf8In, utf8Out, &errorStr, codes);
    }

    result.elapsedNs = timer.nsecsElapsed();

    if (!result.success)
    {
        result.errorMessage = errorStr.empty()
            ? QStringLiteral("Operation failed unexpectedly.")
            : QString::fromUtf8(errorStr.c_str());
        emit finished(result);
        return;
    }

    const QFileInfo outputInfo(outputPath);
    result.outputBytes = outputInfo.exists() ? outputInfo.size() : 0;

    for (int i = 0; i < 256; ++i)
    {
        if (!codes[i].empty())
        {
            CodeTableEntry entry;
            entry.byteValue = static_cast<unsigned char>(i);
            entry.frequency = frequencies[i];
            entry.code = QString::fromLatin1(codes[i].c_str());
            result.codeTable.append(entry);
        }
    }

    std::sort(result.codeTable.begin(), result.codeTable.end(), [mode](const CodeTableEntry& a, const CodeTableEntry& b) {
        if (mode == OperationMode::Compress && a.frequency != b.frequency)
        {
            return a.frequency > b.frequency;
        }
        if (a.code.size() != b.code.size())
        {
            return a.code.size() < b.code.size();
        }
        return a.byteValue < b.byteValue;
    });

    emit finished(result);
}
