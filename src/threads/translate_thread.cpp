#include "threads/translate_thread.h"
#include <algorithm>
#include <iostream>
#include <chrono>

using namespace std;

TranslateThread::TranslateThread(
    LibreTranslateAPI& api,
    queue<std::vector<textData>>& inputQueue,
    vector<textData>& outputQueue,
    mutex& inputMtx,
    mutex& outputMtx,
    condition_variable& cond
) :
    ai(api),
    toTranslateQueue(inputQueue),
    translatedText(outputQueue),
    toTranslateMutex(inputMtx),
    translatedMutex(outputMtx),
    condition(cond),
    running(false){}

void TranslateThread::start()
{
    running = true;
    thread = std::thread(&TranslateThread::run, this);
}

void TranslateThread::stop()
{
    running = false;
}

void TranslateThread::join()
{
    if(thread.joinable())
    {
        thread.join();
    }
}

void TranslateThread::run()
{
    while (running)
    {
        vector<textData> toTranslate;
        {
            unique_lock<mutex> lock(toTranslateMutex);
            if(condition.wait_for(lock, std::chrono::milliseconds(100), [&](){ return !toTranslateQueue.empty(); })) {
                toTranslate = toTranslateQueue.front();
                toTranslateQueue.pop();
            } else {
                continue;
            }
        }

        for (auto& dt : toTranslate)
        {
            try {
                dt.original.erase(std::remove_if(dt.original.begin(), dt.original.end(),
                    [](char c) { return c == '\n' || c == '\r'; }),
                    dt.original.end());

                auto it = translation_cache.find(dt.original);
                if (it != translation_cache.end())
                {
                    dt.translated = it->second;
                }
                else
                {
                    string result = ai.translateText(dt.original, "en", "pt");
                    dt.translated = result;
                    translation_cache[dt.original] = result;
                }
            } catch (const char* e) {
                cerr << "ERRO DE TRADUCAO: " << e << endl;
                dt.translated = "[ERRO NA TRADUCAO]";
            }
        }
        lock_guard<mutex> lock(translatedMutex);
        translatedText = toTranslate;
    }
}