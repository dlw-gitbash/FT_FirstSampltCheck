#include <TB_CM_Document.h>

#include <algorithm>

TB_CM_Document::TB_CM_Document(int capacity)
    : slots_(size_t(std::max(1, capacity)))
{
}
void TB_CM_Document::setCapacity(int capacity)
{
    capacity = std::max(1, capacity);
    if (capacity == int(slots_.size())) {
        return;
    }
    const size_t newCapacity = size_t(capacity);
    std::vector<TB_CM_Record> nextSlots(newCapacity);
    const qint64 last = nextSeq_;
    const qint64 firstKept = std::max(firstSeq_, last - qint64(capacity));
    for (qint64 seq = firstKept; seq < last; ++seq) {
        nextSlots[size_t(seq % capacity)] = std::move(slots_[size_t(seq % slots_.size())]);
    }
    slots_ = std::move(nextSlots);
    firstSeq_ = firstKept;
}
void TB_CM_Document::appendBatch(std::vector<TB_CM_LogEntry>&& entries)
{
    const size_t capacity = slots_.size();
    if (capacity == 0 || entries.empty()) {
        return;
    }
    size_t begin = 0;
    if (entries.size() > capacity) {
        begin = entries.size() - capacity;
    }
    for (size_t i = begin; i < entries.size(); ++i) {
        TB_CM_Record& slot = slots_[size_t(nextSeq_ % qint64(capacity))];
        slot = TB_CM_Record();
        slot.message = std::move(entries[i].message);
        slot.category = std::move(entries[i].category);
        slot.timestampMs = entries[i].timestampMs;
        slot.level = entries[i].level;
        slot.sequence = nextSeq_;
        ++nextSeq_;
        if (nextSeq_ - firstSeq_ > qint64(capacity)) {
            firstSeq_ = nextSeq_ - qint64(capacity);
        }
    }
}
void TB_CM_Document::clear()
{
    for (TB_CM_Record& record : slots_) {
        record = TB_CM_Record();
    }
    firstSeq_ = 0;
    nextSeq_ = 0;
}
const TB_CM_Record& TB_CM_Document::at(qint64 seq) const
{
    Q_ASSERT(contains(seq));
    return slots_[size_t(seq % qint64(slots_.size()))];
}
TB_CM_Record& TB_CM_Document::at(qint64 seq)
{
    Q_ASSERT(contains(seq));
    return slots_[size_t(seq % qint64(slots_.size()))];
}
