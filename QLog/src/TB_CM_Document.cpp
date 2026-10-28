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
    std::vector<Record> nextSlots(newCapacity);
    const qint64 last = nextSeq_;
    const qint64 firstKept = std::max(firstSeq_, last - qint64(capacity));
    for (qint64 seq = firstKept; seq < last; ++seq) {
        nextSlots[size_t(seq % capacity)] = std::move(slots_[size_t(seq % slots_.size())]);
    }
    slots_ = std::move(nextSlots);
    firstSeq_ = firstKept;
}
void TB_CM_Document::appendBatch(std::vector<Record>&& records)
{
    const size_t capacity = slots_.size();
    if (capacity == 0 || records.empty()) {
        return;
    }
    size_t begin = 0;
    if (records.size() > capacity) {
        begin = records.size() - capacity;
    }
    for (size_t i = begin; i < records.size(); ++i) {
        Record& slot = slots_[size_t(nextSeq_ % qint64(capacity))];
        slot = std::move(records[i]);
        slot.sequence = nextSeq_;
        ++nextSeq_;
        if (nextSeq_ - firstSeq_ > qint64(capacity)) {
            firstSeq_ = nextSeq_ - qint64(capacity);
        }
    }
}
void TB_CM_Document::clear()
{
    for (Record& record : slots_) {
        record = Record();
    }
    firstSeq_ = 0;
    nextSeq_ = 0;
}
const Record& TB_CM_Document::at(qint64 seq) const
{
    Q_ASSERT(contains(seq));
    return slots_[size_t(seq % qint64(slots_.size()))];
}
Record& TB_CM_Document::at(qint64 seq)
{
    Q_ASSERT(contains(seq));
    return slots_[size_t(seq % qint64(slots_.size()))];
}
