#include "FileDropZone.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QStackedLayout>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

FileDropZone::FileDropZone(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("DropZone"));
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(96);

    m_stack = new QStackedLayout(this);
    m_stack->setContentsMargins(16, 14, 16, 14);

    // Page 0: Empty dropzone state
    auto* emptyPage = new QWidget(this);
    auto* emptyLayout = new QHBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(4, 4, 4, 4);
    emptyLayout->setSpacing(14);

    m_emptyIconLabel = new QLabel(emptyPage);
    m_emptyIconLabel->setFixedSize(36, 36);
    m_emptyIconLabel->setAlignment(Qt::AlignCenter);

    auto* emptyTextLayout = new QVBoxLayout();
    emptyTextLayout->setContentsMargins(0, 0, 0, 0);
    emptyTextLayout->setSpacing(3);

    m_emptyTitleLabel = new QLabel(QStringLiteral("Drop a file here, or click to browse"), emptyPage);
    m_emptyTitleLabel->setObjectName(QStringLiteral("DropTitle"));

    m_emptyHintLabel = new QLabel(emptyPage);
    m_emptyHintLabel->setObjectName(QStringLiteral("DropSubtitle"));

    emptyTextLayout->addStretch();
    emptyTextLayout->addWidget(m_emptyTitleLabel);
    emptyTextLayout->addWidget(m_emptyHintLabel);
    emptyTextLayout->addStretch();

    emptyLayout->addWidget(m_emptyIconLabel);
    emptyLayout->addLayout(emptyTextLayout, 1);

    // Page 1: Selected file state
    auto* loadedPage = new QWidget(this);
    auto* loadedLayout = new QHBoxLayout(loadedPage);
    loadedLayout->setContentsMargins(4, 2, 4, 2);
    loadedLayout->setSpacing(12);

    m_fileIconLabel = new QLabel(loadedPage);
    m_fileIconLabel->setFixedSize(28, 28);
    m_fileIconLabel->setAlignment(Qt::AlignCenter);

    auto* fileInfoLayout = new QVBoxLayout();
    fileInfoLayout->setContentsMargins(0, 0, 0, 0);
    fileInfoLayout->setSpacing(4);

    auto* nameRow = new QHBoxLayout();
    nameRow->setContentsMargins(0, 0, 0, 0);
    nameRow->setSpacing(8);

    m_fileNameLabel = new QLabel(loadedPage);
    m_fileNameLabel->setObjectName(QStringLiteral("DropTitle"));
    m_fileNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_fileSizeBadge = new QLabel(loadedPage);
    m_fileSizeBadge->setObjectName(QStringLiteral("FileSizeBadge"));
    m_fileSizeBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    nameRow->addWidget(m_fileNameLabel);
    nameRow->addWidget(m_fileSizeBadge);
    nameRow->addStretch();

    m_fileFolderLabel = new QLabel(loadedPage);
    m_fileFolderLabel->setObjectName(QStringLiteral("DropSubtitle"));
    m_fileFolderLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    fileInfoLayout->addStretch();
    fileInfoLayout->addLayout(nameRow);
    fileInfoLayout->addWidget(m_fileFolderLabel);
    fileInfoLayout->addStretch();

    auto* actionsLayout = new QHBoxLayout();
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(4);

    m_replaceButton = new QPushButton(QStringLiteral("Replace"), loadedPage);
    m_replaceButton->setObjectName(QStringLiteral("SubtleIconButton"));
    m_replaceButton->setCursor(Qt::PointingHandCursor);
    m_replaceButton->setToolTip(QStringLiteral("Choose a different input file"));

    m_removeButton = new QPushButton(loadedPage);
    m_removeButton->setObjectName(QStringLiteral("SubtleIconButton"));
    m_removeButton->setCursor(Qt::PointingHandCursor);
    m_removeButton->setToolTip(QStringLiteral("Clear selected file"));
    m_removeButton->setFixedSize(28, 28);

    actionsLayout->addWidget(m_replaceButton);
    actionsLayout->addWidget(m_removeButton);

    loadedLayout->addWidget(m_fileIconLabel);
    loadedLayout->addLayout(fileInfoLayout, 1);
    loadedLayout->addLayout(actionsLayout);

    m_stack->addWidget(emptyPage);
    m_stack->addWidget(loadedPage);

    connect(m_replaceButton, &QPushButton::clicked, this, &FileDropZone::browseForFile);
    connect(m_removeButton, &QPushButton::clicked, this, &FileDropZone::clearFile);

    setMode(OperationMode::Compress);
    refreshState();
}

void FileDropZone::setMode(OperationMode mode)
{
    m_mode = mode;
    if (m_mode == OperationMode::Compress)
    {
        m_emptyHintLabel->setText(QStringLiteral("Supports text and binary files of any extension"));
    }
    else
    {
        m_emptyHintLabel->setText(QStringLiteral("Select a Huffman-compressed file (.huff or raw compressed file)"));
    }
}

void FileDropZone::setFilePath(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile())
    {
        emit fileRejected(QStringLiteral("The selected item is not a valid file."));
        return;
    }

    m_filePath = info.absoluteFilePath();
    m_folderPath = QDir::toNativeSeparators(info.absolutePath());
    m_fileNameLabel->setText(info.fileName());
    m_fileNameLabel->setToolTip(QDir::toNativeSeparators(m_filePath));
    m_fileFolderLabel->setToolTip(m_folderPath);
    m_fileSizeBadge->setText(formatByteSize(info.size()));

    refreshState();
    updateElidedFolder();
    emit fileSelected(m_filePath);
}

void FileDropZone::clearFile()
{
    if (m_filePath.isEmpty())
    {
        return;
    }

    m_filePath.clear();
    m_folderPath.clear();
    m_fileNameLabel->clear();
    m_fileFolderLabel->clear();
    m_fileSizeBadge->clear();

    refreshState();
    emit fileCleared();
}

void FileDropZone::applyTheme(const ThemePalette& palette)
{
    m_emptyIconLabel->setPixmap(createUploadIcon(QColor(palette.accent)).pixmap(28, 28));
    m_fileIconLabel->setPixmap(createFileIcon(QColor(palette.accent)).pixmap(22, 22));
    m_removeButton->setIcon(createCloseIcon(QColor(palette.textSecondary)));
}

void FileDropZone::setBusy(bool busy)
{
    m_busy = busy;
    setAcceptDrops(!busy);
    m_replaceButton->setEnabled(!busy);
    m_removeButton->setEnabled(!busy);
    setCursor(busy ? Qt::ArrowCursor : (m_filePath.isEmpty() ? Qt::PointingHandCursor : Qt::ArrowCursor));
}

QString FileDropZone::filePath() const
{
    return m_filePath;
}

bool FileDropZone::hasFile() const
{
    return !m_filePath.isEmpty();
}

void FileDropZone::browseForFile()
{
    if (m_busy)
    {
        return;
    }

    const QString title = (m_mode == OperationMode::Compress)
        ? QStringLiteral("Select File to Compress")
        : QStringLiteral("Select Compressed File to Decompress");

    const QString filter = (m_mode == OperationMode::Compress)
        ? QStringLiteral("All Files (*)")
        : QStringLiteral("All Files (*);;Huffman Compressed (*.huff)");

    QString startDir = m_folderPath;
    if (startDir.isEmpty())
    {
        startDir = QDir::homePath();
    }

    const QString chosen = QFileDialog::getOpenFileName(this, title, startDir, filter);
    if (!chosen.isEmpty())
    {
        setFilePath(chosen);
    }
}

void FileDropZone::dragEnterEvent(QDragEnterEvent* event)
{
    if (m_busy)
    {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasUrls())
    {
        QString reason;
        if (!extractLocalFile(event->mimeData(), &reason).isEmpty())
        {
            event->acceptProposedAction();
            setDragActive(true);
            return;
        }
    }
    event->ignore();
}

void FileDropZone::dragMoveEvent(QDragMoveEvent* event)
{
    if (!m_busy && event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void FileDropZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event);
    setDragActive(false);
}

void FileDropZone::dropEvent(QDropEvent* event)
{
    setDragActive(false);
    if (m_busy)
    {
        event->ignore();
        return;
    }

    QString reason;
    const QString localFile = extractLocalFile(event->mimeData(), &reason);
    if (localFile.isEmpty())
    {
        if (!reason.isEmpty())
        {
            emit fileRejected(reason);
        }
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    setFilePath(localFile);
}

void FileDropZone::mousePressEvent(QMouseEvent* event)
{
    if (!m_busy && event->button() == Qt::LeftButton && m_filePath.isEmpty())
    {
        browseForFile();
        return;
    }
    QFrame::mousePressEvent(event);
}

void FileDropZone::keyPressEvent(QKeyEvent* event)
{
    if (!m_busy && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space))
    {
        browseForFile();
        return;
    }
    QFrame::keyPressEvent(event);
}

void FileDropZone::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    updateElidedFolder();
}

void FileDropZone::setDragActive(bool active)
{
    if (m_dragActive == active)
    {
        return;
    }
    m_dragActive = active;
    setProperty("dragActive", active);
    style()->unpolish(this);
    style()->polish(this);
}

void FileDropZone::refreshState()
{
    const bool loaded = !m_filePath.isEmpty();
    m_stack->setCurrentIndex(loaded ? 1 : 0);
    setCursor(m_busy ? Qt::ArrowCursor : (loaded ? Qt::ArrowCursor : Qt::PointingHandCursor));
    setProperty("hasFile", loaded);
    style()->unpolish(this);
    style()->polish(this);
}

void FileDropZone::updateElidedFolder()
{
    if (m_folderPath.isEmpty())
    {
        return;
    }
    const QFontMetrics fm(m_fileFolderLabel->font());
    const int avail = qMax(120, m_fileFolderLabel->width());
    m_fileFolderLabel->setText(fm.elidedText(m_folderPath, Qt::ElideMiddle, avail));
}

QString FileDropZone::extractLocalFile(const QMimeData* mimeData, QString* rejectReason) const
{
    if (!mimeData || !mimeData->hasUrls())
    {
        return {};
    }

    const QList<QUrl> urls = mimeData->urls();
    if (urls.isEmpty())
    {
        return {};
    }

    const QUrl& first = urls.first();
    if (!first.isLocalFile())
    {
        if (rejectReason)
            *rejectReason = QStringLiteral("Only local files can be dropped here.");
        return {};
    }

    const QFileInfo info(first.toLocalFile());
    if (!info.exists() || !info.isFile())
    {
        if (rejectReason)
            *rejectReason = QStringLiteral("Folders are not supported. Please drop a single file.");
        return {};
    }

    return info.absoluteFilePath();
}
