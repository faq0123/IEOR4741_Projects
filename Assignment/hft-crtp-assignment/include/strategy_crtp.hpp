#pragma once
#include "market_data.hpp"

// CRTP base class: static polymorphism
template <typename Derived>
struct StrategyBase {
    double on_tick(const Quote& q) {
        return static_cast<Derived*>(this)->on_tick_impl(q);
    }
};

// CRTP implementation of the same signal
struct SignalStrategyCRTP : public StrategyBase<SignalStrategyCRTP> {
    double alpha1, alpha2;

    explicit SignalStrategyCRTP(double a1, double a2)
        : alpha1(a1), alpha2(a2) {}

    double on_tick_impl(const Quote& q) {
        const double mp  = microprice(q);
        const double m   = mid(q);
        const double imb = imbalance(q);
        return alpha1 * (mp - m) + alpha2 * imb;
    }
};
