#pragma once

#include "HuffmanWorker.h"
#include "Theme.h"

#include <QFrame>
#include <QString>

class QLabel;
class QMimeData;
class QPushButton;
class QStackedLayout;

class FileDropZone : public QFrame
{
    Q_OBJECT

public:
    explicit FileDropZone(QWidget* parent = nullptr);

    void setMode(OperationMode mode);
    void setFilePath(const QString& path);
    void clearFile();
    void applyTheme(const ThemePalette& palette);
    void setBusy(bool busy);

    QString filePath() const;
    bool hasFile() const;

signals:
    void fileSelected(const QString& path);
    void fileCleared();
    void fileRejected(const QString& reason);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void browseForFile();

private:
    void setDragActive(bool active);
    void refreshState();
    void updateElidedFolder();
    QString extractLocalFile(const QMimeData* mimeData, QString* rejectReason) const;

    OperationMode m_mode = OperationMode::Compress;
    QString m_filePath;
    QString m_folderPath;
    bool m_busy = false;
    bool m_dragActive = false;

    QStackedLayout* m_stack = nullptr;

    // Empty state widgets
    QLabel* m_emptyIconLabel = nullptr;
    QLabel* m_emptyTitleLabel = nullptr;
    QLabel* m_emptyHintLabel = nullptr;

    // Loaded state widgets
    QLabel* m_fileIconLabel = nullptr;
    QLabel* m_fileNameLabel = nullptr;
    QLabel* m_fileFolderLabel = nullptr;
    QLabel* m_fileSizeBadge = nullptr;
    QPushButton* m_replaceButton = nullptr;
    QPushButton* m_removeButton = nullptr;
};
