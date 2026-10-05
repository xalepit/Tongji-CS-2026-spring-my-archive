#include "MainWindow.h"

#include "AlignmentDetailWidget.h"
#include "ApplicationLimits.h"
#include "GenomeAlignmentWidget.h"
#include "GenomeOverviewWidget.h"
#include "ResultPanel.h"
#include "SequenceFileIO.h"
#include "SnackbarHost.h"
#include "core/Genome.h"
#include "core/ReadGenerator.h"
#include "core/SearchEngine.h"

#include <QApplication>
#include <QByteArray>
#include <QDateTime>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QParallelAnimationGroup>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>

#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>

namespace {

class ScopeExit
{
public:
    explicit ScopeExit(std::function<void()> callback)
        : m_callback(std::move(callback))
    {
    }

    ~ScopeExit()
    {
        m_callback();
    }

    ScopeExit(const ScopeExit &) = delete;
    ScopeExit &operator=(const ScopeExit &) = delete;

private:
    std::function<void()> m_callback;
};

class HoverSearchButton final : public QPushButton
{
public:
    explicit HoverSearchButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent),
          m_shadow(new QGraphicsDropShadowEffect(this)),
          m_animation(new QParallelAnimationGroup(this)),
          m_blurAnimation(
              new QPropertyAnimation(m_shadow, "blurRadius", m_animation)),
          m_colorAnimation(
              new QPropertyAnimation(m_shadow, "color", m_animation)),
          m_offsetAnimation(
              new QPropertyAnimation(m_shadow, "offset", m_animation))
    {
        m_shadow->setBlurRadius(5.0);
        m_shadow->setColor(QColor(40, 118, 158, 45));
        m_shadow->setOffset(0.0, 1.0);
        setGraphicsEffect(m_shadow);
        setCursor(Qt::PointingHandCursor);

        m_blurAnimation->setDuration(150);
        m_colorAnimation->setDuration(150);
        m_offsetAnimation->setDuration(150);
        m_blurAnimation->setEasingCurve(QEasingCurve::OutCubic);
        m_colorAnimation->setEasingCurve(QEasingCurve::OutCubic);
        m_offsetAnimation->setEasingCurve(QEasingCurve::OutCubic);
        m_animation->addAnimation(m_blurAnimation);
        m_animation->addAnimation(m_colorAnimation);
        m_animation->addAnimation(m_offsetAnimation);
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        animateHover(true);
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        animateHover(false);
        QPushButton::leaveEvent(event);
    }

private:
    QGraphicsDropShadowEffect *m_shadow;
    QParallelAnimationGroup *m_animation;
    QPropertyAnimation *m_blurAnimation;
    QPropertyAnimation *m_colorAnimation;
    QPropertyAnimation *m_offsetAnimation;

    void animateHover(bool hovered)
    {
        m_animation->stop();
        m_blurAnimation->setStartValue(m_shadow->blurRadius());
        m_blurAnimation->setEndValue(hovered ? 18.0 : 5.0);
        m_colorAnimation->setStartValue(m_shadow->color());
        m_colorAnimation->setEndValue(
            hovered ? QColor(61, 178, 211, 145)
                    : QColor(40, 118, 158, 45));
        m_offsetAnimation->setStartValue(m_shadow->offset());
        m_offsetAnimation->setEndValue(
            hovered ? QPointF(0.0, 3.0) : QPointF(0.0, 1.0));
        m_animation->start();
    }
};

QLabel *sectionLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(QStringLiteral("sectionLabel"));
    return label;
}

QWidget *parameterLabel(const QString &text,
                        const QString &description,
                        const QString &rangeText)
{
    QWidget *container = new QWidget;
    container->setObjectName(QStringLiteral("parameterLabelGroup"));
    container->setToolTip(description);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    QLabel *label = new QLabel(text);
    label->setObjectName(QStringLiteral("parameterLabel"));
    label->setToolTip(description);
    QLabel *range = new QLabel(rangeText);
    range->setObjectName(QStringLiteral("parameterRange"));
    range->setToolTip(description);
    layout->addWidget(label);
    layout->addWidget(range);
    return container;
}

QWidget *stepperEditor(QSpinBox *spinBox, const QString &description)
{
    QWidget *editor = new QWidget;
    editor->setObjectName(QStringLiteral("spinEditor"));
    editor->setAttribute(Qt::WA_StyledBackground, true);
    editor->setToolTip(description);

    QHBoxLayout *layout = new QHBoxLayout(editor);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    spinBox->setObjectName(QStringLiteral("flatSpinBox"));
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setFrame(false);
    spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QToolButton *decrease = new QToolButton;
    decrease->setObjectName(QStringLiteral("stepButton"));
    decrease->setText(QStringLiteral("−"));
    decrease->setToolTip(QStringLiteral("减小数值"));
    decrease->setAutoRepeat(true);

    QToolButton *increase = new QToolButton;
    increase->setObjectName(QStringLiteral("stepButton"));
    increase->setText(QStringLiteral("+"));
    increase->setToolTip(QStringLiteral("增大数值"));
    increase->setAutoRepeat(true);

    QObject::connect(decrease, &QToolButton::clicked,
                     spinBox, &QSpinBox::stepDown);
    QObject::connect(increase, &QToolButton::clicked,
                     spinBox, &QSpinBox::stepUp);
    layout->addWidget(spinBox, 1);
    layout->addWidget(decrease);
    layout->addWidget(increase);
    return editor;
}

QFrame *separator()
{
    QFrame *line = new QFrame;
    line->setObjectName(QStringLiteral("separator"));
    line->setFrameShape(QFrame::HLine);
    return line;
}

QWidget *metricLine(const QString &name, QLabel **value)
{
    QWidget *row = new QWidget;
    row->setObjectName(QStringLiteral("metricLine"));
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(8);
    QLabel *label = new QLabel(name);
    label->setObjectName(QStringLiteral("metricName"));
    *value = new QLabel(QStringLiteral("—"));
    (*value)->setObjectName(QStringLiteral("metricValue"));
    (*value)->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(*value);
    return row;
}

QToolButton *viewButton(const QString &text, const QString &tooltip)
{
    QToolButton *button = new QToolButton;
    button->setObjectName(QStringLiteral("viewButton"));
    button->setText(text);
    button->setToolTip(tooltip);
    button->setAutoRaise(false);
    return button;
}

bool isDna(const QString &sequence)
{
    if (sequence.isEmpty()) {
        return false;
    }
    for (const QChar base : sequence) {
        if (base != 'A' && base != 'T' && base != 'C' && base != 'G') {
            return false;
        }
    }
    return true;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_engine(nullptr),
      m_currentReadInputIndex(-1),
      m_searchRunning(false),
      m_referenceLengthSpin(nullptr),
      m_kSpin(nullptr),
      m_mismatchSpin(nullptr),
      m_readLengthSpin(nullptr),
      m_readCountSpin(nullptr),
      m_readInput(nullptr),
      m_importReferenceButton(nullptr),
      m_importReadsButton(nullptr),
      m_exportButton(nullptr),
      m_generateReferenceButton(nullptr),
      m_generateReadsButton(nullptr),
      m_buildIndexButton(nullptr),
      m_batchButton(nullptr),
      m_clearButton(nullptr),
      m_singleSearchButton(nullptr),
      m_batchProgress(nullptr),
      m_zoomSlider(nullptr),
      m_overview(nullptr),
      m_alignment(nullptr),
      m_detail(nullptr),
      m_resultPanel(nullptr),
      m_logView(nullptr),
      m_snackbarHost(nullptr),
      m_currentReadLabel(nullptr),
      m_referenceMetric(nullptr),
      m_kmerMetric(nullptr),
      m_buildMetric(nullptr),
      m_loadMetric(nullptr),
      m_searchMetric(nullptr)
{
    setupUi();
    applyStyle();
    m_session.SetReference(QString());
    m_session.SetMaxMismatch(m_mismatchSpin->value());
    resetIndexMetrics();
    refreshViews();
    showFeedback(QStringLiteral("请生成测试数据或导入参考基因组"));
}

MainWindow::~MainWindow()
{
    m_readInputs.clear();
    m_referenceSequence.clear();
    m_session.SetReference(QString());
    delete m_engine;
    m_engine = nullptr;
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("DNA 序列片段错配容错检索引擎"));
    resize(1640, 980);
    setMinimumSize(1280, 760);

    QWidget *central = new QWidget;
    central->setObjectName(QStringLiteral("central"));
    setCentralWidget(central);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QFrame *header = new QFrame;
    header->setObjectName(QStringLiteral("header"));
    header->setFixedHeight(66);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(22, 0, 22, 0);
    headerLayout->setSpacing(8);

    QLabel *title =
        new QLabel(QStringLiteral("DNA 序列片段错配容错检索引擎"));
    title->setObjectName(QStringLiteral("appTitle"));
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_importReferenceButton = new QPushButton(QStringLiteral("导入参考基因组"));
    m_importReferenceButton->setIcon(
        style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_importReadsButton = new QPushButton(QStringLiteral("导入 Reads"));
    m_importReadsButton->setIcon(
        style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_exportButton = new QPushButton(QStringLiteral("导出结果"));
    m_exportButton->setIcon(
        style()->standardIcon(QStyle::SP_DialogSaveButton));
    headerLayout->addWidget(m_importReferenceButton);
    headerLayout->addWidget(m_importReadsButton);
    headerLayout->addWidget(m_exportButton);

    root->addWidget(header);

    QSplitter *workspace = new QSplitter(Qt::Vertical);
    workspace->setObjectName(QStringLiteral("workspaceSplitter"));
    workspace->setChildrenCollapsible(false);
    workspace->setHandleWidth(5);
    root->addWidget(workspace, 1);

    QSplitter *body = new QSplitter(Qt::Horizontal);
    body->setObjectName(QStringLiteral("bodySplitter"));
    body->setChildrenCollapsible(false);
    body->setHandleWidth(1);
    workspace->addWidget(body);

    QScrollArea *leftScroll = new QScrollArea;
    leftScroll->setObjectName(QStringLiteral("sideScroll"));
    leftScroll->setWidgetResizable(true);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setMinimumWidth(270);
    leftScroll->setMaximumWidth(310);
    QWidget *leftPanel = new QWidget;
    leftPanel->setObjectName(QStringLiteral("leftPanel"));
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 16, 18, 16);
    leftLayout->setSpacing(10);

    leftLayout->addWidget(sectionLabel(QStringLiteral("参数设置")));
    QFormLayout *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(8);
    form->setVerticalSpacing(8);

    m_referenceLengthSpin = new QSpinBox;
    m_referenceLengthSpin->setProperty("parameterKey",
                                       QStringLiteral("referenceLength"));
    m_referenceLengthSpin->setRange(
        ApplicationLimits::MinReferenceLength,
        ApplicationLimits::MaxReferenceLength);
    m_referenceLengthSpin->setSingleStep(250);
    m_referenceLengthSpin->setValue(
        ApplicationLimits::DefaultReferenceLength);
    m_referenceLengthSpin->setSuffix(QStringLiteral(" bp"));
    m_kSpin = new QSpinBox;
    m_kSpin->setProperty("parameterKey", QStringLiteral("kmerLength"));
    m_kSpin->setRange(ApplicationLimits::MinKmerLength,
                      ApplicationLimits::MaxKmerLength);
    m_kSpin->setValue(ApplicationLimits::DefaultKmerLength);
    m_mismatchSpin = new QSpinBox;
    m_mismatchSpin->setProperty("parameterKey",
                                QStringLiteral("mismatchCount"));
    m_mismatchSpin->setRange(ApplicationLimits::MinMismatchCount,
                             ApplicationLimits::MaxMismatchCount);
    m_mismatchSpin->setValue(ApplicationLimits::DefaultMismatchCount);
    m_readLengthSpin = new QSpinBox;
    m_readLengthSpin->setProperty("parameterKey",
                                  QStringLiteral("readLength"));
    m_readLengthSpin->setRange(ApplicationLimits::MinReadLength,
                               ApplicationLimits::MaxReadLength);
    m_readLengthSpin->setValue(ApplicationLimits::DefaultReadLength);
    m_readLengthSpin->setSuffix(QStringLiteral(" bp"));
    m_readCountSpin = new QSpinBox;
    m_readCountSpin->setProperty("parameterKey",
                                 QStringLiteral("readCount"));
    m_readCountSpin->setRange(ApplicationLimits::MinReadCount,
                              ApplicationLimits::MaxReadCount);
    m_readCountSpin->setValue(ApplicationLimits::DefaultReadCount);

    form->addRow(parameterLabel(
                     QStringLiteral("参考长度"),
                     QStringLiteral("参考基因组长度，范围 %1-%2 bp")
                         .arg(ApplicationLimits::MinReferenceLength)
                         .arg(ApplicationLimits::MaxReferenceLength),
                     QStringLiteral("范围 %1-%2 bp")
                         .arg(ApplicationLimits::MinReferenceLength)
                         .arg(ApplicationLimits::MaxReferenceLength)),
                 stepperEditor(m_referenceLengthSpin,
                               QStringLiteral("参考基因组长度")));
    form->addRow(parameterLabel(
                     QStringLiteral("K-mer 长度 K"),
                     QStringLiteral("哈希索引键和 Read 种子的长度，范围 %1-%2")
                         .arg(ApplicationLimits::MinKmerLength)
                         .arg(ApplicationLimits::MaxKmerLength),
                     QStringLiteral("范围 %1-%2")
                         .arg(ApplicationLimits::MinKmerLength)
                         .arg(ApplicationLimits::MaxKmerLength)),
                 stepperEditor(m_kSpin, QStringLiteral("K-mer 长度")));
    form->addRow(parameterLabel(
                     QStringLiteral("允许错配数 k"),
                     QStringLiteral("汉明距离允许的最大错配数量，范围 %1-%2")
                         .arg(ApplicationLimits::MinMismatchCount)
                         .arg(ApplicationLimits::MaxMismatchCount),
                     QStringLiteral("范围 %1-%2")
                         .arg(ApplicationLimits::MinMismatchCount)
                         .arg(ApplicationLimits::MaxMismatchCount)),
                 stepperEditor(m_mismatchSpin,
                               QStringLiteral("最大允许错配数")));
    form->addRow(parameterLabel(
                     QStringLiteral("Read 长度"),
                     QStringLiteral("随机生成 Read 的长度，范围 %1-%2 bp")
                         .arg(ApplicationLimits::MinReadLength)
                         .arg(ApplicationLimits::MaxReadLength),
                     QStringLiteral("范围 %1-%2 bp")
                         .arg(ApplicationLimits::MinReadLength)
                         .arg(ApplicationLimits::MaxReadLength)),
                 stepperEditor(m_readLengthSpin,
                               QStringLiteral("Read 长度")));
    form->addRow(parameterLabel(
                     QStringLiteral("Reads 数量"),
                     QStringLiteral("随机生成和批量检索数量，范围 %1-%2")
                         .arg(ApplicationLimits::MinReadCount)
                         .arg(ApplicationLimits::MaxReadCount),
                     QStringLiteral("范围 %1-%2 条")
                         .arg(ApplicationLimits::MinReadCount)
                         .arg(ApplicationLimits::MaxReadCount)),
                 stepperEditor(m_readCountSpin,
                               QStringLiteral("Reads 数量")));
    leftLayout->addLayout(form);

    m_generateReferenceButton =
        new QPushButton(QStringLiteral("生成参考基因组"));
    m_generateReferenceButton->setObjectName(QStringLiteral("primaryButton"));
    m_generateReferenceButton->setIcon(
        style()->standardIcon(QStyle::SP_BrowserReload));
    m_generateReferenceButton->setToolTip(
        QStringLiteral("按参考长度参数随机生成新的 Reference"));
    m_generateReadsButton = new QPushButton(QStringLiteral("随机生成 Reads"));
    m_generateReadsButton->setIcon(
        style()->standardIcon(QStyle::SP_FileDialogListView));
    m_generateReadsButton->setToolTip(
        QStringLiteral("根据当前 Reference、Read 长度、数量和 K 生成 Reads"));
    m_buildIndexButton = new QPushButton(QStringLiteral("建立索引"));
    m_buildIndexButton->setIcon(
        style()->standardIcon(QStyle::SP_DriveHDIcon));
    m_buildIndexButton->setToolTip(
        QStringLiteral("为当前 Reference 强制重新建立 K-mer 哈希索引"));
    m_batchButton = new QPushButton(QStringLiteral("高通量批量检索"));
    m_batchButton->setIcon(
        style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_clearButton = new QPushButton(QStringLiteral("清空检索结果"));
    m_clearButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    leftLayout->addWidget(m_generateReferenceButton);
    leftLayout->addWidget(m_generateReadsButton);
    leftLayout->addWidget(m_buildIndexButton);
    leftLayout->addWidget(m_batchButton);
    leftLayout->addWidget(m_clearButton);

    leftLayout->addWidget(separator());
    QHBoxLayout *singleReadHeader = new QHBoxLayout;
    singleReadHeader->setContentsMargins(0, 0, 0, 0);
    singleReadHeader->setSpacing(6);
    singleReadHeader->addWidget(
        sectionLabel(QStringLiteral("单条 Read 检索")));
    singleReadHeader->addStretch();
    m_currentReadLabel = new QLabel(QStringLiteral("当前：未选择"));
    m_currentReadLabel->setObjectName(
        QStringLiteral("currentReadHint"));
    m_currentReadLabel->setAlignment(
        Qt::AlignRight | Qt::AlignVCenter);
    singleReadHeader->addWidget(m_currentReadLabel);
    leftLayout->addLayout(singleReadHeader);
    m_readInput = new QLineEdit;
    m_readInput->setPlaceholderText(QStringLiteral("输入 A / T / C / G 序列"));
    m_readInput->setClearButtonEnabled(true);
    m_readInput->setMaxLength(ApplicationLimits::MaxReadLength + 1);
    m_readInput->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[ATCGatcg]*")),
        m_readInput));
    m_singleSearchButton =
        new HoverSearchButton(QStringLiteral("检索当前 Read"));
    m_singleSearchButton->setObjectName(QStringLiteral("secondaryAccentButton"));
    m_singleSearchButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    leftLayout->addWidget(m_readInput);
    leftLayout->addWidget(m_singleSearchButton);

    leftLayout->addWidget(separator());
    leftLayout->addWidget(sectionLabel(QStringLiteral("运行统计")));
    leftLayout->addWidget(metricLine(QStringLiteral("参考长度"),
                                     &m_referenceMetric));
    leftLayout->addWidget(metricLine(QStringLiteral("不同 K-mer"),
                                     &m_kmerMetric));
    leftLayout->addWidget(metricLine(QStringLiteral("建索引耗时"),
                                     &m_buildMetric));
    leftLayout->addWidget(metricLine(QStringLiteral("哈希负载"),
                                     &m_loadMetric));
    leftLayout->addWidget(metricLine(QStringLiteral("最近检索"),
                                     &m_searchMetric));
    m_batchProgress = new QProgressBar;
    m_batchProgress->setTextVisible(false);
    m_batchProgress->setRange(0, 100);
    m_batchProgress->setValue(0);
    m_batchProgress->setFixedHeight(5);
    leftLayout->addWidget(m_batchProgress);
    leftLayout->addStretch();
    leftScroll->setWidget(leftPanel);
    body->addWidget(leftScroll);

    QWidget *centerPanel = new QWidget;
    centerPanel->setObjectName(QStringLiteral("centerPanel"));
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(16, 14, 16, 12);
    centerLayout->setSpacing(8);

    QHBoxLayout *overviewHeader = new QHBoxLayout;
    QLabel *overviewTitle = sectionLabel(QStringLiteral("全局基因组概览"));
    QLabel *legend = new QLabel(
        QStringLiteral("<span style='color:#35b779'>■</span> 0 错配&nbsp;&nbsp;"
                       "<span style='color:#e6a93d'>■</span> 1 错配&nbsp;&nbsp;"
                       "<span style='color:#e45b5b'>■</span> 2+ 错配&nbsp;&nbsp;"
                       "<span style='color:#5795ec'>□</span> 当前视口"));
    legend->setObjectName(QStringLiteral("legend"));
    overviewHeader->addWidget(overviewTitle);
    overviewHeader->addStretch();
    overviewHeader->addWidget(legend);
    centerLayout->addLayout(overviewHeader);

    m_overview = new GenomeOverviewWidget;
    m_overview->setObjectName(QStringLiteral("overviewWidget"));
    centerLayout->addWidget(m_overview);

    QHBoxLayout *alignmentHeader = new QHBoxLayout;
    QLabel *alignmentTitle = sectionLabel(QStringLiteral("局部序列主沙盘"));
    alignmentHeader->addWidget(alignmentTitle);
    alignmentHeader->addStretch();

    QToolButton *fitButton =
        viewButton(QStringLiteral("适应全局"),
                   QStringLiteral("缩放至完整参考基因组"));
    QToolButton *resetButton =
        viewButton(QStringLiteral("重置视图"),
                   QStringLiteral("恢复字符级缩放"));
    QWidget *zoomControl = new QWidget;
    zoomControl->setObjectName(QStringLiteral("zoomControl"));
    zoomControl->setAttribute(Qt::WA_StyledBackground, true);
    zoomControl->setFixedWidth(170);
    zoomControl->setFixedHeight(34);
    QHBoxLayout *zoomLayout = new QHBoxLayout(zoomControl);
    zoomLayout->setContentsMargins(2, 0, 2, 0);
    zoomLayout->setSpacing(4);
    QLabel *zoomLabel = new QLabel(QStringLiteral("缩放"));
    zoomLabel->setObjectName(QStringLiteral("subtleText"));
    zoomLabel->setFixedWidth(30);
    m_zoomSlider = new QSlider(Qt::Horizontal, zoomControl);
    m_zoomSlider->setObjectName(QStringLiteral("zoomSlider"));
    m_zoomSlider->setRange(0, 100);
    m_zoomSlider->setValue(79);
    m_zoomSlider->setFixedWidth(124);
    m_zoomSlider->setFixedHeight(30);
    zoomLayout->addWidget(zoomLabel);
    zoomLayout->addWidget(m_zoomSlider);
    alignmentHeader->addWidget(fitButton);
    alignmentHeader->addWidget(resetButton);
    alignmentHeader->addSpacing(6);
    alignmentHeader->addWidget(zoomControl);
    centerLayout->addLayout(alignmentHeader);
    zoomControl->raise();
    m_zoomSlider->raise();

    m_alignment = new GenomeAlignmentWidget;
    m_alignment->setObjectName(QStringLiteral("alignmentWidget"));
    centerLayout->addWidget(m_alignment, 1);

    QHBoxLayout *detailHeader = new QHBoxLayout;
    detailHeader->addWidget(sectionLabel(QStringLiteral("精确比对详情")));
    detailHeader->addStretch();
    QLabel *detailHint =
        new QLabel(QStringLiteral("错配显示替换关系与参考绝对位置"));
    detailHint->setObjectName(QStringLiteral("subtleText"));
    detailHeader->addWidget(detailHint);
    centerLayout->addLayout(detailHeader);
    m_detail = new AlignmentDetailWidget;
    m_detail->setObjectName(QStringLiteral("detailWidget"));
    centerLayout->addWidget(m_detail);
    body->addWidget(centerPanel);

    m_resultPanel = new ResultPanel;
    m_resultPanel->setMinimumWidth(315);
    m_resultPanel->setMaximumWidth(390);
    body->addWidget(m_resultPanel);
    body->setStretchFactor(0, 0);
    body->setStretchFactor(1, 1);
    body->setStretchFactor(2, 0);
    body->setSizes({286, 980, 350});

    QFrame *logPanel = new QFrame;
    logPanel->setObjectName(QStringLiteral("logPanel"));
    logPanel->setMinimumHeight(ApplicationLimits::MinLogPanelHeight);
    logPanel->setMaximumHeight(ApplicationLimits::MaxLogPanelHeight);
    QVBoxLayout *logLayout = new QVBoxLayout(logPanel);
    logLayout->setContentsMargins(18, 7, 18, 9);
    logLayout->setSpacing(4);
    QLabel *logTitle = new QLabel(QStringLiteral("运行日志"));
    logTitle->setObjectName(QStringLiteral("logTitle"));
    m_logView = new QPlainTextEdit;
    m_logView->setObjectName(QStringLiteral("logView"));
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(200);
    logLayout->addWidget(logTitle);
    logLayout->addWidget(m_logView, 1);
    workspace->addWidget(logPanel);
    workspace->setStretchFactor(0, 1);
    workspace->setStretchFactor(1, 0);
    workspace->setSizes(
        {1000, ApplicationLimits::DefaultLogPanelHeight});

    m_snackbarHost = new SnackbarHost(central);
    m_snackbarHost->setGeometry(central->rect());
    m_snackbarHost->raise();

    statusBar()->setSizeGripEnabled(true);
    statusBar()->showMessage(QStringLiteral("就绪"));

    connect(m_importReferenceButton, &QPushButton::clicked,
            this, &MainWindow::importReference);
    connect(m_importReadsButton, &QPushButton::clicked,
            this, &MainWindow::importReads);
    connect(m_exportButton, &QPushButton::clicked,
            this, &MainWindow::exportResults);
    connect(m_generateReferenceButton, &QPushButton::clicked,
            this, &MainWindow::generateReference);
    connect(m_generateReadsButton, &QPushButton::clicked,
            this, &MainWindow::generateReads);
    connect(m_buildIndexButton, &QPushButton::clicked,
            this, &MainWindow::buildIndex);
    connect(m_batchButton, &QPushButton::clicked,
            this, &MainWindow::runBatchSearch);
    connect(m_clearButton, &QPushButton::clicked,
            this, &MainWindow::clearResults);
    connect(m_singleSearchButton, &QPushButton::clicked,
            this, &MainWindow::runSingleSearch);
    connect(m_readInput, &QLineEdit::returnPressed,
            this, &MainWindow::runSingleSearch);
    connect(m_readInput, &QLineEdit::textChanged,
            this, [this](const QString &text) {
                const QString upper = text.toUpper();
                if (upper != text) {
                    m_readInput->setText(upper);
                    return;
                }
                if (m_currentReadInputIndex >= 0
                    && m_currentReadInputIndex < m_readInputs.size()
                    && m_readInputs.at(m_currentReadInputIndex).sequence
                           != upper) {
                    m_currentReadInputIndex = -1;
                }
                updateCurrentReadHint();
            });
    connect(m_kSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &MainWindow::handleKChanged);
    connect(m_mismatchSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &MainWindow::handleMismatchChanged);

    m_overview->SetPositionActivatedCallback([this](int position) {
        m_alignment->CenterOnPosition(position);
    });
    m_overview->SetMatchActivatedCallback(
        [this](int readIndex, int matchIndex) {
            selectResult(readIndex, matchIndex, true);
        });
    m_alignment->SetMatchActivatedCallback(
        [this](int readIndex, int matchIndex) {
            selectResult(readIndex, matchIndex, false);
        });
    m_alignment->SetSelectionClearedCallback([this]() {
        clearSelection();
    });
    m_alignment->SetViewportChangedCallback(
        [this](int start, int end) {
            m_overview->SetViewportRange(start, end);
        });
    m_alignment->SetZoomPercentChangedCallback([this](int value) {
        m_zoomSlider->blockSignals(true);
        m_zoomSlider->setValue(value);
        m_zoomSlider->blockSignals(false);
    });
    connect(m_zoomSlider, &QSlider::valueChanged,
            m_alignment, &GenomeAlignmentWidget::SetZoomPercent);
    connect(fitButton, &QToolButton::clicked,
            m_alignment, &GenomeAlignmentWidget::FitWholeGenome);
    connect(resetButton, &QToolButton::clicked,
            m_alignment, &GenomeAlignmentWidget::ResetView);
    m_resultPanel->SetReadActivatedCallback(
        [this](int readIndex, int matchIndex) {
            selectResult(readIndex, matchIndex, true);
        });
    m_resultPanel->SetMatchActivatedCallback(
        [this](int readIndex, int matchIndex) {
            selectResult(readIndex, matchIndex, true);
        });
    m_resultPanel->SetAvailableReadActivatedCallback(
        [this](int readIndex) {
            selectAvailableRead(readIndex, false);
        });
    m_resultPanel->SetAvailableReadSearchRequestedCallback(
        [this](int readIndex) {
            selectAvailableRead(readIndex, true);
        });
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#central {
            background: #15191c;
            color: #dce4e7;
            font-family: "Microsoft YaHei UI", "Segoe UI";
            font-size: 12px;
        }
        QFrame#header {
            background: #111518;
            border-bottom: 1px solid #303a3f;
        }
        QLabel#appTitle {
            color: #f4f7f8;
            font-size: 19px;
            font-weight: 700;
        }
        QWidget#leftPanel, QWidget#resultPanel {
            background: #1b2023;
        }
        QScrollArea#sideScroll {
            background: #1b2023;
            border-right: 1px solid #303a3f;
        }
        QWidget#centerPanel { background: #171b1e; }
        QLabel#sectionLabel, QLabel#panelTitle {
            color: #eef3f5;
            font-size: 13px;
            font-weight: 700;
        }
        QLabel#parameterLabel {
            color: #b3bfc4;
            font-size: 11px;
            font-weight: 600;
        }
        QLabel#parameterRange {
            color: #7d8a90;
            font-size: 9px;
            font-weight: 400;
        }
        QLabel#currentReadHint {
            color: #7d8a90;
            font-size: 9px;
            font-weight: 400;
        }
        QLabel#subtleText, QLabel#legend {
            color: #8e9ba1;
            font-size: 10px;
        }
        QFrame#separator {
            color: #343e43;
            margin-top: 3px;
            margin-bottom: 3px;
        }
        QLineEdit, QPlainTextEdit {
            background: #22292d;
            color: #e4eaed;
            border: 1px solid #3a464c;
            border-radius: 5px;
            min-height: 32px;
            padding: 0 8px;
            selection-background-color: #2b8277;
        }
        QLineEdit:focus, QPlainTextEdit:focus {
            border: 1px solid #43aa9d;
        }
        QWidget#spinEditor {
            background: #22292d;
            border: 1px solid #3a464c;
            border-radius: 5px;
            min-height: 34px;
        }
        QSpinBox#flatSpinBox {
            background: transparent;
            color: #e4eaed;
            border: none;
            min-height: 32px;
            padding: 0 7px;
        }
        QToolButton#stepButton {
            background: #293237;
            color: #dce4e7;
            border: none;
            border-left: 1px solid #3a464c;
            min-width: 27px;
            max-width: 27px;
            min-height: 34px;
            font-size: 15px;
            font-weight: 700;
        }
        QToolButton#stepButton:hover {
            background: #35514f;
            color: #6ed1c4;
        }
        QPushButton, QToolButton#viewButton {
            background: #242c30;
            color: #dce4e7;
            border: 1px solid #3d494f;
            border-radius: 5px;
            min-height: 32px;
            padding: 0 10px;
            font-weight: 600;
        }
        QPushButton:hover, QToolButton#viewButton:hover {
            background: #2c373b;
            border-color: #5a6a71;
        }
        QPushButton:pressed, QToolButton#viewButton:pressed {
            background: #20272a;
        }
        QPushButton#accentButton, QPushButton#primaryButton {
            background: #187e72;
            color: #ffffff;
            border-color: #23998b;
        }
        QPushButton#accentButton:hover, QPushButton#primaryButton:hover {
            background: #209589;
        }
        QPushButton#secondaryAccentButton {
            background: #2d5f82;
            color: #ffffff;
            border-color: #3e789f;
        }
        QPushButton#secondaryAccentButton:hover {
            background: #37779f;
            border-color: #62a7cf;
        }
        QPushButton#secondaryAccentButton:pressed {
            background: #285573;
            border-color: #4d8db5;
        }
        QTableWidget {
            background: #1d2326;
            color: #dbe3e6;
            alternate-background-color: #22292d;
            border: 1px solid #343f44;
            border-radius: 4px;
            gridline-color: #303a3f;
            selection-background-color: #294f63;
            selection-color: #ffffff;
        }
        QTabWidget#rightPanelTabs::pane {
            background: transparent;
            border: none;
            border-top: 1px solid #354046;
        }
        QTabWidget#rightPanelTabs QTabBar::tab {
            background: #20272a;
            color: #8f9da3;
            border: none;
            border-bottom: 2px solid transparent;
            min-width: 92px;
            min-height: 30px;
            padding: 0 8px;
            font-weight: 600;
        }
        QTabWidget#rightPanelTabs QTabBar::tab:hover {
            color: #d9e3e6;
            background: #252e32;
        }
        QTabWidget#rightPanelTabs QTabBar::tab:selected {
            color: #63c9bc;
            background: #1b2023;
            border-bottom-color: #3fa99c;
        }
        QFrame#readDetails {
            background: #20272a;
            border: 1px solid #343f44;
            border-radius: 4px;
        }
        QLabel#readSequenceDetail {
            color: #e3e9eb;
            font-family: "Cascadia Mono", "Consolas";
            font-size: 10px;
        }
        QLabel#readMetadata {
            color: #95a4aa;
            font-size: 10px;
        }
        QHeaderView::section {
            background: #252d31;
            color: #aebbc0;
            border: none;
            border-right: 1px solid #354046;
            border-bottom: 1px solid #354046;
            padding: 6px;
            font-weight: 650;
        }
        QWidget#metricLine {
            background: #20272a;
            border-bottom: 1px solid #303a3f;
        }
        QLabel#metricName { color: #8f9ca2; font-size: 10px; }
        QLabel#metricValue { color: #e3e9eb; font-weight: 700; }
        QProgressBar { border: none; background: #2d363a; }
        QProgressBar::chunk { background: #e5a13a; }
        QSlider::groove:horizontal {
            height: 4px;
            background: #364146;
            border-radius: 2px;
        }
        QWidget#zoomControl {
            background: #171b1e;
            border: none;
        }
        QSlider#zoomSlider {
            min-height: 30px;
            padding-left: 7px;
            padding-right: 7px;
        }
        QSlider::sub-page:horizontal {
            background: #3fa99c;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #dbe6e8;
            border: 2px solid #3fa99c;
            width: 13px;
            margin: -6px 0;
            border-radius: 7px;
        }
        QScrollBar:horizontal {
            background: #1a2023;
            height: 11px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #4b5a61;
            min-width: 28px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal:hover { background: #65767e; }
        QScrollBar:vertical {
            background: #1a2023;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #4b5a61;
            min-height: 28px;
            border-radius: 4px;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            width: 0;
            height: 0;
        }
        QFrame#logPanel {
            background: #111518;
            border-top: 1px solid #303a3f;
        }
        QLabel#logTitle {
            color: #aebbc0;
            font-size: 10px;
            font-weight: 700;
        }
        QPlainTextEdit#logView {
            background: transparent;
            color: #8fa0a7;
            border: none;
            font-family: "Cascadia Mono", "Consolas";
            font-size: 10px;
            padding: 0;
        }
        QStatusBar {
            background: #111518;
            color: #809097;
            border-top: 1px solid #303a3f;
        }
        QSplitter::handle { background: #303a3f; }
        QSplitter#workspaceSplitter::handle {
            background: #303a3f;
        }
        QSplitter#workspaceSplitter::handle:hover {
            background: #3fa99c;
        }
        QToolTip {
            background: #101417;
            color: #f0f4f5;
            border: 1px solid #536168;
            padding: 6px;
        }
    )"));
}

bool MainWindow::validateCurrentParameters(QString *error) const
{
    if (m_referenceLengthSpin->value()
            < ApplicationLimits::MinReferenceLength
        || m_referenceLengthSpin->value()
            > ApplicationLimits::MaxReferenceLength) {
        *error = QStringLiteral("参考长度必须在 %1-%2 bp 之间")
                     .arg(ApplicationLimits::MinReferenceLength)
                     .arg(ApplicationLimits::MaxReferenceLength);
        return false;
    }
    if (m_kSpin->value() < ApplicationLimits::MinKmerLength
        || m_kSpin->value() > ApplicationLimits::MaxKmerLength) {
        *error = QStringLiteral("K-mer 长度 K 必须在 %1-%2 之间")
                     .arg(ApplicationLimits::MinKmerLength)
                     .arg(ApplicationLimits::MaxKmerLength);
        return false;
    }
    if (m_mismatchSpin->value()
            < ApplicationLimits::MinMismatchCount
        || m_mismatchSpin->value()
            > ApplicationLimits::MaxMismatchCount) {
        *error = QStringLiteral("允许错配数必须在 %1-%2 之间")
                     .arg(ApplicationLimits::MinMismatchCount)
                     .arg(ApplicationLimits::MaxMismatchCount);
        return false;
    }
    if (m_readLengthSpin->value() < ApplicationLimits::MinReadLength
        || m_readLengthSpin->value()
            > ApplicationLimits::MaxReadLength) {
        *error = QStringLiteral("Read 长度必须在 %1-%2 bp 之间")
                     .arg(ApplicationLimits::MinReadLength)
                     .arg(ApplicationLimits::MaxReadLength);
        return false;
    }
    if (m_readCountSpin->value() < ApplicationLimits::MinReadCount
        || m_readCountSpin->value()
            > ApplicationLimits::MaxReadCount) {
        *error = QStringLiteral("Reads 数量必须在 %1-%2 条之间")
                     .arg(ApplicationLimits::MinReadCount)
                     .arg(ApplicationLimits::MaxReadCount);
        return false;
    }
    return true;
}

bool MainWindow::validateReadSequence(const QString &sequence,
                                      QString *error,
                                      int readNumber) const
{
    const QString subject =
        readNumber > 0
            ? QStringLiteral("第 %1 条 Read").arg(readNumber)
            : QStringLiteral("Read");
    if (sequence.size() < ApplicationLimits::MinReadLength
        || sequence.size() > ApplicationLimits::MaxReadLength) {
        *error = QStringLiteral("%1 长度必须在 %2-%3 bp 之间，当前为 %4 bp")
                     .arg(subject)
                     .arg(ApplicationLimits::MinReadLength)
                     .arg(ApplicationLimits::MaxReadLength)
                     .arg(sequence.size());
        return false;
    }
    if (!isDna(sequence)) {
        *error = QStringLiteral("%1 只能包含 A、T、C、G").arg(subject);
        return false;
    }
    if (!m_referenceSequence.isEmpty()
        && sequence.size() > m_referenceSequence.size()) {
        *error = QStringLiteral("%1 不能长于参考基因组").arg(subject);
        return false;
    }
    return true;
}

void MainWindow::importReference()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入参考基因组"),
        QString(),
        QStringLiteral("DNA / FASTA (*.txt *.fa *.fasta *.fna);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }

    QString sequence;
    QString error;
    if (!SequenceFileIO::LoadReference(path, &sequence, &error)) {
        showFeedback(error, true);
        appendLog(error, true);
        return;
    }
    if (sequence.size() < ApplicationLimits::MinReferenceLength
        || sequence.size() > ApplicationLimits::MaxReferenceLength) {
        const QString message =
            QStringLiteral("参考基因组长度必须为 %1-%2 bp，当前为 %3 bp")
                .arg(ApplicationLimits::MinReferenceLength)
                .arg(ApplicationLimits::MaxReferenceLength)
                .arg(sequence.size());
        showFeedback(message, true);
        appendLog(message, true);
        return;
    }

    delete m_engine;
    m_engine = nullptr;
    m_referenceSequence = sequence;
    m_referenceLengthSpin->setValue(
        static_cast<int>(sequence.size()));
    m_readInputs.clear();
    m_currentReadInputIndex = -1;
    m_session.SetReference(sequence);
    m_alignment->ClearLayout();
    resetIndexMetrics();
    refreshViews();
    showFeedback(QStringLiteral("参考基因组已导入，请生成或导入 Reads 并建立索引"));
    appendLog(QStringLiteral("导入参考基因组：%1 bp").arg(sequence.size()));
}

void MainWindow::importReads()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入 Reads"),
        QString(),
        QStringLiteral("DNA / FASTA (*.txt *.fa *.fasta);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }

    QVector<QString> reads;
    QString error;
    if (!SequenceFileIO::LoadReads(path, &reads, &error)) {
        showFeedback(error, true);
        appendLog(error, true);
        return;
    }
    if (reads.size() < ApplicationLimits::MinReadCount
        || reads.size() > ApplicationLimits::MaxReadCount) {
        error = QStringLiteral("Reads 数量必须在 %1-%2 条之间，当前为 %3 条")
                    .arg(ApplicationLimits::MinReadCount)
                    .arg(ApplicationLimits::MaxReadCount)
                    .arg(reads.size());
    }

    if (error.isEmpty()) {
        for (int index = 0; index < reads.size(); ++index) {
            if (!validateReadSequence(reads.at(index),
                                      &error,
                                      index + 1)) {
                break;
            }
        }
    }
    if (!error.isEmpty()) {
        showFeedback(error, true);
        appendLog(error, true);
        return;
    }

    m_readInputs.clear();
    m_readInputs.reserve(reads.size());
    for (int index = 0; index < reads.size(); ++index) {
        ReadInputViewData input;
        input.readId = index + 1;
        input.sequence = reads.at(index);
        m_readInputs.append(input);
    }
    m_currentReadInputIndex = m_readInputs.isEmpty() ? -1 : 0;
    m_readCountSpin->setValue(static_cast<int>(reads.size()));
    clearResults();
    if (!m_readInputs.isEmpty()) {
        m_readInput->setText(m_readInputs.first().sequence);
    }
    m_resultPanel->ShowReadsTab();
    showFeedback(QStringLiteral("已导入 %1 条 Reads").arg(reads.size()));
    appendLog(QStringLiteral("导入 Reads：%1 条").arg(reads.size()));
}

void MainWindow::exportResults()
{
    if (m_session.Reads().isEmpty()) {
        showFeedback(QStringLiteral("没有可导出的检索结果"), true);
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出检索结果"),
        QStringLiteral("DNA_Search_Results.csv"),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!SequenceFileIO::ExportResults(path, m_session, &error)) {
        showFeedback(error, true);
        appendLog(error, true);
        return;
    }
    showFeedback(QStringLiteral("结果已导出"));
    appendLog(QStringLiteral("导出结果：%1").arg(path));
}

void MainWindow::buildIndex()
{
    QString validationError;
    if (!validateCurrentParameters(&validationError)) {
        showFeedback(validationError, true);
        appendLog(validationError, true);
        return;
    }
    if (m_referenceSequence.isEmpty()) {
        validationError = QStringLiteral("请先生成或导入参考基因组");
        showFeedback(validationError, true);
        appendLog(validationError, true);
        return;
    }
    if (m_referenceSequence.size()
                < ApplicationLimits::MinReferenceLength
            || m_referenceSequence.size()
                > ApplicationLimits::MaxReferenceLength) {
        validationError =
            QStringLiteral("参考基因组长度必须在 %1-%2 bp 之间")
                .arg(ApplicationLimits::MinReferenceLength)
                .arg(ApplicationLimits::MaxReferenceLength);
        showFeedback(validationError, true);
        appendLog(validationError, true);
        return;
    }
    appendLog(
        QStringLiteral("开始构建索引：K=%1，允许错配=%2")
            .arg(m_kSpin->value())
            .arg(m_mismatchSpin->value()));
    try {
        if (m_referenceSequence.isEmpty()) {
            throw std::invalid_argument("Reference genome is empty");
        }
        const QByteArray bases = m_referenceSequence.toLatin1();
        Genome reference(
            bases.constData(), static_cast<int>(bases.size()));
        SearchEngine *replacement =
            new SearchEngine(reference,
                             m_kSpin->value(),
                             m_mismatchSpin->value());
        delete m_engine;
        m_engine = replacement;
        m_session.SetReference(m_referenceSequence);
        m_session.SetMaxMismatch(m_engine->GetMaxMismatch());
        m_alignment->ClearLayout();
        m_searchMetric->setText(QStringLiteral("—"));
        m_batchProgress->setValue(0);
        updateIndexMetrics();
        refreshViews();
        showFeedback(QStringLiteral("索引构建完成"));
        appendLog(
            QStringLiteral("索引构建完成：K=%1，容量=%2，位置数=%3，耗时=%4 μs")
                .arg(m_engine->GetK())
                .arg(m_engine->GetHashCapacity())
                .arg(m_engine->GetIndexedPositionCount())
                .arg(m_engine->GetIndexBuildMicroseconds()));
    } catch (const std::exception &error) {
        showFeedback(QString::fromLocal8Bit(error.what()), true);
        appendLog(QString::fromLocal8Bit(error.what()), true);
    }
}

bool MainWindow::validateIndexReady(QString *error) const
{
    if (m_referenceSequence.isEmpty()) {
        *error = QStringLiteral("请先生成或导入参考基因组");
        return false;
    }
    if (m_engine == nullptr) {
        *error = QStringLiteral("当前 Reference 尚未建立索引，请先点击“建立索引”");
        return false;
    }
    if (m_engine->GetK() != m_kSpin->value()
        || m_engine->GetReferenceGenome().Length()
               != m_referenceSequence.size()) {
        *error = QStringLiteral("当前索引已失效，请重新点击“建立索引”");
        return false;
    }
    const Genome &reference = m_engine->GetReferenceGenome();
    for (int index = 0; index < reference.Length(); ++index) {
        if (reference.Bases()[index]
            != m_referenceSequence.at(index).toLatin1()) {
            *error = QStringLiteral("当前索引已失效，请重新点击“建立索引”");
            return false;
        }
    }
    return true;
}

void MainWindow::generateReference()
{
    QString validationError;
    if (!validateCurrentParameters(&validationError)) {
        showFeedback(validationError, true);
        appendLog(validationError, true);
        return;
    }
    try {
        const unsigned int seed =
            static_cast<unsigned int>(QDateTime::currentMSecsSinceEpoch());
        Genome reference =
            ReadGenerator::GenerateReference(m_referenceLengthSpin->value(),
                                             seed);
        m_referenceSequence =
            QString::fromLatin1(reference.Bases(), reference.Length());
        delete m_engine;
        m_engine = nullptr;
        m_session.SetReference(m_referenceSequence);
        m_session.SetMaxMismatch(m_mismatchSpin->value());
        m_readInputs.clear();
        m_currentReadInputIndex = -1;
        m_readInput->clear();
        m_alignment->ClearLayout();
        resetIndexMetrics();
        refreshViews();
        showFeedback(
            QStringLiteral("参考基因组已生成，请随机生成或导入 Reads，并建立索引"));
        appendLog(
            QStringLiteral("生成参考基因组：%1 bp；旧 Reads、索引和结果已清空")
                .arg(m_referenceSequence.size()));
    } catch (const std::exception &error) {
        showFeedback(QString::fromLocal8Bit(error.what()), true);
        appendLog(QString::fromLocal8Bit(error.what()), true);
    }
}

void MainWindow::generateReads()
{
    QString validationError;
    if (!validateCurrentParameters(&validationError)) {
        showFeedback(validationError, true);
        appendLog(validationError, true);
        return;
    }
    if (m_referenceSequence.isEmpty()) {
        validationError = QStringLiteral("请先生成或导入参考基因组");
        showFeedback(validationError, true);
        appendLog(validationError, true);
        return;
    }

    try {
        const QByteArray referenceBases = m_referenceSequence.toLatin1();
        const Genome reference(referenceBases.constData(),
                               static_cast<int>(referenceBases.size()));
        const unsigned int seed =
            static_cast<unsigned int>(QDateTime::currentMSecsSinceEpoch());
        const ReadBatch reads =
            ReadGenerator::Generate(reference,
                                    m_readCountSpin->value(),
                                    m_readLengthSpin->value(),
                                    m_kSpin->value(),
                                    seed);
        m_readInputs.clear();
        m_readInputs.reserve(reads.Count());
        for (int index = 0; index < reads.Count(); ++index) {
            const ReadRecord &record = reads.At(index);
            ReadInputViewData input;
            input.readId = index + 1;
            input.sequence =
                QString::fromLatin1(record.Sequence().Bases(),
                                    record.Sequence().Length());
            input.sourcePosition = record.SourcePosition();
            input.mutations.reserve(record.MutationCount());
            for (int mutationIndex = 0;
                 mutationIndex < record.MutationCount();
                 ++mutationIndex) {
                ReadMutationViewData mutation;
                mutation.offset = record.MutationOffset(mutationIndex);
                mutation.absolutePosition =
                    input.sourcePosition + mutation.offset;
                mutation.referenceBase =
                    m_referenceSequence.at(mutation.absolutePosition);
                mutation.readBase = input.sequence.at(mutation.offset);
                input.mutations.append(mutation);
            }
            m_readInputs.append(input);
        }
        m_currentReadInputIndex = m_readInputs.isEmpty() ? -1 : 0;
        m_session.ClearResults();
        m_alignment->ClearLayout();
        m_batchProgress->setValue(0);
        m_searchMetric->setText(QStringLiteral("—"));
        if (!m_readInputs.isEmpty()) {
            m_readInput->setText(m_readInputs.first().sequence);
        }
        refreshViews();
        m_resultPanel->ShowReadsTab();
        showFeedback(
            QStringLiteral("已根据当前 Reference 生成 %1 条 Reads")
                .arg(m_readInputs.size()));
        appendLog(
            QStringLiteral("随机生成 Reads：%1 条，每条 %2 bp，保护前 %3 位种子")
                .arg(m_readInputs.size())
                .arg(m_readLengthSpin->value())
                .arg(m_kSpin->value()));
    } catch (const std::exception &error) {
        showFeedback(QString::fromLocal8Bit(error.what()), true);
        appendLog(QString::fromLocal8Bit(error.what()), true);
    }
}

void MainWindow::runSingleSearch()
{
    if (m_searchRunning) {
        return;
    }
    m_searchRunning = true;
    setOperationControlsEnabled(false);
    ScopeExit operationFinished([this]() {
        m_searchRunning = false;
        setOperationControlsEnabled(true);
    });

    try {
        const QString sequence = m_readInput->text().trimmed().toUpper();
        QString validationError;
        if (!validateReadSequence(sequence, &validationError)) {
            showFeedback(validationError, true);
            appendLog(validationError, true);
            return;
        }
        if (!validateIndexReady(&validationError)) {
            showFeedback(validationError, true);
            appendLog(validationError, true);
            return;
        }
        if (sequence.size() < m_engine->GetK()) {
            throw std::invalid_argument("Read length must be at least K");
        }
        if (sequence.size() > m_referenceSequence.size()) {
            throw std::invalid_argument(
                "Read cannot be longer than the reference genome");
        }

        const QByteArray bases = sequence.toLatin1();
        appendLog(
            QStringLiteral("开始执行检索：单条 Read，长度 %1 bp")
                .arg(sequence.size()));
        m_engine->Search(
            Genome(bases.constData(), static_cast<int>(bases.size())));
        int readId = 0;
        int sourcePosition = -1;
        if (m_currentReadInputIndex < 0
            || m_currentReadInputIndex >= m_readInputs.size()
            || m_readInputs.at(m_currentReadInputIndex).sequence
                   != sequence) {
            m_currentReadInputIndex = -1;
            for (int index = 0; index < m_readInputs.size(); ++index) {
                if (m_readInputs.at(index).sequence == sequence) {
                    m_currentReadInputIndex = index;
                    break;
                }
            }
        }
        if (m_currentReadInputIndex >= 0
            && m_currentReadInputIndex < m_readInputs.size()
            && m_readInputs.at(m_currentReadInputIndex).sequence
                   == sequence) {
            const ReadInputViewData &input =
                m_readInputs.at(m_currentReadInputIndex);
            readId = input.readId;
            sourcePosition = input.sourcePosition;
        }
        const int searchedAvailableReadIndex =
            m_currentReadInputIndex;
        m_session.AddResult(readId,
                            sourcePosition,
                            m_engine->GetLastResult());
        const int resultIndex = m_session.FindReadIndexById(readId);
        const ReadResultViewData *resultView =
            m_session.ReadAt(resultIndex);
        const int focusPosition =
            resultView != nullptr && resultView->IsMatched()
                ? resultView->matches.first().start
                    + resultView->matches.first().length / 2
                : -1;
        m_alignment->RebuildLayout();
        m_session.ClearSelection();
        m_currentReadInputIndex = searchedAvailableReadIndex;
        m_searchMetric->setText(
            QStringLiteral("%1 μs")
                .arg(m_engine->GetLastResult().ElapsedMicroseconds(),
                     0, 'f', 2));
        refreshViews();
        m_resultPanel->ShowReadsTab();
        if (focusPosition >= 0) {
            m_alignment->CenterOnPosition(focusPosition);
        }

        const SearchResult &result = m_engine->GetLastResult();
        showFeedback(
            QStringLiteral("检索完成：%1 个候选，%2 个匹配")
                .arg(result.CandidateCount())
                .arg(result.MatchCount()));
        appendLog(
            QStringLiteral("检索完成：单条 Read，候选 %1，成功 %2，耗时 %3 μs")
                .arg(result.CandidateCount())
                .arg(result.MatchCount())
                .arg(result.ElapsedMicroseconds(), 0, 'f', 2));
    } catch (const std::exception &error) {
        showFeedback(QString::fromLocal8Bit(error.what()), true);
        appendLog(QString::fromLocal8Bit(error.what()), true);
    }
}

void MainWindow::runBatchSearch()
{
    if (m_searchRunning) {
        return;
    }
    m_searchRunning = true;
    setOperationControlsEnabled(false);
    ScopeExit operationFinished([this]() {
        m_searchRunning = false;
        setOperationControlsEnabled(true);
    });

    try {
        QString validationError;
        if (!validateCurrentParameters(&validationError)) {
            showFeedback(validationError, true);
            appendLog(validationError, true);
            return;
        }
        if (m_readInputs.isEmpty()) {
            validationError = QStringLiteral("请先随机生成或导入 Reads");
            showFeedback(validationError, true);
            appendLog(validationError, true);
            return;
        }
        if (!validateIndexReady(&validationError)) {
            showFeedback(validationError, true);
            appendLog(validationError, true);
            return;
        }
        if (m_readInputs.size() < ApplicationLimits::MinReadCount
            || m_readInputs.size() > ApplicationLimits::MaxReadCount) {
            validationError =
                QStringLiteral("Reads 数量必须在 %1-%2 条之间，当前为 %3 条")
                    .arg(ApplicationLimits::MinReadCount)
                    .arg(ApplicationLimits::MaxReadCount)
                    .arg(m_readInputs.size());
            showFeedback(validationError, true);
            appendLog(validationError, true);
            return;
        }
        for (int index = 0; index < m_readInputs.size(); ++index) {
            if (!validateReadSequence(m_readInputs.at(index).sequence,
                                      &validationError,
                                      index + 1)) {
                showFeedback(validationError, true);
                appendLog(validationError, true);
                return;
            }
        }

        m_session.ClearResults();
        m_alignment->ClearLayout();
        m_batchProgress->setRange(
            0, static_cast<int>(m_readInputs.size()));
        m_batchProgress->setValue(0);
        double totalMicroseconds = 0.0;
        int matchedReads = 0;
        appendLog(
            QStringLiteral("开始执行检索：批量 %1 条 Reads")
                .arg(m_readInputs.size()));

        for (int index = 0; index < m_readInputs.size(); ++index) {
            const ReadInputViewData &input = m_readInputs.at(index);
            const QString &sequence = input.sequence;
            const QByteArray bases = sequence.toLatin1();
            m_engine->Search(
                Genome(bases.constData(),
                       static_cast<int>(bases.size())));
            m_session.AddResult(input.readId,
                                input.sourcePosition,
                                m_engine->GetLastResult());
            if (m_engine->GetLastResult().MatchCount() > 0) {
                ++matchedReads;
            }
            totalMicroseconds +=
                m_engine->GetLastResult().ElapsedMicroseconds();
            m_batchProgress->setValue(index + 1);
            if ((index + 1) % 10 == 0) {
                QApplication::processEvents();
            }
        }

        m_alignment->RebuildLayout();
        m_session.ClearSelection();
        m_currentReadInputIndex = -1;
        m_searchMetric->setText(
            QStringLiteral("%1 μs")
                .arg(totalMicroseconds, 0, 'f', 2));
        refreshViews();
        m_resultPanel->ShowResultsTab();
        m_readInput->clear();
        showFeedback(
            QStringLiteral("批量完成：%1 / %2 条匹配")
                .arg(matchedReads)
                .arg(m_readInputs.size()),
            matchedReads != m_readInputs.size());
        appendLog(
            QStringLiteral("检索完成：批量 %1/%2 条成功，总耗时 %3 μs")
                .arg(matchedReads)
                .arg(m_readInputs.size())
                .arg(totalMicroseconds, 0, 'f', 2),
            matchedReads != m_readInputs.size());
    } catch (const std::exception &error) {
        showFeedback(QString::fromLocal8Bit(error.what()), true);
        appendLog(QString::fromLocal8Bit(error.what()), true);
    }
}

void MainWindow::clearResults()
{
    m_session.ClearResults();
    m_alignment->ClearLayout();
    m_batchProgress->setValue(0);
    m_searchMetric->setText(QStringLiteral("—"));
    refreshViews();
    showFeedback(QStringLiteral("检索结果已清空"));
    appendLog(QStringLiteral("清空检索结果"));
}

void MainWindow::clearSelection()
{
    m_session.ClearSelection();
    m_currentReadInputIndex = -1;
    updateCurrentReadHint();
    m_overview->Refresh();
    m_alignment->Refresh();
    m_detail->Refresh();
    m_resultPanel->SetCurrentAvailableReadIndex(-1);
    m_resultPanel->Refresh();
}

void MainWindow::handleKChanged(int value)
{
    Q_UNUSED(value)
    delete m_engine;
    m_engine = nullptr;
    m_session.ClearResults();
    m_alignment->ClearLayout();
    m_batchProgress->setValue(0);
    m_searchMetric->setText(QStringLiteral("—"));
    resetIndexMetrics();
    refreshViews();
    showFeedback(QStringLiteral("K 已改变，请重新点击“建立索引”"));
    appendLog(QStringLiteral("K 已改变：旧索引和检索结果已失效"));
}

void MainWindow::handleMismatchChanged(int value)
{
    if (m_engine != nullptr) {
        m_engine->SetMaxMismatch(value);
    }
    m_session.SetMaxMismatch(value);
    m_session.ClearResults();
    m_alignment->ClearLayout();
    m_batchProgress->setValue(0);
    m_searchMetric->setText(QStringLiteral("—"));
    refreshViews();
    showFeedback(QStringLiteral("允许错配数已改变，索引继续复用"));
    appendLog(QStringLiteral("允许错配数已改变：检索结果已清空，索引保留"));
}

void MainWindow::setOperationControlsEnabled(bool enabled)
{
    m_importReferenceButton->setEnabled(enabled);
    m_importReadsButton->setEnabled(enabled);
    m_exportButton->setEnabled(enabled);
    m_generateReferenceButton->setEnabled(enabled);
    m_generateReadsButton->setEnabled(enabled);
    m_buildIndexButton->setEnabled(enabled);
    m_batchButton->setEnabled(enabled);
    m_clearButton->setEnabled(enabled);
    m_singleSearchButton->setEnabled(enabled);
    m_referenceLengthSpin->setEnabled(enabled);
    m_kSpin->setEnabled(enabled);
    m_mismatchSpin->setEnabled(enabled);
    m_readLengthSpin->setEnabled(enabled);
    m_readCountSpin->setEnabled(enabled);
    m_resultPanel->setEnabled(enabled);
}

void MainWindow::selectAvailableRead(int index, bool executeSearch)
{
    if (index < 0 || index >= m_readInputs.size()) {
        return;
    }

    m_currentReadInputIndex = index;
    const ReadInputViewData &input = m_readInputs.at(index);
    m_readInput->setText(input.sequence);

    const int resultIndex =
        m_session.FindReadIndexById(input.readId);
    if (resultIndex >= 0) {
        const ReadResultViewData *result =
            m_session.ReadAt(resultIndex);
        selectResult(resultIndex,
                     result != nullptr && result->IsMatched() ? 0 : -1,
                     true);
    } else {
        m_session.ClearSelection();
        refreshViews();
    }

    if (executeSearch) {
        runSingleSearch();
    }
}

void MainWindow::selectResult(int readIndex,
                              int matchIndex,
                              bool centerView)
{
    if (!m_session.Select(readIndex, matchIndex)) {
        return;
    }
    const ReadResultViewData *read = m_session.SelectedRead();
    if (read != nullptr) {
        m_currentReadInputIndex =
            findAvailableReadIndexById(read->readId);
        m_readInput->setText(read->sequence);
        updateCurrentReadHint();
    }
    m_overview->Refresh();
    m_alignment->Refresh();
    m_detail->Refresh();
    m_resultPanel->SetCurrentAvailableReadIndex(
        m_currentReadInputIndex);
    m_resultPanel->Refresh();
    if (centerView && matchIndex >= 0) {
        m_alignment->FocusSelectedMatch();
    }
}

int MainWindow::findAvailableReadIndexById(int readId) const
{
    for (int index = 0; index < m_readInputs.size(); ++index) {
        if (m_readInputs.at(index).readId == readId) {
            return index;
        }
    }
    return -1;
}

void MainWindow::updateCurrentReadHint()
{
    if (m_currentReadLabel == nullptr) {
        return;
    }
    if (m_currentReadInputIndex >= 0
        && m_currentReadInputIndex < m_readInputs.size()) {
        m_currentReadLabel->setText(
            QStringLiteral("当前：Read #%1")
                .arg(m_readInputs.at(m_currentReadInputIndex).readId));
        return;
    }
    m_currentReadLabel->setText(
        m_readInput != nullptr && !m_readInput->text().isEmpty()
            ? QStringLiteral("当前：手动输入")
            : QStringLiteral("当前：未选择"));
}

void MainWindow::refreshViews()
{
    updateCurrentReadHint();
    m_overview->SetSession(&m_session);
    m_alignment->SetSession(&m_session);
    m_detail->SetSession(&m_session);
    m_resultPanel->SetAvailableReads(m_readInputs);
    m_resultPanel->SetCurrentAvailableReadIndex(
        m_currentReadInputIndex);
    m_resultPanel->SetSession(&m_session);
    m_overview->SetViewportRange(m_alignment->ViewportStart(),
                                 m_alignment->ViewportEnd());
}

void MainWindow::updateIndexMetrics()
{
    if (m_engine == nullptr) {
        resetIndexMetrics();
        return;
    }
    m_referenceMetric->setText(
        QStringLiteral("%1 bp")
            .arg(m_engine->GetReferenceGenome().Length()));
    m_kmerMetric->setText(
        QString::number(m_engine->GetIndexedKmerCount()));
    m_buildMetric->setText(
        QStringLiteral("%1 μs")
            .arg(m_engine->GetIndexBuildMicroseconds()));
    m_loadMetric->setText(
        QString::number(m_engine->GetLoadFactor(), 'f', 3));
}

void MainWindow::resetIndexMetrics()
{
    m_referenceMetric->setText(
        m_referenceSequence.isEmpty()
            ? QStringLiteral("—")
            : QStringLiteral("%1 bp").arg(m_referenceSequence.size()));
    m_kmerMetric->setText(QStringLiteral("—"));
    m_buildMetric->setText(QStringLiteral("—"));
    m_loadMetric->setText(QStringLiteral("—"));
    m_searchMetric->setText(QStringLiteral("—"));
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_snackbarHost != nullptr && centralWidget() != nullptr) {
        m_snackbarHost->setGeometry(centralWidget()->rect());
        m_snackbarHost->raise();
    }
}

void MainWindow::showFeedback(const QString &message, bool error)
{
    if (m_snackbarHost == nullptr || centralWidget() == nullptr) {
        return;
    }
    m_snackbarHost->setGeometry(centralWidget()->rect());
    m_snackbarHost->ShowMessage(message, error);
}

void MainWindow::appendLog(const QString &message, bool error)
{
    const QString time =
        QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_logView->appendPlainText(
        QStringLiteral("[%1] %2%3")
            .arg(time)
            .arg(error ? QStringLiteral("错误：") : QString())
            .arg(message));
}
