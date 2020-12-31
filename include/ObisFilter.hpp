#ifndef __OBISFILTER_HPP__
#define __OBISFILTER_HPP__

#include <cstdint>
#include <vector>
#include <ObisData.hpp>


class ObisConsumer {
public:
    virtual void consume(const ObisFilterData &element) = 0;
};


class ObisFilter {

protected:
    std::vector<ObisConsumer*>     consumerTable;
    std::vector<ObisFilterData> filterTable;

public:
    ObisFilter(void);
    ~ObisFilter(void);

    void addFilter   (const ObisFilterData &entry);
    void removeFilter(const ObisFilterData &entry);
    const std::vector<ObisFilterData> &getFilter(void) const;

    void addConsumer   (ObisConsumer *obisConsumer);
    void removeConsumer(ObisConsumer *obisConsumer);

    bool consume(const void *const obis, const uint32_t timer) const;
    const ObisFilterData *const filter(const ObisData &element) const;
    void produce(const ObisFilterData &element) const;
};

#endif
