#include "threads/detection_thread.h"
#include <chrono>

using namespace std;
using namespace std::chrono;

DetectionThread::DetectionThread(
    Detector& tesseract,
    queue<cv::Mat>& inputQueue,
    queue<std::vector<textData>>& outputQueue,
    mutex& inputMtx,
    mutex& outputMtx,
    condition_variable& inputCond,
    condition_variable& outputCond
) :
    ai(tesseract),
    toDetectQueue(inputQueue),
    detectedQueue(outputQueue),
    toDetectMutex(inputMtx),
    detectedMutex(outputMtx),
    inCondition(inputCond),
    outCondition(outputCond),
    running(false){}

void DetectionThread::start()
{
    running = true;
    thread = std::thread(&DetectionThread::run, this);
}

void DetectionThread::stop()
{
    running = false;
}

void DetectionThread::join()
{
    if (thread.joinable())
    {
        thread.join();
    }
}

void DetectionThread::run()
{
    while(running)
    {
        cv::Mat toDetect;
        {
            unique_lock<mutex> lock(toDetectMutex);
            if(inCondition.wait_for(lock, milliseconds(100), [&](){ return !toDetectQueue.empty(); })) {
                toDetect = toDetectQueue.front();
                toDetectQueue.pop();
            } else {
                continue;
            }
        }

        vector<textData> detectedText = ai.detect_text_box(toDetect);

        if (!detectedText.empty()) {
            lock_guard<mutex> lock(detectedMutex);
            detectedQueue.push(detectedText);
            outCondition.notify_one();
        }
    }
}