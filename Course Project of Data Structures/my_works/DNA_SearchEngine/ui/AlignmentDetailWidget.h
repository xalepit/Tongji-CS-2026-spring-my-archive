#ifndef ALIGNMENTDETAILWIDGET_H
#define ALIGNMENTDETAILWIDGET_H

#include <QAbstractScrollArea>

class SearchSessionModel;

class AlignmentDetailWidget : public QAbstractScrollArea
{
public:
    explicit AlignmentDetailWidget(QWidget *parent = nullptr);

    void SetSession(const SearchSessionModel *session);
    void Refresh();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    static constexpr int GutterWidth = 126;
    static constexpr int CellWidth = 32;
    static constexpr int RowHeight = 30;

    const SearchSessionModel *m_session;

    void updateAccessibleDescription();
    void updateScrollRange();
    void focusFirstMismatch();
};

#endif // ALIGNMENTDETAILWIDGET_H
