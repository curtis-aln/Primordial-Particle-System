#pragma once
#include <atomic>

template<typename T>
class TripleBuffer
{
public:
    explicit TripleBuffer(int cell_render_reserve)
        : m_latest_ready(-1)
        , m_render_idx(-1)
        , m_write_idx(0)
    {
        for (int i = 0; i < 3; ++i)
        {
            new (&m_buffers[i]) T(cell_render_reserve);
        }
    }

    ~TripleBuffer()
    {
        for (int i = 0; i < 3; ++i)
        {
            reinterpret_cast<T*>(&m_buffers[i])->~T();
        }
    }

    // ── Update thread ─────────────────────────────────────────────────────

    // Get a reference to write the next frame into.
    T& get_write_buffer()
    {
        return m_buffers[m_write_idx];
    }

    // Call when done writing. Makes this frame available to the renderer
    // and finds the next buffer to write into.
    void publish()
    {
        // Atomically publish the buffer we just finished writing.
        // "release" ensures the buffer contents are visible before the index.
        const int old_ready = m_latest_ready.exchange(m_write_idx,
            std::memory_order_release);

        // old_ready is now free (renderer has its own copy of render_idx).
        // Find the new write buffer: anything that isn't latest_ready or render_idx.
        const int render = m_render_idx.load(std::memory_order_relaxed);
        const int ready = m_write_idx; // what we just published

        for (int i = 0; i < 3; ++i)
        {
            if (i != ready && i != render)
            {
                m_write_idx = i;
                break;
            }
        }
    }

    // ── Render thread ─────────────────────────────────────────────────────

    // Returns true if a new frame is available since the last call to
    // begin_read(). Call this before begin_read() to avoid unnecessary swaps.
    bool has_new_frame() const
    {
        const int ready = m_latest_ready.load(std::memory_order_relaxed);
        return ready != -1 && ready != m_render_idx.load(std::memory_order_relaxed);
    }

    // Swap to the latest ready frame if one exists.
    // Returns the buffer to render. Always valid after first publish().
    const T& begin_read()
    {
        const int ready = m_latest_ready.load(std::memory_order_acquire);
        if (ready != -1)
            m_render_idx.store(ready, std::memory_order_relaxed);

        return m_buffers[m_render_idx.load(std::memory_order_relaxed)];
    }

    // Call when done rendering this frame.
    void end_read()
    {
        // Nothing strictly needed here in this design,
        // but having the call site keeps the API symmetric and
        // gives you a place to add metrics later.
    }

private:
    T          m_buffers[3];
    std::atomic<int> m_latest_ready;  // written by update, read by render
    std::atomic<int> m_render_idx;    // written by render, read by update
    int              m_write_idx;     // only ever touched by update thread
};