#include "percent_tracker.hpp"

ProgressTracker::ProgressTracker(std::string p, std::string c, int m) {
    process = p;
    completionMessage = c;
    max = m;
    current = 0;
    done = false;
}

void ProgressTracker::begin() {
    current = 0;
    done = false;
    std::cout << process << " starting... \n" << std::endl;
}

void ProgressTracker::increment_and_print() {
    if (current >= max && done == false) {
        std::cout << process << " 100% complete. " << completionMessage << std::endl;
        done = true;
        return;
    }

    int percent = (float)((float)current++ / (float)max) * 100;

    std::lock_guard<std::mutex> lock(print_semaphore);
    std::cout << "\r" << percent << "% complete...";
}
