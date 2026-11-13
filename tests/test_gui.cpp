#include "FileDropZone.h"
#include "HuffmanWorker.h"
#include "MainWindow.h"
#include "Theme.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <cstdlib>
#include <iostream>

#define CHECK(cond)                                                              \
    do                                                                           \
    {                                                                            \
        if (!(cond))                                                             \
        {                                                                        \
            std::cerr << "CHECK FAILED at " << __FILE__ << ":" << __LINE__       \
                      << " -> " << #cond << std::endl;                           \
            std::exit(1);                                                        \
        }                                                                        \
    } while (false)

static QByteArray readAllBytes(const QString& path)
{
    QFile f(path);
    CHECK(f.open(QIODevice::ReadOnly));
    return f.readAll();
}

static void writeBytes(const QString& path, const QByteArray& data)
{
    QFile f(path);
    CHECK(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    CHECK(f.write(data) == data.size());
    f.close();
}

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("HuffmanProjectTest"));
    app.setApplicationName(QStringLiteral("HuffmanStudioTest"));

    QTemporaryDir tmpDir;
    CHECK(tmpDir.isValid());
    const QString dir = tmpDir.path();

    // 1. Theme persistence
    {
        saveTheme(ThemePreset::Midnight);
        CHECK(loadSavedTheme() == ThemePreset::Midnight);
        saveTheme(ThemePreset::Evergreen);
        CHECK(loadSavedTheme() == ThemePreset::Evergreen);
        saveTheme(ThemePreset::Graphite);
        CHECK(loadSavedTheme() == ThemePreset::Graphite);
        std::cout << "[PASS] Theme persistence\n";
    }

    // 2. MainWindow UI refinements, validation, worker thread, overwrite safety
    {
        MainWindow win;
        win.show();

        // Title refinement: no duplicated "— Huffman"
        CHECK(win.windowTitle() == QStringLiteral("Huffman — File Compression Studio"));
        std::cout << "[PASS] Window title verified without duplication\n";

        auto* dropZone = win.findChild<FileDropZone*>();
        auto* actionBtn = win.findChild<QPushButton*>(QStringLiteral("PrimaryButton"));
        auto* outputDirEdit = win.findChildren<QLineEdit*>().at(0);
        auto* outputNameEdit = win.findChildren<QLineEdit*>().at(1);
        auto* resultsPanel = win.findChild<QFrame*>(QStringLiteral("ResultsPanel"));
        auto* codeToggle = win.findChild<QPushButton*>(QStringLiteral("DisclosureButton"));
        auto* compressBtn = win.findChildren<QPushButton*>(QStringLiteral("SegmentButton")).at(0);
        auto* decompressBtn = win.findChildren<QPushButton*>(QStringLiteral("SegmentButton")).at(1);
        auto* errorBanner = win.findChild<QFrame*>(QStringLiteral("ErrorBanner"));
        auto* errorBannerLabel = win.findChild<QLabel*>(QStringLiteral("ErrorBannerText"));

        QLabel* outputHintLabel = nullptr;
        for (QLabel* lbl : win.findChild<QFrame*>(QStringLiteral("SectionSurface"))->findChildren<QLabel*>())
        {
            if (lbl->wordWrap())
            {
                outputHintLabel = lbl;
                break;
            }
        }

        CHECK(dropZone && actionBtn && outputDirEdit && outputNameEdit && resultsPanel && codeToggle &&
              compressBtn && decompressBtn && errorBanner && errorBannerLabel && outputHintLabel);
        CHECK(!actionBtn->isEnabled());
        CHECK(!resultsPanel->isVisible());

        // 2a. Independent Compress / Decompress session states
        {
            const QString customOutDir = dir + QStringLiteral("/custom_out");
            CHECK(QDir().mkpath(customOutDir));

            const QString assignTxt = dir + QStringLiteral("/assignment.txt");
            const QString assignHuff = dir + QStringLiteral("/assignment.txt.huff");
            const QString otherHuff = dir + QStringLiteral("/other.huff");
            writeBytes(assignTxt, QByteArray("Assignment sample content"));
            writeBytes(assignHuff, QByteArray("stub_compressed_1"));
            writeBytes(otherHuff, QByteArray("stub_compressed_2"));

            // Compress starts empty; select assignment.txt and customize its output dir & filename
            CHECK(!dropZone->hasFile());
            dropZone->setFilePath(assignTxt);
            CHECK(outputNameEdit->text() == QStringLiteral("assignment.txt.huff"));
            outputDirEdit->setText(QDir::toNativeSeparators(customOutDir));
            emit outputDirEdit->textEdited(outputDirEdit->text());
            outputNameEdit->setText(QStringLiteral("assignment_custom.huff"));
            emit outputNameEdit->textEdited(QStringLiteral("assignment_custom.huff"));

            // Switch to Decompress: must start empty with no Compress paths/warnings/results
            QTest::mouseClick(decompressBtn, Qt::LeftButton);
            CHECK(!dropZone->hasFile());
            CHECK(outputDirEdit->text().isEmpty());
            CHECK(outputNameEdit->text().isEmpty());
            CHECK(!actionBtn->isEnabled());
            CHECK(!resultsPanel->isVisible());
            CHECK(!errorBanner->isVisible());
            CHECK(!outputHintLabel->isVisible());

            // Select assignment.txt.huff in Decompress mode
            dropZone->setFilePath(assignHuff);
            CHECK(outputNameEdit->text() == QStringLiteral("assignment.txt"));
            CHECK(actionBtn->isEnabled());

            // Switch back to Compress: assignment.txt & custom output settings must be restored
            QTest::mouseClick(compressBtn, Qt::LeftButton);
            CHECK(dropZone->filePath() == assignTxt);
            CHECK(outputDirEdit->text() == QDir::toNativeSeparators(customOutDir));
            CHECK(outputNameEdit->text() == QStringLiteral("assignment_custom.huff"));
            CHECK(actionBtn->isEnabled());

            // Switch to Decompress again: assignment.txt.huff & its output settings restored
            QTest::mouseClick(decompressBtn, Qt::LeftButton);
            CHECK(dropZone->filePath() == assignHuff);
            CHECK(outputDirEdit->text() == QDir::toNativeSeparators(dir));
            CHECK(outputNameEdit->text() == QStringLiteral("assignment.txt"));

            // Replace Decompress file -> updates only Decompress state; Compress remains intact
            dropZone->setFilePath(otherHuff);
            CHECK(outputNameEdit->text() == QStringLiteral("other"));
            QTest::mouseClick(compressBtn, Qt::LeftButton);
            CHECK(dropZone->filePath() == assignTxt);
            CHECK(outputDirEdit->text() == QDir::toNativeSeparators(customOutDir));
            CHECK(outputNameEdit->text() == QStringLiteral("assignment_custom.huff"));

            // Clear Compress input without affecting Decompress state
            dropZone->clearFile();
            CHECK(!dropZone->hasFile());
            QTest::mouseClick(decompressBtn, Qt::LeftButton);
            CHECK(dropZone->filePath() == otherHuff);
            CHECK(outputNameEdit->text() == QStringLiteral("other"));

            // Return to Compress and clear custom dir/name for remaining tests
            QTest::mouseClick(compressBtn, Qt::LeftButton);
            outputDirEdit->clear();
            emit outputDirEdit->textEdited(QString());
            outputNameEdit->clear();
            emit outputNameEdit->textEdited(QString());
            std::cout << "[PASS] Independent Compress and Decompress UI states\n";
        }

        // Fast small file -> verify < 1 ms formatting, negative savings, and NO stale existing-file warning after completion
        const QString tinyIn = dir + QStringLiteral("/tiny file.txt");
        writeBytes(tinyIn, QByteArray("ABCD"));
        dropZone->setFilePath(tinyIn);
        CHECK(outputNameEdit->text() == QStringLiteral("tiny file.txt.huff"));
        CHECK(actionBtn->isEnabled());
        CHECK(!outputHintLabel->isVisible());

        QTest::mouseClick(actionBtn, Qt::LeftButton);
        QElapsedTimer waitTimer;
        waitTimer.start();
        while (!resultsPanel->isVisible() && waitTimer.elapsed() < 5000)
        {
            QCoreApplication::processEvents();
            QTest::qWait(10);
        }
        CHECK(resultsPanel->isVisible());

        // Refinement A: warning must NOT be visible immediately after generating output
        CHECK(!outputHintLabel->isVisible());

        // Refinement B & negative savings check
        bool foundSubMs = false;
        bool foundLarger = false;
        for (QLabel* lbl : resultsPanel->findChildren<QLabel*>())
        {
            if (lbl->text() == QStringLiteral("< 1 ms"))
            {
                foundSubMs = true;
            }
            if (lbl->text().contains(QStringLiteral("larger")))
            {
                foundLarger = true;
            }
        }
        CHECK(foundSubMs);
        CHECK(foundLarger);
        std::cout << "[PASS] Sub-millisecond timing (< 1 ms), negative savings, and post-completion warning cleared\n";

        // Editing output name to another existing file restores the overwrite warning
        outputNameEdit->setText(QStringLiteral("tiny file.txt"));
        emit outputNameEdit->textEdited(QStringLiteral("tiny file.txt"));
        CHECK(!actionBtn->isEnabled());

        const QString preExisting = dir + QStringLiteral("/existing_target.huff");
        writeBytes(preExisting, QByteArray("KEEP_ME"));
        outputNameEdit->setText(QStringLiteral("existing_target.huff"));
        emit outputNameEdit->textEdited(QStringLiteral("existing_target.huff"));
        CHECK(outputHintLabel->isVisible());
        CHECK(outputHintLabel->text().contains(QStringLiteral("already exists")));
        CHECK(actionBtn->isEnabled());

        // Overwrite confirmation: Cancel keeps existing file untouched
        QTimer::singleShot(50, [&]() {
            if (auto* msg = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
            {
                msg->reject();
            }
        });
        QTest::mouseClick(actionBtn, Qt::LeftButton);
        CHECK(readAllBytes(preExisting) == QByteArray("KEEP_ME"));
        std::cout << "[PASS] Overwrite warning restored on edit and Cancel preserves existing file\n";

        // Larger 3 MB file responsiveness + round-trip decompression + code table
        QByteArray bigData(3 * 1024 * 1024, '\0');
        for (int i = 0; i < bigData.size(); ++i)
        {
            bigData[i] = static_cast<char>('A' + (i % 13));
        }
        const QString bigIn = dir + QStringLiteral("/large sample.txt");
        writeBytes(bigIn, bigData);

        dropZone->setFilePath(bigIn);
        QTest::mouseClick(actionBtn, Qt::LeftButton);

        int ticks = 0;
        waitTimer.restart();
        while (!resultsPanel->isVisible() && waitTimer.elapsed() < 15000)
        {
            QCoreApplication::processEvents();
            ++ticks;
            QTest::qWait(5);
        }
        CHECK(resultsPanel->isVisible());
        CHECK(ticks >= 1);

        auto* codeTable = win.findChild<QTableWidget*>();
        CHECK(codeTable && codeToggle->isVisible());
        QTest::mouseClick(codeToggle, Qt::LeftButton);
        CHECK(codeTable->isVisible());
        CHECK(codeTable->rowCount() == 13);

        // Switch to Decompress mode & restore
        QTest::mouseClick(decompressBtn, Qt::LeftButton);
        dropZone->setFilePath(dir + QStringLiteral("/large sample.txt.huff"));
        outputNameEdit->setText(QStringLiteral("large sample restored.txt"));
        emit outputNameEdit->textEdited(QStringLiteral("large sample restored.txt"));
        QTest::mouseClick(actionBtn, Qt::LeftButton);

        waitTimer.restart();
        while (!resultsPanel->isVisible() && waitTimer.elapsed() < 15000)
        {
            QCoreApplication::processEvents();
            QTest::qWait(5);
        }
        CHECK(resultsPanel->isVisible());
        CHECK(readAllBytes(dir + QStringLiteral("/large sample restored.txt")) == bigData);
        std::cout << "[PASS] Background responsiveness, code table, and 3 MB decompression round-trip\n";

        // Invalid compressed input in GUI -> ErrorBanner shown, destination NOT created
        const QString bogus = dir + QStringLiteral("/bogus.huff");
        const QString bogusOut = dir + QStringLiteral("/bogus_should_not_exist.txt");
        writeBytes(bogus, QByteArray("Plain text, invalid Huffman file."));
        dropZone->setFilePath(bogus);
        outputNameEdit->setText(QStringLiteral("bogus_should_not_exist.txt"));
        emit outputNameEdit->textEdited(QStringLiteral("bogus_should_not_exist.txt"));
        QTest::mouseClick(actionBtn, Qt::LeftButton);

        waitTimer.restart();
        while (!errorBanner->isVisible() && waitTimer.elapsed() < 5000)
        {
            QCoreApplication::processEvents();
            QTest::qWait(10);
        }
        CHECK(errorBanner->isVisible());
        CHECK(!resultsPanel->isVisible());
        CHECK(!errorBannerLabel->text().isEmpty());
        CHECK(!QFile::exists(bogusOut));

        // Switching to Compress mode clears stale Decompress error banner
        QTest::mouseClick(compressBtn, Qt::LeftButton);
        CHECK(!errorBanner->isVisible());
        CHECK(!resultsPanel->isVisible());
        std::cout << "[PASS] Invalid compressed file shows ErrorBanner, leaves no output file, and clears on mode switch\n";
    }

    std::cout << "ALL GUI TESTS PASSED\n";
    return 0;
}
