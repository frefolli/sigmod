#ifndef SEEK_HH
#define SEEK_HH

#include <sigmod/record.hh>

template <typename Accessor>
uint32_t SeekLow(Accessor accessor,
                 uint32_t start,
                 uint32_t end,
                 float32_t value) {
    uint32_t l = start;
    uint32_t r = end;
    uint32_t m = (l + r) / 2;

    while(l <= r) {
        const float32_t center = accessor(m);
        if (center > value) {
            r = m - 1;
            m = (l + r) / 2;
        } else if (center < value) {
            l = m + 1;
            m = (l + r) / 2;
        } else {
            return m;
        }
    }

    return r;
}

template <typename Accessor>
uint32_t SeekHigh(Accessor accessor,
                  uint32_t start,
                  uint32_t end,
                  float32_t value) {
    uint32_t l = start;
    uint32_t r = end;
    uint32_t m = (l + r) / 2;

    while(l <= r) {
        const float32_t center = accessor(m);
        if (center > value) {
            r = m - 1;
            m = (l + r) / 2;
        } else if (center < value) {
            l = m + 1;
            m = (l + r) / 2;
        } else {
            return m;
        }
    }

    return l;
}

#endif
