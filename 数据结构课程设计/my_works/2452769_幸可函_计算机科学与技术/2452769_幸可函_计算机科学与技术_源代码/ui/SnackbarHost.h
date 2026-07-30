#ifndef SNACKBARHOST_H
#define SNACKBARHOST_H

#include <QList>
#include <QString>
#include <QWidget>

class SnackbarItem;

class SnackbarHost final : public QWidget
{
public:
    explicit SnackbarHost(QWidget *parent = nullptr);
    ~SnackbarHost() override;

    void ShowMessage(const QString &message, bool error);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    static constexpr int MaxVisibleItems = 5;
    static constexpr int LeftMargin = 16;
    static constexpr int BottomMargin = 16;
    static constexpr int ItemSpacing = 9;

    QList<SnackbarItem *> m_items;

    void relayoutItems(SnackbarItem *enteringItem = nullptr);
    void dismissItem(SnackbarItem *item);
};

#endif // SNACKBARHOST_H
