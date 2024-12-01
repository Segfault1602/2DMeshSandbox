#include "mat2d.h"

#include <iostream>
#include <string>

template <typename T>
Mat2D<T>::Mat2D()
    : rows_(0)
    , cols_(0)
{
}

template <typename T>
Mat2D<T>::~Mat2D()
{
}

template <typename T>
Mat2D<T>::Mat2D(const Mat2D<T>& other)
{
    rows_ = other.rows_;
    cols_ = other.cols_;
    data_ = other.data_;
}

template <typename T>
Mat2D<T>& Mat2D<T>::operator=(const Mat2D<T>& other)
{
    if (this != &other)
    {
        rows_ = other.rows_;
        cols_ = other.cols_;
        data_ = other.data_;
    }
    return *this;
}

template <typename T>
Mat2D<T>::Mat2D(Mat2D<T>&& other)
{
    rows_ = other.rows_;
    cols_ = other.cols_;
    data_ = std::move(other.data_);
}

template <typename T>
void Mat2D<T>::allocate(size_t rows, size_t cols)
{
    rows_ = rows;
    cols_ = cols;

    data_.resize(rows * cols);
}

template <typename T>
void Mat2D<T>::free()
{
    data_.resize(0);
    rows_ = 0;
    cols_ = 0;
}

template <typename T>
void Mat2D<T>::clear()
{
    data_.clear();
    data_.resize(rows_ * cols_);
}

template <typename T>
float Mat2D<T>::get_energy() const
{
    // float energy = 0;
    // for (size_t i = 0; i < rows_ * cols_; i++)
    // {
    //     energy += data_[i] * data_[i];
    // }
    // return energy;
    return 0;
}

template <typename T>
size_t Mat2D<T>::get_row_size() const
{
    return rows_;
}

template <typename T>
size_t Mat2D<T>::get_col_size() const
{
    return cols_;
}

template <typename T>
size_t Mat2D<T>::size() const
{
    return rows_ * cols_;
}