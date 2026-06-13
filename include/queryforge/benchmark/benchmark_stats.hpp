#pragma once

#include <vector>

struct BenchmarkStats {
    std::vector<double> latencies_ms;
    double p50_ms;
    double p95_ms;
    double p99_ms;
    double avg_ms;
    double min_ms;
    double max_ms;
};

double percentile(const std::vector<double>& sorted, double p);
BenchmarkStats compute_benchmark_stats(std::vector<double> latencies_ms);
