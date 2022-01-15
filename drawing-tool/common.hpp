#pragma once
#include <memory>
#include <limits>
#include <condition_variable>
#include <deque>
#include <queue>
#include <stdio.h>
#include <cassert>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <thread>
#include <mutex>
#include <sstream>
#include <atomic>
#include <tuple>
#include <conio.h>

#define NOMINMAX
#include <Windows.h>

using namespace std;

void sleep_ms(const int ms);
void listdir(const string path, vector<string> &files);

#define SEL_VERSION 2