#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "External/concurrentqueue.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std::chrono_literals;

std::mutex folderMutex, fileMutex;
moodycamel::ConcurrentQueue<fs::path> folders;
moodycamel::ConcurrentQueue<fs::path> files;
moodycamel::ConcurrentQueue<fs::path> results;
std::condition_variable folderCondVar, fileCondVar;
int threadsAvailable;
int maxThreads;
std::atomic_int foldersLeft = 0;
std::atomic_int filesLeft = 0;
std::vector<std::string> extensions;
std::atomic_bool areFoldersLeft = true;

fs::path path;
std::string fileName;
std::string extensionString;
std::string contains;

void addWork(const fs::path &path) {
    if (fs::is_regular_file(path)) {
        if (!contains.empty()) {
            files.enqueue(path);
            filesLeft++;
            fileCondVar.notify_one();
        }
    } else {
        folders.enqueue(path);
        foldersLeft++;
        folderCondVar.notify_one();
    }
}

bool hasEnding(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void processFolderQueue() {
    std::unique_lock<std::mutex> lck(folderMutex);
    bool areFoldersLeftHere = false;
    while(areFoldersLeft) {
        if (foldersLeft == 0) {
            folderCondVar.wait(lck);
        }
        fs::path folder;
        folders.try_dequeue(folder);
        foldersLeft--;

        for (auto &subFolder : boost::make_iterator_range(fs::directory_iterator(folder), {})) {
            if (fs::is_directory(subFolder)) {
                areFoldersLeftHere = true;
                addWork(subFolder);
            }
            if (fs::is_regular(subFolder)) {
                if (subFolder.path().string().find('.') == std::string::npos) {
                    continue;
                }
                std::istringstream ss(subFolder.path().string());
                std::string token;
                while (std::getline(ss, token, '.'));
                if (std::any_of(extensions.begin(), extensions.end(),
                                [&](std::string test) { return token == test; })) {
                    if (contains != "") {
                        addWork(subFolder);
                    } else {
                        results.enqueue(subFolder);
                    }
                }
            }
        }
    }
    lck.unlock();
}

void processFileQueue() {

}

int main(int argc, char **argv) {
    threadsAvailable = std::thread::hardware_concurrency();
    po::options_description desc("Allowed options");

    desc.add_options()
            ("help", "produce help message")
            ("path", po::value<fs::path>(&path), "Starting path")
            ("file", po::value<std::string>(&fileName), "Filename to search for")
            ("extensions", po::value<std::string>(&extensionString),
             "Comma separated list of extentions to search for/in")
            ("contains", po::value<std::string>(&contains), "Files that contain this string")
            ("threads", po::value<int>(&maxThreads), "Max amount of threads to use (default = max threads-1)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        BOOST_LOG_TRIVIAL(info) << desc << "\n";
        return 0;
    }

    if (vm.count("file") && vm.count("extensions")) {
        BOOST_LOG_TRIVIAL(error) << "Please use only filename or extensions";
        return 1;
    }

    if (vm.count("extensions")) {
        std::istringstream ss(extensionString);
        std::string extentionSplit;
        while (std::getline(ss, extentionSplit, ',')) {
            extensions.push_back(extentionSplit);
        }
    }

    if (!vm.count("path")) {
        path = boost::filesystem::current_path();
    } else {
        path = fs::canonical(path);
    }

    BOOST_LOG_TRIVIAL(info) << "listing files in: " << path.string();

    if (!vm.count("threads")) {
        maxThreads = threadsAvailable - 1;
    }

    BOOST_LOG_TRIVIAL(info) << "found " << threadsAvailable << " threads will use " << maxThreads << std::endl;


    if (fs::is_regular_file(path)) {
        if (contains.empty()) {
            BOOST_LOG_TRIVIAL(info) << "File given on non file search";
            return 1;
        }
    }

    addWork(path);

    std::thread test;

    test = std::thread(processFolderQueue);

    test.join();

    folderCondVar.notify_all();

//    std::thread folderWorkers[maxThreads], fileWorkers[maxThreads];

    fs::path testPath;
    results.try_dequeue(testPath);
    BOOST_LOG_TRIVIAL(debug)  << testPath.string();



    return 0;
}
