#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "SearchSessionModel.h"

#include <QMainWindow>
#include <QString>
#include <QVector>

class AlignmentDetailWidget;
class GenomeAlignmentWidget;
class GenomeOverviewWidget;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QResizeEvent;
class QSlider;
class QSpinBox;
class ResultPanel;
class SearchEngine;
class SnackbarHost;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    SearchEngine *m_engine;
    QString m_referenceSequence;
    QVector<ReadInputViewData> m_readInputs;
    int m_currentReadInputIndex;
    bool m_searchRunning;
    SearchSessionModel m_session;

    QSpinBox *m_referenceLengthSpin;
    QSpinBox *m_kSpin;
    QSpinBox *m_mismatchSpin;
    QSpinBox *m_readLengthSpin;
    QSpinBox *m_readCountSpin;
    QLineEdit *m_readInput;
    QPushButton *m_importReferenceButton;
    QPushButton *m_importReadsButton;
    QPushButton *m_exportButton;
    QPushButton *m_generateReferenceButton;
    QPushButton *m_generateReadsButton;
    QPushButton *m_buildIndexButton;
    QPushButton *m_batchButton;
    QPushButton *m_clearButton;
    QPushButton *m_singleSearchButton;
    QProgressBar *m_batchProgress;
    QSlider *m_zoomSlider;

    GenomeOverviewWidget *m_overview;
    GenomeAlignmentWidget *m_alignment;
    AlignmentDetailWidget *m_detail;
    ResultPanel *m_resultPanel;
    QPlainTextEdit *m_logView;

    SnackbarHost *m_snackbarHost;
    QLabel *m_currentReadLabel;
    QLabel *m_referenceMetric;
    QLabel *m_kmerMetric;
    QLabel *m_buildMetric;
    QLabel *m_loadMetric;
    QLabel *m_searchMetric;

    void setupUi();
    void applyStyle();
    void importReference();
    void importReads();
    void exportResults();
    void buildIndex();
    bool validateIndexReady(QString *error) const;
    void generateReference();
    void generateReads();
    void runSingleSearch();
    void runBatchSearch();
    void clearResults();
    void clearSelection();
    void handleKChanged(int value);
    void handleMismatchChanged(int value);
    void setOperationControlsEnabled(bool enabled);
    void selectAvailableRead(int index, bool executeSearch);
    void selectResult(int readIndex, int matchIndex, bool centerView);
    int findAvailableReadIndexById(int readId) const;
    void updateCurrentReadHint();
    bool validateCurrentParameters(QString *error) const;
    bool validateReadSequence(const QString &sequence,
                              QString *error,
                              int readNumber = -1) const;
    void refreshViews();
    void updateIndexMetrics();
    void resetIndexMetrics();
    void showFeedback(const QString &message, bool error = false);
    void appendLog(const QString &message, bool error = false);
};

#endif // MAINWINDOW_H
