#pragma once

#include <TB_CM_Types.h>

#include <vector>

struct TB_CM_Record {
    QString message;
    QString category;
    mutable QString timeText;
    quint64 timestampMs = 0;
    qint64 sequence = 0;
    int level = L_INFO;
    mutable int cachedWidth = -1;
    mutable int cachedHeight = -1;
};

// 定长环形缓冲：seq 单调递增、容量满时只淘汰最旧记录，仅限 GUI 线程访问
class TB_CM_Document final
{
public:
    explicit TB_CM_Document(int capacity = 20000);

    void setCapacity(int capacity);
    void appendBatch(std::vector<TB_CM_LogEntry>&& entries);
    void clear();

    int capacity() const { return int(slots_.size()); }
    int size() const { return int(nextSeq_ - firstSeq_); }
    qint64 firstSeq() const { return firstSeq_; }
    qint64 nextSeq() const { return nextSeq_; }
    bool contains(qint64 seq) const { return seq >= firstSeq_ && seq < nextSeq_; }

    const TB_CM_Record& at(qint64 seq) const;
    TB_CM_Record& at(qint64 seq);

private:
    std::vector<TB_CM_Record> slots_;
    qint64 firstSeq_ = 0;
    qint64 nextSeq_ = 0;
};
