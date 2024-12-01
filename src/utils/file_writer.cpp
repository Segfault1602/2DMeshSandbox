#include "file_writer.h"

namespace
{
constexpr uint32_t kMagic = 0xCDCDABAB;
constexpr uint32_t kVersion = 1;

struct __attribute__((packed)) Header
{
    uint32_t magic;
    uint32_t version;
    uint32_t rows;
    uint32_t cols;
};
} // namespace

FileWriter::~FileWriter()
{
    close();
}

bool FileWriter::open(const std::string& filename, uint32_t frame_size, uint32_t rows, uint32_t cols)
{
    filename_ = filename;
    file_.open(filename_, std::ios::binary);
    if (!file_.is_open())
    {
        return false;
    }

    Header header = {kMagic, kVersion, rows, cols};
    frame_size_ = frame_size;

    file_.write(reinterpret_cast<char*>(&header), sizeof(Header));
    return true;
}

void FileWriter::close()
{
    if (file_.is_open())
    {
        file_.close();
    }
}

void FileWriter::write_frame(float energy, const float* data, size_t size)
{
    if (size != frame_size_)
    {
        return;
    }

    file_.write(reinterpret_cast<const char*>(&energy), sizeof(float));
    file_.write(reinterpret_cast<const char*>(data), size * sizeof(float));
}

void FileWriter::write_junction_types(const uint8_t* types, size_t size)
{
    file_.write(reinterpret_cast<const char*>(types), size * sizeof(uint8_t));
}