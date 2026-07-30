#include "SnackbarHost.h"

#include <QEasingCurve>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTimer>

#include <functional>
#include <utility>

class SnackbarItem final : public QFrame
{
public:
    SnackbarItem(const QString &message, bool error, QWidget *parent)
        : QFrame(parent),
          m_message(message),
          m_error(error),
          m_opacityEffect(new QGraphicsOpacityEffect(this)),
          m_moveAnimation(new QPropertyAnimation(this, "pos", this)),
          m_opacityAnimation(
              new QPropertyAnimation(m_opacityEffect, "opacity", this)),
          m_lifetimeTimer(new QTimer(this)),
          m_dismissing(false)
    {
        setObjectName(QStringLiteral("snackbar"));
        setFixedWidth(460);
        setGraphicsEffect(m_opacityEffect);
        setStyleSheet(error
            ? QStringLiteral(
                  "QFrame#snackbar {"
                  "background:#2b2224;"
                  "border:1px solid #b9505a;"
                  "border-radius:6px;"
                  "}"
                  "QLabel#snackbarIcon {"
                  "color:#ff7d87;font-size:16px;font-weight:700;"
                  "}"
                  "QLabel#snackbarText {"
                  "color:#f3dfe1;font-size:11px;font-weight:600;"
                  "}")
            : QStringLiteral(
                  "QFrame#snackbar {"
                  "background:#202a2b;"
                  "border:1px solid #319d90;"
                  "border-radius:6px;"
                  "}"
                  "QLabel#snackbarIcon {"
                  "color:#62d3c4;font-size:16px;font-weight:700;"
                  "}"
                  "QLabel#snackbarText {"
                  "color:#e2f1ef;font-size:11px;font-weight:600;"
                  "}"));

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(14, 9, 16, 9);
        layout->setSpacing(10);

        QLabel *icon = new QLabel(
            error ? QStringLiteral("!") : QStringLiteral("✓"));
        icon->setObjectName(QStringLiteral("snackbarIcon"));
        icon->setAlignment(Qt::AlignCenter);
        icon->setFixedWidth(18);

        QLabel *text = new QLabel(message);
        text->setObjectName(QStringLiteral("snackbarText"));
        text->setWordWrap(true);
        text->setTextInteractionFlags(Qt::NoTextInteraction);

        layout->addWidget(icon);
        layout->addWidget(text, 1);
        layout->activate();
        setFixedHeight(qMax(50, sizeHint().height()));

        m_moveAnimation->setDuration(220);
        m_moveAnimation->setEasingCurve(QEasingCurve::OutCubic);
        m_opacityAnimation->setDuration(190);
        m_opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
        m_lifetimeTimer->setSingleShot(true);
        connect(m_opacityAnimation,
                &QPropertyAnimation::finished,
                this,
                [this]() {
                    if (m_dismissing && m_dismissedCallback) {
                        const std::function<void()> callback =
                            std::move(m_dismissedCallback);
                        callback();
                    }
                });
        connect(m_lifetimeTimer, &QTimer::timeout, this, [this]() {
            if (!m_dismissing && m_expiredCallback) {
                m_expiredCallback();
            }
        });
    }

    bool Matches(const QString &message, bool error) const
    {
        return m_message == message && m_error == error;
    }

    void RestartLifetime(int milliseconds,
                         std::function<void()> expiredCallback)
    {
        m_expiredCallback = std::move(expiredCallback);
        m_lifetimeTimer->start(milliseconds);
    }

    bool IsDismissing() const
    {
        return m_dismissing;
    }

    void AnimateIn(const QPoint &target)
    {
        m_moveAnimation->stop();
        m_opacityAnimation->stop();
        move(-width(), target.y());
        m_opacityEffect->setOpacity(0.0);
        show();

        m_moveAnimation->setStartValue(pos());
        m_moveAnimation->setEndValue(target);
        m_opacityAnimation->setStartValue(0.0);
        m_opacityAnimation->setEndValue(1.0);
        m_moveAnimation->start();
        m_opacityAnimation->start();
    }

    void MoveTo(const QPoint &target)
    {
        if (m_dismissing || pos() == target) {
            return;
        }
        m_moveAnimation->stop();
        m_moveAnimation->setDuration(180);
        m_moveAnimation->setStartValue(pos());
        m_moveAnimation->setEndValue(target);
        m_moveAnimation->start();
    }

    void Dismiss(std::function<void()> callback)
    {
        if (m_dismissing) {
            return;
        }
        m_dismissing = true;
        m_dismissedCallback = std::move(callback);
        m_lifetimeTimer->stop();
        m_moveAnimation->stop();
        m_opacityAnimation->stop();

        m_moveAnimation->setDuration(260);
        m_moveAnimation->setEasingCurve(QEasingCurve::InCubic);
        m_moveAnimation->setStartValue(pos());
        m_moveAnimation->setEndValue(pos() + QPoint(0, -22));
        m_opacityAnimation->setDuration(240);
        m_opacityAnimation->setEasingCurve(QEasingCurve::InCubic);
        m_opacityAnimation->setStartValue(m_opacityEffect->opacity());
        m_opacityAnimation->setEndValue(0.0);
        m_moveAnimation->start();
        m_opacityAnimation->start();
    }

private:
    QString m_message;
    bool m_error;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPropertyAnimation *m_moveAnimation;
    QPropertyAnimation *m_opacityAnimation;
    QTimer *m_lifetimeTimer;
    bool m_dismissing;
    std::function<void()> m_expiredCallback;
    std::function<void()> m_dismissedCallback;
};

SnackbarHost::SnackbarHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setStyleSheet(QStringLiteral("background:transparent;"));
}

SnackbarHost::~SnackbarHost() = default;

void SnackbarHost::ShowMessage(const QString &message, bool error)
{
    const QString normalizedMessage = message.trimmed();
    if (normalizedMessage.isEmpty()) {
        return;
    }
    const int visibleMilliseconds = error ? 2000 : 1500;
    for (int index = 0; index < m_items.size(); ++index) {
        SnackbarItem *existing = m_items.at(index);
        if (!existing->Matches(normalizedMessage, error)) {
            continue;
        }
        m_items.removeAt(index);
        m_items.append(existing);
        existing->RestartLifetime(visibleMilliseconds,
                                  [this, existing]() {
                                      dismissItem(existing);
                                  });
        relayoutItems();
        raise();
        return;
    }
    while (m_items.size() >= MaxVisibleItems) {
        SnackbarItem *oldest = m_items.takeFirst();
        delete oldest;
    }

    SnackbarItem *item = new SnackbarItem(normalizedMessage, error, this);
    m_items.append(item);
    relayoutItems(item);
    raise();

    item->RestartLifetime(visibleMilliseconds, [this, item]() {
        dismissItem(item);
    });
}

void SnackbarHost::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayoutItems();
}

void SnackbarHost::relayoutItems(SnackbarItem *enteringItem)
{
    int y = height() - BottomMargin;
    for (int index = m_items.size() - 1; index >= 0; --index) {
        SnackbarItem *item = m_items.at(index);
        y -= item->height();
        const QPoint target(LeftMargin, y);
        if (item == enteringItem) {
            item->AnimateIn(target);
        } else {
            item->MoveTo(target);
        }
        y -= ItemSpacing;
    }
}

void SnackbarHost::dismissItem(SnackbarItem *item)
{
    if (item == nullptr || item->IsDismissing()) {
        return;
    }
    m_items.removeOne(item);
    item->Dismiss([this, item]() {
        item->deleteLater();
    });
    relayoutItems();
}
