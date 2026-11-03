#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <vector>

namespace ura {

using AnimationClock = std::chrono::steady_clock;
using AnimationTimePoint = AnimationClock::time_point;
using AnimationDuration = AnimationClock::duration;

// A 256-entry lookup table keeps easing evaluation out of the Lua call path.
// The curve is generated once when an animation is requested.
class EasingCurve {
public:
  static constexpr std::size_t sample_count = 256;

  EasingCurve() {
    this->build([](double t) { return t; });
  }

  static EasingCurve from_name(std::string_view name) {
    EasingCurve curve;

    if (name == "linear")
      return curve;

    if (name == "in_quad")
      return curve.build([](double t) { return t * t; });
    if (name == "out_quad")
      return curve.build([](double t) { return 1.0 - (1.0 - t) * (1.0 - t); });
    if (name == "in_out_quad")
      return curve.build([](double t) {
        return t < 0.5 ? 2.0 * t * t
                       : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
      });

    if (name == "in_cubic")
      return curve.build([](double t) { return t * t * t; });
    if (name == "out_cubic")
      return curve.build([](double t) { return 1.0 - std::pow(1.0 - t, 3.0); });
    if (name == "in_quart")
      return curve.build([](double t) { return t * t * t * t; });
    if (name == "out_quart")
      return curve.build([](double t) { return 1.0 - std::pow(1.0 - t, 4.0); });
    if (name == "in_out_quart")
      return curve.build([](double t) {
        return t < 0.5 ? 8.0 * t * t * t * t
                       : 1.0 - std::pow(-2.0 * t + 2.0, 4.0) / 2.0;
      });

    if (name == "in_expo")
      return curve.build([](double t) {
        return t == 0.0 ? 0.0 : std::pow(2.0, 10.0 * t - 10.0);
      });
    if (name == "out_expo")
      return curve.build([](double t) {
        return t == 1.0 ? 1.0 : 1.0 - std::pow(2.0, -10.0 * t);
      });
    if (name == "in_out_expo")
      return curve.build([](double t) {
        if (t == 0.0)
          return 0.0;
        if (t == 1.0)
          return 1.0;
        return t < 0.5 ? std::pow(2.0, 20.0 * t - 10.0) / 2.0
                       : (2.0 - std::pow(2.0, -20.0 * t + 10.0)) / 2.0;
      });

    if (name == "out_back") {
      constexpr double c1 = 1.70158;
      constexpr double c3 = c1 + 1.0;
      return curve.build([=](double t) {
        return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
      });
    }

    if (name == "in_out_back") {
      constexpr double c1 = 1.70158;
      constexpr double c2 = c1 * 1.525;
      return curve.build([=](double t) {
        return t < 0.5
          ? (std::pow(2.0 * t, 2.0) * ((c2 + 1.0) * 2.0 * t - c2)) / 2.0
          : (std::pow(2.0 * t - 2.0, 2.0) * ((c2 + 1.0) * (t * 2.0 - 2.0) + c2)
             + 2.0)
            / 2.0;
      });
    }

    // In-out cubic is the documented default and a safe fallback for an
    // unknown name.
    return curve.build([](double t) {
      return t < 0.5 ? 4.0 * t * t * t
                     : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;
    });
  }

  static EasingCurve from_samples(const std::vector<float>& values) {
    if (values.empty())
      return EasingCurve::from_name("in_out_cubic");

    EasingCurve curve;
    if (values.size() == 1) {
      curve.samples.fill(values.front());
      return curve;
    }

    for (std::size_t i = 0; i < sample_count; ++i) {
      auto position = static_cast<double>(i)
        / static_cast<double>(sample_count - 1)
        * static_cast<double>(values.size() - 1);
      auto index = static_cast<std::size_t>(position);
      auto fraction = position - static_cast<double>(index);
      auto lhs = values[index];
      auto rhs = values[std::min(index + 1, values.size() - 1)];
      curve.samples[i] = static_cast<float>(lhs + (rhs - lhs) * fraction);
    }
    return curve;
  }

  float sample(double t) const {
    if (t <= 0.0)
      return this->samples.front();
    if (t >= 1.0)
      return this->samples.back();

    auto position = t * static_cast<double>(sample_count - 1);
    auto index = static_cast<std::size_t>(position);
    auto fraction = position - static_cast<double>(index);
    auto lhs = this->samples[index];
    auto rhs = this->samples[std::min(index + 1, sample_count - 1)];
    return static_cast<float>(lhs + (rhs - lhs) * fraction);
  }

  float operator()(double t) const {
    return this->sample(t);
  }

  const std::array<float, sample_count>& data() const {
    return this->samples;
  }

private:
  std::array<float, sample_count> samples {};

  template<typename F>
  EasingCurve build(F&& function) {
    for (std::size_t i = 0; i < sample_count; ++i) {
      auto t = static_cast<double>(i) / static_cast<double>(sample_count - 1);
      this->samples[i] = static_cast<float>(function(t));
    }
    return *this;
  }
};

struct AnimationOptions {
  std::chrono::milliseconds duration { 200 };
  EasingCurve easing = EasingCurve::from_name("in_out_cubic");
  bool animate_opacity = false;
  float opacity = 1.f;
};

} // namespace ura
