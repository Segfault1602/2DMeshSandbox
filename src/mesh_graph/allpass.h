#pragma once

/**
 * @brief A linear allpass filter implementation
 */
class Allpass
{
  public:
    /**
     * @brief Constructs an allpass filter
     * @param a The allpass coefficient (-1 < a < 1)
     */
    Allpass(float a = 0.f);

    /**
     * @brief Sets the allpass coefficient
     * @param a The new coefficient value (-1 < a < 1)
     */
    void setA(float a);

    /**
     * @brief Processes one input sample through the filter
     * @param x Input sample
     * @return Filtered output sample
     */
    float process(float x);

    /**
     * @brief Gets the last computed output value
     * @return The most recent output sample
     */
    float last_out() const;

  private:
    float a_;
    float last_out_;
    float delays_[2];
};

/**
 * @brief A non-linear allpass filter implementation with two coefficients
 */
class NonLinearAllpass
{
  public:
    /**
     * @brief Constructs a non-linear allpass filter
     * @param a1 First allpass coefficient (-1 < a1 < 1)
     * @param a2 Second allpass coefficient (-1 < a2 < 1)
     */
    NonLinearAllpass(float a1, float a2);

    /**
     * @brief Sets both allpass coefficients
     * @param a1 First coefficient (-1 < a1 < 1)
     * @param a2 Second coefficient (-1 < a2 < 1)
     */
    void setA(float a1, float a2);

    /**
     * @brief Processes one input sample through the filter
     * @param x Input sample
     * @return Filtered output sample
     */
    float process(float x);

    /**
     * @brief Gets the last computed output value
     * @return The most recent output sample
     */
    float last_out() const;

  private:
    float a_[2];
    float last_out_;
    float delays_[2];
};