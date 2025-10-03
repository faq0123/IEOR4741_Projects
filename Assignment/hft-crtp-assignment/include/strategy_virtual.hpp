#pragma once
#include "market_data.hpp"

// Base interface for runtime polymorphism
struct IStrategy {
    virtual ~IStrategy() = default;
    virtual double on_tick(const Quote& q) = 0;
};

// Strategy implemented with virtual function
struct SignalStrategyVirtual : public IStrategy {
    double alpha1, alpha2;

    explicit SignalStrategyVirtual(double a1, double a2)
        : alpha1(a1), alpha2(a2) {}

    double on_tick(const Quote& q) override {
        const double mp  = microprice(q);
        const double m   = mid(q);
        const double imb = imbalance(q);
        return alpha1 * (mp - m) + alpha2 * imb;
    }
};
