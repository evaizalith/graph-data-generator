#ifndef EVA_PERCENT_TRACKER
#define EVA_PERCENT_TRACKER

#include <string>
#include <atomic>
#include <iostream>
#include <mutex>

class ProgressTracker {
public:
    ProgressTracker(std::string process, std::string completionMessage, int max); //!< Initializes tracker

    void begin();
    void increment_and_print();

private:
    std::string process;
    std::string completionMessage; 

    std::atomic<int> max;
    std::atomic<int> current;

    bool done;

    std::mutex print_semaphore;
};


#endif
