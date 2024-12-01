#pragma once

#include <cstddef>
#include <exception>
#include <iostream>
#include <vector>

template <typename T>
class Mat2D
{
  public:
    Mat2D();
    ~Mat2D();

    Mat2D(const Mat2D& other);
    Mat2D(Mat2D&& other);
    Mat2D& operator=(const Mat2D& other);

    void allocate(size_t rows, size_t cols);
    void free();
    void clear();

    size_t get_row_size() const;
    size_t get_col_size() const;
    size_t size() const;

    float get_energy() const;

    const T* data() const
    {
        return data_;
    }

    inline const T& operator()(size_t row, size_t col) const
    {
#ifdef DEBUG
        if (row >= rows_ || col >= cols_)
        {
            throw std::out_of_range("Mat2D::operator(): out of range");
        }
#endif
        return data_[cols_ * row + col];
    }

    inline T& operator()(size_t row, size_t col)
    {
#ifdef DEBUG
        if (row >= rows_ || col >= cols_)
        {
            throw std::out_of_range("Mat2D::operator(): out of range (row: " + std::to_string(row) +
                                    ", col: " + std::to_string(col) + ")");
        }
#endif
        return data_[cols_ * row + col];
    }

    inline const T& operator[](size_t idx) const
    {
#ifdef DEBUG
        if (idx >= rows_ * cols_)
        {
            throw std::out_of_range("Mat2D::operator[]: out of range");
        }
#endif
        return data_[idx];
    }

    inline T& operator[](size_t idx)
    {
#ifdef DEBUG
        if (idx >= rows_ * cols_)
        {
            throw std::out_of_range("Mat2D::operator[]: out of range");
        }
#endif
        return data_[idx];
    }

    std::vector<T>& container()
    {
        return data_;
    }

    const std::vector<T>& container() const
    {
        return data_;
    }

  private:
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;
};

#include "mat2d.tpp"