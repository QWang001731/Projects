#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <map>
#include <boost/math/special_functions/erf.hpp> // Include Boost for erfinv
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <cstring>
#include <chrono>
#include <immintrin.h>
#include <omp.h>

double max_element(double* array, int n) {
    double maxValue = -std::numeric_limits<double>::infinity();
    for (int i = 0; i < n; i++) {
        if (!std::isnan(array[i]) && array[i] > maxValue)
            maxValue = array[i];
    }
    if (maxValue == -std::numeric_limits<double>::infinity())
        return std::nan("");
    return maxValue;
}


void rankdata(double* data, int n, const std::string& method, double* ranks) {
    auto compare = [&](size_t i, size_t j) {
        if (std::isnan(data[i]) && std::isnan(data[j])) {
            return false; 
        }
        if (std::isnan(data[i])) {
            return false; 
        }
        if (std::isnan(data[j])) {
            return true; 
        }
        return data[i] < data[j]; 
    };

    int* indices = new int[n];
    std::iota(indices, indices + n, 0);
    std::sort(indices, indices + n, compare);

    double currentRank = 1.0;
    if (method == "ordinal") {
        for (int i = 0; i < n; i++) {
            int idx = indices[i];
            if (std::isnan(data[idx]))
                ranks[idx] = std::numeric_limits<double>::quiet_NaN();
            else
                ranks[idx] = currentRank++;
        }
    } else if (method == "min") {
        int rank = 0;
        int cnt = 0;
        for (int i = 0; i < n; i++) {
            if (std::isnan(data[indices[i]]))
                ranks[indices[i]] = std::numeric_limits<double>::quiet_NaN();
            else {
                if (i > 0 && data[indices[i]] == data[indices[i - 1]]){
                    ranks[indices[i]] = rank;
                    cnt++;
                } 
                else {
                    rank = cnt + 1;
                    ranks[indices[i]] = rank;
                    cnt++;
                }
            }
        }
    }
    delete[] indices;
}


double norm_ppf(double p) {
    if(std::isnan(p))
        return std::nan("");
    return std::sqrt(2) * boost::math::erf_inv(2 * p - 1);
}

pybind11::array_t<double> sc_best_norm(const pybind11::array_t<double>& matrix, int method = 1, int handle_repeat = 1) {
    pybind11::buffer_info array_buf = matrix.request();
    double* array_ptr = static_cast<double*>(array_buf.ptr);
    auto shape = matrix.shape();

    int n = shape[0];
    double* var_arr = new double[n];
    std::memcpy(var_arr, array_ptr, n * sizeof(double));

    double* fr = new double[n];
    rankdata(var_arr, n, "ordinal", fr);

    double* mr = new double[n];
    rankdata(var_arr, n, "min", mr);

    double max_fr = max_element(fr, n);
    double* fpr = new double[n];

    int nthread = omp_get_max_threads();

    #pragma omp parallel for num_threads(nthread)
    for (int i=0; i < n; i++) {
        fpr[i] = fr[i] / (max_fr + 1);
    }

    double* fn = new double[n];
    if (method == 0) {
        #pragma omp parallel for num_threads(nthread)
        for (int i=0; i < n; i++) {
            fn[i] = fpr[i] - .5;
        }


    } else if (method == 1) {
        #pragma omp parallel for num_threads(nthread)
        for (int i = 0; i < n; i++) {
            fn[i] = norm_ppf(fpr[i]);
        }
    } else {
        delete[] var_arr;
        delete[] fr;
        delete[] mr;
        delete[] fpr;
        delete[] fn;
        throw std::invalid_argument("Method only accept 0:rank, 1: z_normal");
    }

    if (handle_repeat){
        std::map<int, int> rank_counts;
        #pragma omp parallel for num_threads(nthread)
        for (int i = 0; i < n; i++) {
            rank_counts[static_cast<int>(mr[i])]++;
        }

        for (const auto& [rank, count] : rank_counts) {
            if (count > 1) {
                double mean_fn = 0.0;
                int cnt = 0;
                #pragma omp parallel for shared(mean_fn) num_threads(nthread)
                for (int i = 0; i < n; i++) {
                    if (mr[i] == rank) {
                        mean_fn += fn[i];
                        cnt++;
                    }
                }
                mean_fn = mean_fn / cnt;
                #pragma omp parallel for num_threads(nthread)
                for (int i = 0; i < n; i++) {
                    if (mr[i] == rank) {
                        fn[i] = mean_fn;
                    }
                }
            }
        }
    }

    pybind11::array_t<double> fn_np_array(n, fn);
    delete[] var_arr;
    delete[] fr;
    delete[] mr;
    delete[] fpr;
    delete[] fn;
    return fn_np_array;
}

void sc_best_norm1(double* var_arr, int n, int method, int handle_repeat, double* fn) {
    double* fr = new double[n];
    rankdata(var_arr, n, "ordinal", fr);

    double* mr = new double[n];
    rankdata(var_arr, n, "min", mr);

    double max_fr = max_element(fr, n);
    double* fpr = new double[n];
    int nthread = omp_get_max_threads();

    #pragma omp parallel for num_threads(nthread)
    for (int i=0; i < n; i++) {
        fpr[i] = fr[i] / (max_fr + 1);
    }

    if (method == 0) {
        #pragma omp parallel for num_threads(nthread)
        for (int i=0; i < n; i++) {
            fn[i] = fpr[i] - .5;
        }


    } else if (method == 1) {
        #pragma omp parallel for num_threads(nthread)
        for (int i = 0; i < n; i++) {
            fn[i] = norm_ppf(fpr[i]);
        }
    } else {
        delete[] fr;
        delete[] mr;
        delete[] fpr;
        throw std::invalid_argument("Method only accept 0:rank, 1: z_normal");
    }

    if (handle_repeat) {
        std::map<int, int> rank_counts;
        for (int i = 0; i < n; i++) {
            rank_counts[static_cast<int>(mr[i])]++;
        }

        for (const auto& [rank, count] : rank_counts) {
            if (count > 1) {
                double mean_fn = 0.0;
                int cnt = 0;
                #pragma omp parallel for shared(mean_fn) num_threads(nthread)
                for (int i = 0; i < n; i++) {
                    if (mr[i] == rank) {
                        mean_fn += fn[i];
                        cnt++;
                    }
                }
                mean_fn = mean_fn / cnt;
                #pragma omp parallel for num_threads(nthread)
                for (int i = 0; i < n; i++) {
                    if (mr[i] == rank) {
                        fn[i] = mean_fn;
                    }
                }
            }
        }
    }

    delete[] fr;
    delete[] mr;
    delete[] fpr;
}

pybind11::array_t<double> sc_best_norm_2d(pybind11::array_t<double> matrix, int method, int handle_repeat) {
    pybind11::buffer_info buf_info = matrix.request();
    if (buf_info.ndim != 2) {
        throw std::runtime_error("Input should be a 2D array.");
    }

    auto shape = matrix.shape();
    int rows = shape[0];
    int cols = shape[1];
    double* ptr = static_cast<double*>(buf_info.ptr);

    double* result = new double[rows * cols];

    for (int i = 0; i < rows; i++) {
        double* row = ptr + i * cols;
        double* fn_row = new double[cols];
        sc_best_norm1(row, cols, method, handle_repeat, fn_row);
        std::memcpy(result + i * cols, fn_row, sizeof(double) * cols);
        delete[] fn_row;
    }

    pybind11::array_t<double> res_array({rows, cols}, result);
    delete[] result;
    return res_array;
}

PYBIND11_MODULE(normalization, m) {
    m.def("sc_best_norm", &sc_best_norm, "");
    m.def("sc_best_norm_2d", &sc_best_norm_2d, "");
}