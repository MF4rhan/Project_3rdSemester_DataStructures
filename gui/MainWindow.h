#pragma once

#include "HuffmanWorker.h"
#include "Theme.h"

#include <QMainWindow>

class FileDropZone;
class QButtonGroup;
class QComboBox;
class QFrame;
class QGridLayout;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void requestOperation(OperationMode mode, const QString& inputPath, const QString& outputPath);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onModeChanged(int id);
    void onThemeChanged(int index);
    void onFileSelected(const QString& path);
    void onFileCleared();
    void onFileRejected(const QString& reason);
    void onBrowseOutputDir();
    void onSaveAsOutput();
    void onOutputEdited();
    void onActionClicked();
    void onOperationFinished(const OperationResult& result);
    void onOpenOutputFolder();
    void onToggleCodeTable();
    void dismissErrorBanner();

private:
    void setupUi();
    void applyTheme(ThemePreset preset);
    void updateModeUi();
    void saveCurrentModeState();
    void restoreModeState(OperationMode mode);
    void suggestOutputPath();
    void validateInputs();
    void setBusy(bool busy);
    void showError(const QString& message);
    void hideError();
    void clearResults();
    void populateResults(const OperationResult& result);
    void populateCodeTable(const OperationResult& result);
    QString currentOutputPath() const;
    bool pathsReferToSameFile(const QString& pathA, const QString& pathB) const;

    struct OperationUiState
    {
        QString inputPath;
        QString outputDir;
        QString outputName;
        bool outputDirCustom = false;
        bool outputNameCustom = false;
    };

    OperationMode m_mode = OperationMode::Compress;
    OperationUiState m_modeStates[2];
    ThemePreset m_themePreset = ThemePreset::Graphite;
    ThemePalette m_palette;
    bool m_busy = false;
    bool m_outputDirCustom = false;
    bool m_outputNameCustom = false;
    bool m_suppressOverwriteHintForCurrentOutput = false;
    bool m_codeTableExpanded = false;
    QString m_lastOutputPath;

    QThread* m_workerThread = nullptr;
    HuffmanWorker* m_worker = nullptr;

    // Header
    QLabel* m_logoLabel = nullptr;
    QComboBox* m_themeCombo = nullptr;

    // Mode selector
    QButtonGroup* m_modeGroup = nullptr;
    QPushButton* m_compressModeBtn = nullptr;
    QPushButton* m_decompressModeBtn = nullptr;

    // Input
    QLabel* m_inputSectionLabel = nullptr;
    FileDropZone* m_dropZone = nullptr;

    // Output
    QLineEdit* m_outputDirEdit = nullptr;
    QPushButton* m_browseDirBtn = nullptr;
    QLineEdit* m_outputNameEdit = nullptr;
    QPushButton* m_saveAsBtn = nullptr;
    QLabel* m_outputHintLabel = nullptr;

    // Primary action & progress
    QPushButton* m_actionBtn = nullptr;
    QWidget* m_progressContainer = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_progressLabel = nullptr;

    // Error banner
    QFrame* m_errorBanner = nullptr;
    QLabel* m_errorBannerLabel = nullptr;
    QPushButton* m_errorCloseBtn = nullptr;

    // Results
    QFrame* m_resultsPanel = nullptr;
    QLabel* m_resultsTitleLabel = nullptr;
    QPushButton* m_openFolderBtn = nullptr;
    QGridLayout* m_statsGrid = nullptr;

    // Optional Huffman code table
    QPushButton* m_codeTableToggleBtn = nullptr;
    QTableWidget* m_codeTableWidget = nullptr;
};
