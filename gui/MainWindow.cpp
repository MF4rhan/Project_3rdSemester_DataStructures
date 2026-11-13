#include "MainWindow.h"
#include "FileDropZone.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

namespace {
    QString formatSymbolDisplay(unsigned char byteVal)
    {
        switch (byteVal)
        {
        case '\n': return QStringLiteral("\\n (LF)");
        case '\r': return QStringLiteral("\\r (CR)");
        case '\t': return QStringLiteral("\\t (TAB)");
        case ' ':  return QStringLiteral("SPACE");
        case '\0': return QStringLiteral("\\0 (NUL)");
        default:
            if (byteVal >= 33 && byteVal <= 126)
            {
                return QStringLiteral("'%1'").arg(QChar(byteVal));
            }
            return QStringLiteral("<0x%1>").arg(byteVal, 2, 16, QLatin1Char('0')).toUpper();
        }
    }

    QString formatHexByte(unsigned char byteVal)
    {
        return QStringLiteral("0x%1").arg(byteVal, 2, 16, QLatin1Char('0')).toUpper();
    }

    QString buildSuggestedFilename(const QFileInfo& inputInfo, OperationMode mode)
    {
        const QString name = inputInfo.fileName();
        if (mode == OperationMode::Compress)
        {
            return name + QStringLiteral(".huff");
        }

        if (name.endsWith(QStringLiteral(".huff"), Qt::CaseInsensitive) && name.size() > 5)
        {
            return name.left(name.size() - 5);
        }

        const QString base = inputInfo.completeBaseName();
        const QString suffix = inputInfo.suffix();
        if (!suffix.isEmpty() && !base.isEmpty())
        {
            return base + QStringLiteral("_decompressed.") + suffix;
        }
        return name + QStringLiteral("_decompressed");
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    qRegisterMetaType<OperationMode>("OperationMode");
    qRegisterMetaType<OperationResult>("OperationResult");

    setWindowTitle(QStringLiteral("Huffman — File Compression Studio"));
    setMinimumSize(560, 580);
    resize(660, 720);

    m_workerThread = new QThread(this);
    m_worker = new HuffmanWorker();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MainWindow::requestOperation, m_worker, &HuffmanWorker::process);
    connect(m_worker, &HuffmanWorker::finished, this, &MainWindow::onOperationFinished);
    m_workerThread->start();

    setupUi();

    const ThemePreset saved = loadSavedTheme();
    m_themeCombo->blockSignals(true);
    m_themeCombo->setCurrentIndex(static_cast<int>(saved));
    m_themeCombo->blockSignals(false);
    applyTheme(saved);

    updateModeUi();
    validateInputs();
}

MainWindow::~MainWindow()
{
    if (m_workerThread)
    {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_busy)
    {
        QMessageBox::information(
            this,
            QStringLiteral("Operation in Progress"),
            QStringLiteral("Please wait for the current file operation to finish before closing."));
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    auto* root = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header bar
    auto* headerBar = new QFrame(root);
    headerBar->setObjectName(QStringLiteral("HeaderBar"));
    auto* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(20, 12, 20, 12);
    headerLayout->setSpacing(12);

    m_logoLabel = new QLabel(headerBar);
    m_logoLabel->setFixedSize(28, 28);

    auto* titleColumn = new QVBoxLayout();
    titleColumn->setContentsMargins(0, 0, 0, 0);
    titleColumn->setSpacing(1);

    auto* appTitle = new QLabel(QStringLiteral("Huffman"), headerBar);
    appTitle->setObjectName(QStringLiteral("AppTitle"));
    auto* appSubtitle = new QLabel(QStringLiteral("File Compression Studio"), headerBar);
    appSubtitle->setObjectName(QStringLiteral("AppSubtitle"));

    titleColumn->addWidget(appTitle);
    titleColumn->addWidget(appSubtitle);

    auto* themeLabel = new QLabel(QStringLiteral("Theme"), headerBar);
    themeLabel->setObjectName(QStringLiteral("MutedLabel"));

    m_themeCombo = new QComboBox(headerBar);
    m_themeCombo->addItem(QStringLiteral("Graphite"));
    m_themeCombo->addItem(QStringLiteral("Midnight"));
    m_themeCombo->addItem(QStringLiteral("Evergreen"));
    m_themeCombo->setToolTip(QStringLiteral("Switch color preset"));
    m_themeCombo->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_logoLabel);
    headerLayout->addLayout(titleColumn);
    headerLayout->addStretch();
    headerLayout->addWidget(themeLabel);
    headerLayout->addWidget(m_themeCombo);

    rootLayout->addWidget(headerBar);

    // Scrollable main content area
    auto* scrollArea = new QScrollArea(root);
    scrollArea->setObjectName(QStringLiteral("MainScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollViewportWidget = new QWidget();
    scrollViewportWidget->setObjectName(QStringLiteral("ScrollContent"));
    auto* centerWrapper = new QHBoxLayout(scrollViewportWidget);
    centerWrapper->setContentsMargins(20, 18, 20, 22);

    auto* contentColumn = new QWidget(scrollViewportWidget);
    contentColumn->setMaximumWidth(740);
    contentColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* contentLayout = new QVBoxLayout(contentColumn);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);

    // Segmented mode control
    auto* segmentedContainer = new QFrame(contentColumn);
    segmentedContainer->setObjectName(QStringLiteral("SegmentedContainer"));
    auto* segmentedLayout = new QHBoxLayout(segmentedContainer);
    segmentedLayout->setContentsMargins(4, 4, 4, 4);
    segmentedLayout->setSpacing(4);

    m_compressModeBtn = new QPushButton(QStringLiteral("Compress"), segmentedContainer);
    m_compressModeBtn->setObjectName(QStringLiteral("SegmentButton"));
    m_compressModeBtn->setCheckable(true);
    m_compressModeBtn->setChecked(true);
    m_compressModeBtn->setCursor(Qt::PointingHandCursor);

    m_decompressModeBtn = new QPushButton(QStringLiteral("Decompress"), segmentedContainer);
    m_decompressModeBtn->setObjectName(QStringLiteral("SegmentButton"));
    m_decompressModeBtn->setCheckable(true);
    m_decompressModeBtn->setCursor(Qt::PointingHandCursor);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);
    m_modeGroup->addButton(m_compressModeBtn, static_cast<int>(OperationMode::Compress));
    m_modeGroup->addButton(m_decompressModeBtn, static_cast<int>(OperationMode::Decompress));

    segmentedLayout->addWidget(m_compressModeBtn, 1);
    segmentedLayout->addWidget(m_decompressModeBtn, 1);
    contentLayout->addWidget(segmentedContainer);

    // Input file section
    auto* inputSectionLayout = new QVBoxLayout();
    inputSectionLayout->setContentsMargins(0, 2, 0, 0);
    inputSectionLayout->setSpacing(6);

    m_inputSectionLabel = new QLabel(QStringLiteral("Source File"), contentColumn);
    m_inputSectionLabel->setObjectName(QStringLiteral("SectionLabel"));

    m_dropZone = new FileDropZone(contentColumn);

    inputSectionLayout->addWidget(m_inputSectionLabel);
    inputSectionLayout->addWidget(m_dropZone);
    contentLayout->addLayout(inputSectionLayout);

    // Output destination section
    auto* outputSectionLayout = new QVBoxLayout();
    outputSectionLayout->setContentsMargins(0, 2, 0, 0);
    outputSectionLayout->setSpacing(6);

    auto* outputSectionLabel = new QLabel(QStringLiteral("Output Destination"), contentColumn);
    outputSectionLabel->setObjectName(QStringLiteral("SectionLabel"));

    auto* outputSurface = new QFrame(contentColumn);
    outputSurface->setObjectName(QStringLiteral("SectionSurface"));
    auto* outputGrid = new QGridLayout(outputSurface);
    outputGrid->setContentsMargins(14, 14, 14, 14);
    outputGrid->setHorizontalSpacing(10);
    outputGrid->setVerticalSpacing(10);

    auto* dirLabel = new QLabel(QStringLiteral("Folder"), outputSurface);
    dirLabel->setObjectName(QStringLiteral("FieldLabel"));
    dirLabel->setMinimumWidth(68);

    m_outputDirEdit = new QLineEdit(outputSurface);
    m_outputDirEdit->setPlaceholderText(QStringLiteral("Choose destination folder..."));

    m_browseDirBtn = new QPushButton(QStringLiteral("Browse..."), outputSurface);
    m_browseDirBtn->setCursor(Qt::PointingHandCursor);

    auto* nameLabel = new QLabel(QStringLiteral("File Name"), outputSurface);
    nameLabel->setObjectName(QStringLiteral("FieldLabel"));

    m_outputNameEdit = new QLineEdit(outputSurface);
    m_outputNameEdit->setPlaceholderText(QStringLiteral("Output file name..."));

    m_saveAsBtn = new QPushButton(QStringLiteral("Save As..."), outputSurface);
    m_saveAsBtn->setCursor(Qt::PointingHandCursor);

    m_outputHintLabel = new QLabel(outputSurface);
    m_outputHintLabel->setWordWrap(true);
    m_outputHintLabel->setVisible(false);

    outputGrid->addWidget(dirLabel, 0, 0);
    outputGrid->addWidget(m_outputDirEdit, 0, 1);
    outputGrid->addWidget(m_browseDirBtn, 0, 2);

    outputGrid->addWidget(nameLabel, 1, 0);
    outputGrid->addWidget(m_outputNameEdit, 1, 1);
    outputGrid->addWidget(m_saveAsBtn, 1, 2);

    outputGrid->addWidget(m_outputHintLabel, 2, 1, 1, 2);
    outputGrid->setColumnStretch(1, 1);

    outputSectionLayout->addWidget(outputSectionLabel);
    outputSectionLayout->addWidget(outputSurface);
    contentLayout->addLayout(outputSectionLayout);

    // Primary action button + progress indicator
    auto* actionSection = new QVBoxLayout();
    actionSection->setContentsMargins(0, 4, 0, 0);
    actionSection->setSpacing(8);

    m_actionBtn = new QPushButton(QStringLiteral("Compress File"), contentColumn);
    m_actionBtn->setObjectName(QStringLiteral("PrimaryButton"));
    m_actionBtn->setCursor(Qt::PointingHandCursor);
    m_actionBtn->setMinimumHeight(40);

    m_progressContainer = new QWidget(contentColumn);
    auto* progressLayout = new QVBoxLayout(m_progressContainer);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(5);

    m_progressBar = new QProgressBar(m_progressContainer);
    m_progressBar->setRange(0, 0);
    m_progressBar->setTextVisible(false);

    m_progressLabel = new QLabel(m_progressContainer);
    m_progressLabel->setObjectName(QStringLiteral("HintLabel"));

    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_progressLabel);
    m_progressContainer->setVisible(false);

    actionSection->addWidget(m_actionBtn);
    actionSection->addWidget(m_progressContainer);
    contentLayout->addLayout(actionSection);

    // Error notification banner
    m_errorBanner = new QFrame(contentColumn);
    m_errorBanner->setObjectName(QStringLiteral("ErrorBanner"));
    auto* errorLayout = new QHBoxLayout(m_errorBanner);
    errorLayout->setContentsMargins(12, 10, 10, 10);
    errorLayout->setSpacing(10);

    m_errorBannerLabel = new QLabel(m_errorBanner);
    m_errorBannerLabel->setObjectName(QStringLiteral("ErrorBannerText"));
    m_errorBannerLabel->setWordWrap(true);

    m_errorCloseBtn = new QPushButton(m_errorBanner);
    m_errorCloseBtn->setObjectName(QStringLiteral("SubtleIconButton"));
    m_errorCloseBtn->setFixedSize(24, 24);
    m_errorCloseBtn->setCursor(Qt::PointingHandCursor);
    m_errorCloseBtn->setToolTip(QStringLiteral("Dismiss message"));

    errorLayout->addWidget(m_errorBannerLabel, 1);
    errorLayout->addWidget(m_errorCloseBtn, 0, Qt::AlignTop);
    m_errorBanner->setVisible(false);
    contentLayout->addWidget(m_errorBanner);

    // Results panel
    m_resultsPanel = new QFrame(contentColumn);
    m_resultsPanel->setObjectName(QStringLiteral("ResultsPanel"));
    auto* resultsLayout = new QVBoxLayout(m_resultsPanel);
    resultsLayout->setContentsMargins(16, 14, 16, 14);
    resultsLayout->setSpacing(12);

    auto* resultsHeader = new QHBoxLayout();
    resultsHeader->setContentsMargins(0, 0, 0, 0);
    resultsHeader->setSpacing(10);

    m_resultsTitleLabel = new QLabel(m_resultsPanel);
    m_resultsTitleLabel->setObjectName(QStringLiteral("StatusBadgeSuccess"));

    m_openFolderBtn = new QPushButton(QStringLiteral("Open Folder"), m_resultsPanel);
    m_openFolderBtn->setCursor(Qt::PointingHandCursor);

    resultsHeader->addWidget(m_resultsTitleLabel);
    resultsHeader->addStretch();
    resultsHeader->addWidget(m_openFolderBtn);

    auto* divider = new QFrame(m_resultsPanel);
    divider->setObjectName(QStringLiteral("DividerLine"));

    m_statsGrid = new QGridLayout();
    m_statsGrid->setContentsMargins(0, 0, 0, 0);
    m_statsGrid->setHorizontalSpacing(18);
    m_statsGrid->setVerticalSpacing(10);

    resultsLayout->addLayout(resultsHeader);
    resultsLayout->addWidget(divider);
    resultsLayout->addLayout(m_statsGrid);

    m_resultsPanel->setVisible(false);
    contentLayout->addWidget(m_resultsPanel);

    // Optional expandable Huffman code table
    m_codeTableToggleBtn = new QPushButton(contentColumn);
    m_codeTableToggleBtn->setObjectName(QStringLiteral("DisclosureButton"));
    m_codeTableToggleBtn->setCursor(Qt::PointingHandCursor);
    m_codeTableToggleBtn->setVisible(false);
    contentLayout->addWidget(m_codeTableToggleBtn);

    m_codeTableWidget = new QTableWidget(contentColumn);
    m_codeTableWidget->setColumnCount(5);
    m_codeTableWidget->setHorizontalHeaderLabels({
        QStringLiteral("Byte"),
        QStringLiteral("Symbol"),
        QStringLiteral("Frequency"),
        QStringLiteral("Huffman Code"),
        QStringLiteral("Bits")
    });
    m_codeTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_codeTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_codeTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_codeTableWidget->setAlternatingRowColors(true);
    m_codeTableWidget->verticalHeader()->setVisible(false);
    m_codeTableWidget->verticalHeader()->setDefaultSectionSize(26);
    m_codeTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_codeTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_codeTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_codeTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_codeTableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_codeTableWidget->setFixedHeight(210);
    m_codeTableWidget->setVisible(false);
    contentLayout->addWidget(m_codeTableWidget);

    contentLayout->addStretch();

    centerWrapper->addStretch();
    centerWrapper->addWidget(contentColumn, 1);
    centerWrapper->addStretch();

    scrollArea->setWidget(scrollViewportWidget);
    rootLayout->addWidget(scrollArea, 1);

    setCentralWidget(root);

    // Tab order
    setTabOrder(m_themeCombo, m_compressModeBtn);
    setTabOrder(m_compressModeBtn, m_decompressModeBtn);
    setTabOrder(m_decompressModeBtn, m_dropZone);
    setTabOrder(m_dropZone, m_outputDirEdit);
    setTabOrder(m_outputDirEdit, m_browseDirBtn);
    setTabOrder(m_browseDirBtn, m_outputNameEdit);
    setTabOrder(m_outputNameEdit, m_saveAsBtn);
    setTabOrder(m_saveAsBtn, m_actionBtn);

    // Signals
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onThemeChanged);
    connect(m_modeGroup, &QButtonGroup::idClicked, this, &MainWindow::onModeChanged);

    connect(m_dropZone, &FileDropZone::fileSelected, this, &MainWindow::onFileSelected);
    connect(m_dropZone, &FileDropZone::fileCleared, this, &MainWindow::onFileCleared);
    connect(m_dropZone, &FileDropZone::fileRejected, this, &MainWindow::onFileRejected);

    connect(m_browseDirBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDir);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &MainWindow::onSaveAsOutput);
    connect(m_outputDirEdit, &QLineEdit::textEdited, this, &MainWindow::onOutputEdited);
    connect(m_outputNameEdit, &QLineEdit::textEdited, this, &MainWindow::onOutputEdited);

    connect(m_actionBtn, &QPushButton::clicked, this, &MainWindow::onActionClicked);
    connect(m_errorCloseBtn, &QPushButton::clicked, this, &MainWindow::dismissErrorBanner);
    connect(m_openFolderBtn, &QPushButton::clicked, this, &MainWindow::onOpenOutputFolder);
    connect(m_codeTableToggleBtn, &QPushButton::clicked, this, &MainWindow::onToggleCodeTable);
}

void MainWindow::applyTheme(ThemePreset preset)
{
    m_themePreset = preset;
    m_palette = themePalette(preset);
    saveTheme(preset);

    qApp->setStyleSheet(buildApplicationStyleSheet(m_palette));

    const QIcon appIcon = createAppLogoIcon(m_palette);
    setWindowIcon(appIcon);
    m_logoLabel->setPixmap(appIcon.pixmap(28, 28));

    m_dropZone->applyTheme(m_palette);
    m_openFolderBtn->setIcon(createFolderIcon(QColor(m_palette.textSecondary)));
    m_errorCloseBtn->setIcon(createCloseIcon(QColor(m_palette.textSecondary)));
    m_codeTableToggleBtn->setIcon(createChevronIcon(QColor(m_palette.textSecondary), m_codeTableExpanded));
}

void MainWindow::onThemeChanged(int index)
{
    if (index < 0 || index > 2)
    {
        return;
    }
    applyTheme(static_cast<ThemePreset>(index));
}

void MainWindow::onModeChanged(int id)
{
    if (m_busy)
    {
        return;
    }

    const auto nextMode = static_cast<OperationMode>(id);
    if (nextMode == m_mode)
    {
        return;
    }

    saveCurrentModeState();

    m_mode = nextMode;
    m_suppressOverwriteHintForCurrentOutput = false;
    hideError();
    clearResults();
    updateModeUi();
    restoreModeState(m_mode);
    validateInputs();
}

void MainWindow::saveCurrentModeState()
{
    OperationUiState& state = m_modeStates[static_cast<int>(m_mode)];
    state.inputPath = m_dropZone->filePath();
    state.outputDir = m_outputDirEdit->text();
    state.outputName = m_outputNameEdit->text();
    state.outputDirCustom = m_outputDirCustom;
    state.outputNameCustom = m_outputNameCustom;
}

void MainWindow::restoreModeState(OperationMode mode)
{
    const OperationUiState& state = m_modeStates[static_cast<int>(mode)];

    {
        const QSignalBlocker dropBlocker(m_dropZone);
        const QSignalBlocker dirBlocker(m_outputDirEdit);
        const QSignalBlocker nameBlocker(m_outputNameEdit);

        m_dropZone->clearFile();
        if (!state.inputPath.isEmpty())
        {
            const QFileInfo info(state.inputPath);
            if (info.exists() && info.isFile())
            {
                m_dropZone->setFilePath(state.inputPath);
            }
            else
            {
                showError(QStringLiteral("Previously selected file no longer exists: %1")
                              .arg(QDir::toNativeSeparators(state.inputPath)));
            }
        }

        m_outputDirEdit->setText(state.outputDir);
        m_outputNameEdit->setText(state.outputName);
    }

    m_outputDirCustom = state.outputDirCustom;
    m_outputNameCustom = state.outputNameCustom;
}

void MainWindow::updateModeUi()
{
    if (m_mode == OperationMode::Compress)
    {
        m_inputSectionLabel->setText(QStringLiteral("Source File"));
        m_actionBtn->setText(m_busy ? QStringLiteral("Compressing...") : QStringLiteral("Compress File"));
    }
    else
    {
        m_inputSectionLabel->setText(QStringLiteral("Compressed File"));
        m_actionBtn->setText(m_busy ? QStringLiteral("Decompressing...") : QStringLiteral("Decompress File"));
    }
    m_dropZone->setMode(m_mode);
}

void MainWindow::onFileSelected(const QString& path)
{
    Q_UNUSED(path);
    hideError();
    clearResults();
    m_outputNameCustom = false;
    m_suppressOverwriteHintForCurrentOutput = false;
    suggestOutputPath();
    saveCurrentModeState();
    validateInputs();
}

void MainWindow::onFileCleared()
{
    hideError();
    clearResults();
    m_suppressOverwriteHintForCurrentOutput = false;
    if (!m_outputDirCustom)
    {
        m_outputDirEdit->clear();
    }
    if (!m_outputNameCustom)
    {
        m_outputNameEdit->clear();
    }
    saveCurrentModeState();
    validateInputs();
}

void MainWindow::onFileRejected(const QString& reason)
{
    showError(reason);
}

void MainWindow::suggestOutputPath()
{
    if (!m_dropZone->hasFile())
    {
        return;
    }

    const QFileInfo inputInfo(m_dropZone->filePath());
    if (!m_outputDirCustom || m_outputDirEdit->text().trimmed().isEmpty())
    {
        m_outputDirEdit->setText(QDir::toNativeSeparators(inputInfo.absolutePath()));
        m_outputDirCustom = false;
    }
    m_outputNameEdit->setText(buildSuggestedFilename(inputInfo, m_mode));
}

void MainWindow::onBrowseOutputDir()
{
    if (m_busy)
    {
        return;
    }

    QString startDir = m_outputDirEdit->text().trimmed();
    if (startDir.isEmpty() && m_dropZone->hasFile())
    {
        startDir = QFileInfo(m_dropZone->filePath()).absolutePath();
    }
    if (startDir.isEmpty())
    {
        startDir = QDir::homePath();
    }

    const QString chosenDir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select Output Folder"),
        startDir);

    if (!chosenDir.isEmpty())
    {
        m_outputDirEdit->setText(QDir::toNativeSeparators(chosenDir));
        m_outputDirCustom = true;
        m_suppressOverwriteHintForCurrentOutput = false;
        hideError();
        saveCurrentModeState();
        validateInputs();
    }
}

void MainWindow::onSaveAsOutput()
{
    if (m_busy)
    {
        return;
    }

    QString initialPath = currentOutputPath();
    if (initialPath.isEmpty())
    {
        initialPath = QDir::homePath();
    }

    const QString filter = (m_mode == OperationMode::Compress)
        ? QStringLiteral("Huffman Compressed (*.huff);;All Files (*)")
        : QStringLiteral("All Files (*)");

    const QString chosen = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Choose Output File"),
        initialPath,
        filter,
        nullptr,
        QFileDialog::DontConfirmOverwrite);

    if (!chosen.isEmpty())
    {
        const QFileInfo info(chosen);
        m_outputDirEdit->setText(QDir::toNativeSeparators(info.absolutePath()));
        m_outputNameEdit->setText(info.fileName());
        m_outputDirCustom = true;
        m_outputNameCustom = true;
        m_suppressOverwriteHintForCurrentOutput = false;
        hideError();
        saveCurrentModeState();
        validateInputs();
    }
}

void MainWindow::onOutputEdited()
{
    if (sender() == m_outputDirEdit)
    {
        m_outputDirCustom = !m_outputDirEdit->text().trimmed().isEmpty();
    }
    else if (sender() == m_outputNameEdit)
    {
        m_outputNameCustom = !m_outputNameEdit->text().trimmed().isEmpty();
    }
    else
    {
        m_outputDirCustom = true;
        m_outputNameCustom = true;
    }
    m_suppressOverwriteHintForCurrentOutput = false;
    hideError();
    saveCurrentModeState();
    validateInputs();
}

QString MainWindow::currentOutputPath() const
{
    const QString dirText = m_outputDirEdit->text().trimmed();
    const QString nameText = m_outputNameEdit->text().trimmed();
    if (dirText.isEmpty() || nameText.isEmpty())
    {
        return {};
    }
    return QDir(QDir::fromNativeSeparators(dirText)).filePath(nameText);
}

bool MainWindow::pathsReferToSameFile(const QString& pathA, const QString& pathB) const
{
    if (pathA.isEmpty() || pathB.isEmpty())
    {
        return false;
    }

    const QFileInfo infoA(pathA);
    const QFileInfo infoB(pathB);

    const QString canonA = infoA.canonicalFilePath();
    const QString canonB = infoB.canonicalFilePath();
    if (!canonA.isEmpty() && !canonB.isEmpty())
    {
#ifdef Q_OS_WIN
        return canonA.compare(canonB, Qt::CaseInsensitive) == 0;
#else
        return canonA == canonB;
#endif
    }

    const QString absA = QDir::cleanPath(infoA.absoluteFilePath());
    const QString absB = QDir::cleanPath(infoB.absoluteFilePath());
#ifdef Q_OS_WIN
    return absA.compare(absB, Qt::CaseInsensitive) == 0;
#else
    return absA == absB;
#endif
}

void MainWindow::validateInputs()
{
    m_outputHintLabel->setVisible(false);
    m_outputHintLabel->setObjectName(QStringLiteral("HintLabel"));

    if (m_busy)
    {
        m_actionBtn->setEnabled(false);
        return;
    }

    if (!m_dropZone->hasFile())
    {
        m_actionBtn->setEnabled(false);
        return;
    }

    const QString inputPath = m_dropZone->filePath();
    const QFileInfo inputInfo(inputPath);
    if (!inputInfo.exists() || !inputInfo.isFile())
    {
        m_outputHintLabel->setObjectName(QStringLiteral("ErrorHint"));
        m_outputHintLabel->setText(QStringLiteral("The selected input file no longer exists."));
        m_outputHintLabel->style()->unpolish(m_outputHintLabel);
        m_outputHintLabel->style()->polish(m_outputHintLabel);
        m_outputHintLabel->setVisible(true);
        m_actionBtn->setEnabled(false);
        return;
    }

    const QString dirText = m_outputDirEdit->text().trimmed();
    const QString nameText = m_outputNameEdit->text().trimmed();

    if (dirText.isEmpty() || nameText.isEmpty())
    {
        m_actionBtn->setEnabled(false);
        return;
    }

    if (nameText.contains(QLatin1Char('/')) || nameText.contains(QLatin1Char('\\')) ||
        nameText == QStringLiteral(".") || nameText == QStringLiteral(".."))
    {
        m_outputHintLabel->setObjectName(QStringLiteral("ErrorHint"));
        m_outputHintLabel->setText(QStringLiteral("Enter a valid file name without folder separators."));
        m_outputHintLabel->style()->unpolish(m_outputHintLabel);
        m_outputHintLabel->style()->polish(m_outputHintLabel);
        m_outputHintLabel->setVisible(true);
        m_actionBtn->setEnabled(false);
        return;
    }

    const QFileInfo dirInfo(QDir::fromNativeSeparators(dirText));
    if (!dirInfo.exists() || !dirInfo.isDir())
    {
        m_outputHintLabel->setObjectName(QStringLiteral("ErrorHint"));
        m_outputHintLabel->setText(QStringLiteral("The output folder does not exist."));
        m_outputHintLabel->style()->unpolish(m_outputHintLabel);
        m_outputHintLabel->style()->polish(m_outputHintLabel);
        m_outputHintLabel->setVisible(true);
        m_actionBtn->setEnabled(false);
        return;
    }

    const QString outPath = currentOutputPath();
    if (pathsReferToSameFile(inputPath, outPath))
    {
        m_outputHintLabel->setObjectName(QStringLiteral("ErrorHint"));
        m_outputHintLabel->setText(QStringLiteral("Input and output cannot refer to the same file."));
        m_outputHintLabel->style()->unpolish(m_outputHintLabel);
        m_outputHintLabel->style()->polish(m_outputHintLabel);
        m_outputHintLabel->setVisible(true);
        m_actionBtn->setEnabled(false);
        return;
    }

    if (QFileInfo::exists(outPath) && !m_suppressOverwriteHintForCurrentOutput)
    {
        m_outputHintLabel->setObjectName(QStringLiteral("WarningHint"));
        m_outputHintLabel->setText(QStringLiteral("A file with this name already exists. You will be asked before overwriting."));
        m_outputHintLabel->style()->unpolish(m_outputHintLabel);
        m_outputHintLabel->style()->polish(m_outputHintLabel);
        m_outputHintLabel->setVisible(true);
    }

    m_actionBtn->setEnabled(true);
}

void MainWindow::onActionClicked()
{
    if (m_busy)
    {
        return;
    }

    validateInputs();
    if (!m_actionBtn->isEnabled())
    {
        return;
    }

    const QString inputPath = m_dropZone->filePath();
    const QString outputPath = currentOutputPath();

    if (QFileInfo::exists(outputPath))
    {
        const QFileInfo outInfo(outputPath);
        const auto choice = QMessageBox::question(
            this,
            QStringLiteral("Replace Existing File?"),
            QStringLiteral("\"%1\" already exists in \"%2\".\n\nDo you want to overwrite it?")
                .arg(outInfo.fileName(), QDir::toNativeSeparators(outInfo.absolutePath())),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);

        if (choice != QMessageBox::Yes)
        {
            return;
        }
    }

    m_suppressOverwriteHintForCurrentOutput = false;
    hideError();
    clearResults();
    setBusy(true);

    emit requestOperation(m_mode, inputPath, outputPath);
}

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    m_compressModeBtn->setEnabled(!busy);
    m_decompressModeBtn->setEnabled(!busy);
    m_dropZone->setBusy(busy);
    m_outputDirEdit->setEnabled(!busy);
    m_browseDirBtn->setEnabled(!busy);
    m_outputNameEdit->setEnabled(!busy);
    m_saveAsBtn->setEnabled(!busy);
    m_themeCombo->setEnabled(!busy);

    m_progressContainer->setVisible(busy);
    if (busy)
    {
        m_progressLabel->setText(
            m_mode == OperationMode::Compress
                ? QStringLiteral("Analyzing frequencies, building Huffman tree, and encoding file...")
                : QStringLiteral("Reconstructing Huffman tree and decoding bitstream..."));
    }

    updateModeUi();
    validateInputs();
}

void MainWindow::onOperationFinished(const OperationResult& result)
{
    setBusy(false);

    if (!result.success)
    {
        showError(result.errorMessage);
        return;
    }

    m_lastOutputPath = result.outputPath;
    m_suppressOverwriteHintForCurrentOutput = true;
    populateResults(result);
    populateCodeTable(result);
    validateInputs();
}

void MainWindow::showError(const QString& message)
{
    m_errorBannerLabel->setText(message);
    m_errorBanner->setVisible(true);
}

void MainWindow::hideError()
{
    m_errorBanner->setVisible(false);
    m_errorBannerLabel->clear();
}

void MainWindow::dismissErrorBanner()
{
    hideError();
}

void MainWindow::clearResults()
{
    m_resultsPanel->setVisible(false);
    m_codeTableToggleBtn->setVisible(false);
    m_codeTableWidget->setVisible(false);
    m_codeTableWidget->setRowCount(0);
    m_codeTableExpanded = false;
    m_codeTableToggleBtn->setIcon(createChevronIcon(QColor(m_palette.textSecondary), false));

    while (QLayoutItem* item = m_statsGrid->takeAt(0))
    {
        if (QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }
}

void MainWindow::populateResults(const OperationResult& result)
{
    while (QLayoutItem* item = m_statsGrid->takeAt(0))
    {
        if (QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }

    auto addStatCell = [this](int row, int col, const QString& key, const QString& value,
                              const QString& valueObjectName = QStringLiteral("StatValue"),
                              const QString& tooltip = QString(),
                              int colSpan = 1) {
        auto* cell = new QWidget(m_resultsPanel);
        auto* cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(2);

        auto* keyLabel = new QLabel(key, cell);
        keyLabel->setObjectName(QStringLiteral("StatKey"));

        auto* valLabel = new QLabel(value, cell);
        valLabel->setObjectName(valueObjectName);
        valLabel->setWordWrap(colSpan > 1);
        valLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (!tooltip.isEmpty())
        {
            valLabel->setToolTip(tooltip);
        }

        cellLayout->addWidget(keyLabel);
        cellLayout->addWidget(valLabel);
        m_statsGrid->addWidget(cell, row, col, 1, colSpan);
    };

    const QString nativeOut = QDir::toNativeSeparators(result.outputPath);

    if (result.mode == OperationMode::Compress)
    {
        m_resultsTitleLabel->setText(QStringLiteral("Compression complete"));

        QString savedText;
        QString savedStyle = QStringLiteral("StatValue");
        if (result.inputBytes == 0)
        {
            savedText = QStringLiteral("0 B (empty file)");
        }
        else if (result.outputBytes < result.inputBytes)
        {
            const qint64 savedBytes = result.inputBytes - result.outputBytes;
            const double savedPct = (static_cast<double>(savedBytes) / static_cast<double>(result.inputBytes)) * 100.0;
            savedText = QStringLiteral("%1 (%2% saved)")
                .arg(formatByteSize(savedBytes), QString::number(savedPct, 'f', 1));
            savedStyle = QStringLiteral("StatValuePositive");
        }
        else if (result.outputBytes == result.inputBytes)
        {
            savedText = QStringLiteral("0 B (0.0% saved)");
        }
        else
        {
            const qint64 extraBytes = result.outputBytes - result.inputBytes;
            const double extraPct = (static_cast<double>(extraBytes) / static_cast<double>(result.inputBytes)) * 100.0;
            savedText = QStringLiteral("+%1 larger (+%2% overhead)")
                .arg(formatByteSize(extraBytes), QString::number(extraPct, 'f', 1));
            savedStyle = QStringLiteral("StatValueWarning");
        }

        QString ratioText = QStringLiteral("—");
        if (result.inputBytes > 0 && result.outputBytes > 0)
        {
            const double ratio = static_cast<double>(result.inputBytes) / static_cast<double>(result.outputBytes);
            const double pctOfOrig = (static_cast<double>(result.outputBytes) / static_cast<double>(result.inputBytes)) * 100.0;
            ratioText = QStringLiteral("%1 : 1 (%2% of original)")
                .arg(QString::number(ratio, 'f', 2), QString::number(pctOfOrig, 'f', 1));
        }

        addStatCell(0, 0, QStringLiteral("Original Size"), formatByteSize(result.inputBytes));
        addStatCell(0, 1, QStringLiteral("Compressed Size"), formatByteSize(result.outputBytes));
        addStatCell(1, 0, QStringLiteral("Space Saved"), savedText, savedStyle);
        addStatCell(1, 1, QStringLiteral("Compression Ratio"), ratioText);
        addStatCell(2, 0, QStringLiteral("Processing Time"), formatDurationNs(result.elapsedNs));
        addStatCell(2, 1, QStringLiteral("Unique Symbols"), QString::number(result.codeTable.size()));
        addStatCell(3, 0, QStringLiteral("Output Location"), nativeOut, QStringLiteral("StatValue"), nativeOut, 2);
    }
    else
    {
        m_resultsTitleLabel->setText(QStringLiteral("Decompression complete"));

        addStatCell(0, 0, QStringLiteral("Compressed Size"), formatByteSize(result.inputBytes));
        addStatCell(0, 1, QStringLiteral("Decompressed Size"), formatByteSize(result.outputBytes));
        addStatCell(1, 0, QStringLiteral("Processing Time"), formatDurationNs(result.elapsedNs));
        addStatCell(1, 1, QStringLiteral("Unique Symbols"), QString::number(result.codeTable.size()));
        addStatCell(2, 0, QStringLiteral("Output Location"), nativeOut, QStringLiteral("StatValue"), nativeOut, 2);
    }

    m_statsGrid->setColumnStretch(0, 1);
    m_statsGrid->setColumnStretch(1, 1);
    m_resultsPanel->setVisible(true);
}

void MainWindow::populateCodeTable(const OperationResult& result)
{
    if (result.codeTable.isEmpty())
    {
        m_codeTableToggleBtn->setVisible(false);
        m_codeTableWidget->setVisible(false);
        return;
    }

    const int count = result.codeTable.size();
    m_codeTableExpanded = false;
    m_codeTableToggleBtn->setText(
        QStringLiteral("Show Huffman Code Table (%1 unique %2)")
            .arg(count)
            .arg(count == 1 ? QStringLiteral("symbol") : QStringLiteral("symbols")));
    m_codeTableToggleBtn->setIcon(createChevronIcon(QColor(m_palette.textSecondary), false));
    m_codeTableToggleBtn->setVisible(true);

    const bool showFrequency = (result.mode == OperationMode::Compress);
    m_codeTableWidget->setColumnHidden(2, !showFrequency);

    m_codeTableWidget->setRowCount(count);
    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    for (int row = 0; row < count; ++row)
    {
        const CodeTableEntry& entry = result.codeTable.at(row);

        auto* hexItem = new QTableWidgetItem(formatHexByte(entry.byteValue));
        hexItem->setFont(monoFont);
        hexItem->setTextAlignment(Qt::AlignCenter);

        auto* symItem = new QTableWidgetItem(formatSymbolDisplay(entry.byteValue));
        symItem->setTextAlignment(Qt::AlignCenter);

        auto* freqItem = new QTableWidgetItem(showFrequency ? QString::number(entry.frequency) : QStringLiteral("—"));
        freqItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* codeItem = new QTableWidgetItem(entry.code);
        codeItem->setFont(monoFont);

        auto* bitsItem = new QTableWidgetItem(QString::number(entry.code.size()));
        bitsItem->setTextAlignment(Qt::AlignCenter);

        m_codeTableWidget->setItem(row, 0, hexItem);
        m_codeTableWidget->setItem(row, 1, symItem);
        m_codeTableWidget->setItem(row, 2, freqItem);
        m_codeTableWidget->setItem(row, 3, codeItem);
        m_codeTableWidget->setItem(row, 4, bitsItem);
    }

    m_codeTableWidget->setVisible(false);
}

void MainWindow::onToggleCodeTable()
{
    m_codeTableExpanded = !m_codeTableExpanded;
    m_codeTableWidget->setVisible(m_codeTableExpanded);
    m_codeTableToggleBtn->setIcon(createChevronIcon(QColor(m_palette.textSecondary), m_codeTableExpanded));

    const int count = m_codeTableWidget->rowCount();
    m_codeTableToggleBtn->setText(
        QStringLiteral("%1 Huffman Code Table (%2 unique %3)")
            .arg(m_codeTableExpanded ? QStringLiteral("Hide") : QStringLiteral("Show"))
            .arg(count)
            .arg(count == 1 ? QStringLiteral("symbol") : QStringLiteral("symbols")));
}

void MainWindow::onOpenOutputFolder()
{
    if (m_lastOutputPath.isEmpty())
    {
        return;
    }

    const QString folder = QFileInfo(m_lastOutputPath).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}
