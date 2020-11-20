#ifndef __OBISFILTER_HPP__
#define __OBISFILTER_HPP__

#include <bits/types.h>
#include <vector>
#include <ObisElement.hpp>


class ObisConsumer {
public:
    virtual void consume(const ObisFilterElement &element) = 0;
};


class ObisFilter {

protected:
    std::vector<ObisConsumer*>     consumerTable;
    std::vector<ObisFilterElement> filterTable;

public:
    ObisFilter(void);
    ~ObisFilter(void);

    void addFilter   (const ObisFilterElement &entry);
    void removeFilter(const ObisFilterElement &entry);
    const std::vector<ObisFilterElement> &getFilter(void) const;

    void addConsumer   (ObisConsumer *obisConsumer);
    void removeConsumer(ObisConsumer *obisConsumer);

    bool consume(const void *const obis, const __uint32_t timer) const;
    const ObisFilterElement *const filter(const ObisElement &element) const;
    void produce(const ObisFilterElement &element) const;
};

#endif
