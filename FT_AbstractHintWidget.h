#pragma once
#include <QString>
#include <QFont>

class QWidget;
class QEvent;
class QPainter;
class QRect;

class FT_AbstractHintWidget
{
protected:
    explicit FT_AbstractHintWidget(QWidget *owner);
    virtual ~FT_AbstractHintWidget() = default;

public:
    void setHint(const QString &hint);
    QString hint() const { return m_hint; }

protected:
    struct FT_HintMetrics {
        QFont font;
        int   height  = 0;
        int   leading = 0;
        int   topArea = 0;
    };
    static FT_HintMetrics makeHintMetrics(const QFont &baseFont);

    static constexpr int kSidePad   = 8;
    static constexpr int kBottomPad = 6;

    int  hintTopPad() const;
    bool hintReady() const { return m_initialized; }
    bool hasHint() const   { return !m_hint.isEmpty(); }
    QWidget *owner() const { return m_owner; }

    bool ftChangeEvent(QEvent *e);
    void ftResizeEvent();
    void ftPaint(QPainter &p, const QRect &area, int rightReserve = 0);

    virtual void applyMargins() = 0;
    virtual void ftPaintExtra(QPainter &p, const QRect &area) { (void)p; (void)area; }

private:
    QWidget *m_owner = nullptr;
    QString  m_hint;
    bool     m_initialized = false;
};