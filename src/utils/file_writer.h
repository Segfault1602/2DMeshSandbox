#pragma once

#include <cstddef>
#include <fstream>
#include <span>
#include <stdint.h>
#include <string>

class FileWriter
{
  public:
    FileWriter() = default;
    ~FileWriter();

    bool open(const std::string& filename, uint32_t frame_size, uint32_t rows, uint32_t cols);
    void close();

    void write_frame(float energy, const float* data, size_t size);
    void write_junction_types(const uint8_t* types, size_t size);

  private:
    std::string filename_;
    std::ofstream file_;
    uint32_t frame_size_;
};