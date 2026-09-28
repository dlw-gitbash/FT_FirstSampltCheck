#ifndef FT_WIDGET_H
#define FT_WIDGET_H

#include <QComboBox>
#include <QTextEdit>
#include <QSpinBox>
#include <QKeyEvent>
#include <QString>

class QLabel;
class QWheelEvent;

constexpr int kFHintTitleHeight      = 13;
constexpr int kFHintMinHeight        = 36;
constexpr int kFHintTitleTop         = 2;
constexpr int kFHintSingleTopPadding = kFHintTitleTop + kFHintTitleHeight;
constexpr int kFHintComboArrowReserve = 20;
constexpr int kTitleLeft             = 6;

class FHintComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit FHintComboBox(const QString& title = QString(), QWidget* parent = nullptr);

    void    setTitle(const QString& title);
    QString title() const;

    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QLabel* m_titleLabel = nullptr;
};

class FHintTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit FHintTextEdit(const QString& title = QString(), QWidget* parent = nullptr);

    void    setTitle(const QString& title);
    QString title() const;

    void setAutoGrow(bool on);
    void refreshHeight();

    QSize minimumSizeHint() const override;

signals:
    void heightChanged(int pixelHeight);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void adjustHeightToContent(bool force = false);

    QLabel* m_titleLabel = nullptr;
    bool    m_autoGrow   = true;
    int     m_singleHeight = kFHintMinHeight;
    // 最近一次做“文档换行宽度”时用的视口宽度。文档高度只由宽度和内容决定,
    // 宽度不变时重算结果必然相同,可用于短路掉 resize/layout 引起的重复计算。
    int     m_lastLayoutWidth = -1;
};

class FHintSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit FHintSpinBox(const QString& title = QString(), QWidget* parent = nullptr);

    void    setTitle(const QString& title);
    QString title() const;

    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QLabel* m_titleLabel = nullptr;
};

#endif // FT_WIDGET_H