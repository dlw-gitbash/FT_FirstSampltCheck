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

    QSize minimumSizeHint() const override;

signals:
    void heightChanged(int pixelHeight);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void adjustHeightToContent();

    QLabel* m_titleLabel = nullptr;
    bool    m_autoGrow   = true;
    int     m_singleHeight = kFHintMinHeight;
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
