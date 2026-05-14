#pragma once
#include <memory>

// A non-owning view over a fixed contiguous heap-allocated buffer.
// Zero overhead — drop-in replacement for query result containers.
template<typename T>
struct FixedSpan
{
    std::unique_ptr<T[]> buffer;
    uint8_t count = 0;
    uint8_t max_size = 0;

	FixedSpan(uint8_t max_elements)
        : buffer(std::make_unique<T[]>(max_elements))
        , max_size(max_elements)
    {}

    // Adds an element if there is remaining capacity, otherwise silently drops it
    void add(T value)
    {
        if (count < max_size)
            buffer[count++] = value;
    }

    // Direct index access into the buffer
    T operator[](uint8_t index) const
    {
        return buffer[index];
    }

    T* begin() { return buffer.get(); }
    T* end() { return buffer.get() + count; }
    const T* begin() const { return buffer.get(); }
    const T* end()   const { return buffer.get() + count; }

    FixedSpan(const FixedSpan& other)
        : buffer(std::make_unique<T[]>(other.max_size))
        , count(other.count)
        , max_size(other.max_size)
    {
        std::copy(other.buffer.get(), other.buffer.get() + other.count, buffer.get());
    }

    // Resets the count without deallocating — existing data is simply overwritten on next add
    void clear() { count = 0; }

    // O(1) removal — swaps target with last element then decrements count.
// Does not preserve order, which is fine for an unordered query result.
    void remove(uint8_t index)
    {
        buffer[index] = buffer[--count];
    }
};