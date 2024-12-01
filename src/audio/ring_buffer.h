#pragma once

#include <atomic>
#include <memory>

template <typename T>
class RingBuffer
{
  public:
    RingBuffer(size_t size = 32768);
    ~RingBuffer();

    void Resize(size_t size);

    size_t GetSize() const;
    size_t GetReadAvailable() const;
    size_t GetWriteAvailable() const;

    void Write(const T* data, size_t size);
    void Read(T* data, size_t& size);
    void Peek(T* data, size_t& size);

    void Reset();

  private:
    size_t max_size_ = 0;
    std::atomic<size_t> read_index_ = 0;
    std::atomic<size_t> write_index_ = 0;
    std::atomic_bool overflow_flag_ = false;
    T* buffer_ = nullptr;
};

#include "ring_buffer.tpp"