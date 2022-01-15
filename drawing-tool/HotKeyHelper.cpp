#include "common.hpp"
#include "HotKeyHelper.h"
#include <Windows.h>

using namespace std;
using namespace std::chrono;

bool request_thread_exit = false;
HANDLE recv_thead_handle;
DWORD recv_thread_id;
std::shared_ptr<std::thread> spliter_thread = nullptr;

mutex mutex_func_map;
std::map<int, HotKeyProcessFunc> func_map;

void HotKeyHelper::register_func(const int id, HotKeyProcessFunc func) {
    auto inst = HotKeyHelper::instance();
    if (inst == nullptr) {
        cerr << "HotKeyHelper instance is empty" << endl;
        return;
    }
    if (func != nullptr) {
        lock_guard<mutex> g(mutex_func_map);
        func_map[id] = func;
    } else {
        unregister_func(id);
    }
}

void HotKeyHelper::unregister_func(const int id) {
    auto inst = HotKeyHelper::instance();
    if (inst == nullptr) {
        cerr << "HotKeyHelper instance is empty" << endl;
        return;
    }
    lock_guard<mutex> g(mutex_func_map);
    if (func_map.count(id) != 0) {
        func_map.erase(id);
    }
}

void register_hotkeys() {
    for (int i = 0; i <= 9; i++) {
        RegisterHotKey(NULL, i, MOD_ALT | MOD_NOREPEAT, '0' + i);
    }
    RegisterHotKey(NULL, SPECIAL_KEY_1, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, '1');
    RegisterHotKey(NULL, SPECIAL_KEY_2, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, '2');
}

void unregister_hotkeys() {
    for (int i = 0; i <= 9; i++) {
        UnregisterHotKey(NULL, i);
    }
    UnregisterHotKey(NULL, SPECIAL_KEY_1);
    UnregisterHotKey(NULL, SPECIAL_KEY_2);
}

void call_register_funcs(const KeySeq key_seq) {
    for (auto it = func_map.begin(); it != func_map.end(); it++) {
        it->second(key_seq);
    }
}

mutex mutex_hotkey_seq;
typedef pair<WPARAM, DWORD> key_stroke_info;
queue<key_stroke_info> key_stroke_queue;

void thread_entry_hotkey_spliter() {
    const DWORD CONST_SEQ_INTERVAL = (DWORD) 300;
    KeySeq cur_key_seq;
    key_stroke_info cur_stroke;
    DWORD last_time = 0, cur_time;
    int state = 0;

    while (!request_thread_exit) {
        bool have_new_stroke = false;
        {
            lock_guard<mutex> g(mutex_hotkey_seq);
            if (!key_stroke_queue.empty()) {
                cur_stroke = key_stroke_queue.front();
                key_stroke_queue.pop();
                have_new_stroke = true;
            }
        }

        switch (state) {
        case 0:
            if (have_new_stroke) {
                cur_key_seq.clear();
                cur_key_seq.push_back((unsigned int)cur_stroke.first);
                last_time = cur_stroke.second;
                state = 1;
            }
            break;
        case 1:
            if (have_new_stroke) {
                cur_time = cur_stroke.second;
                if (cur_time - last_time > CONST_SEQ_INTERVAL) {
                    call_register_funcs(cur_key_seq);
                    cur_key_seq.clear();
                } else {
                    cur_key_seq.push_back((unsigned int)cur_stroke.first);
                    last_time = cur_stroke.second;
                }
            } else {
                cur_time = GetTickCount();
                if (cur_time - last_time > CONST_SEQ_INTERVAL) {
                    call_register_funcs(cur_key_seq);
                    cur_key_seq.clear();
                    state = 0;
                }
            }
            break;
        }
        std::this_thread::sleep_for(milliseconds(25));
    }
    //cout << "hotkey spliter thread exited." << endl;
}

DWORD WINAPI thread_entry_hotkey_receiver(PVOID param) {
    register_hotkeys();
    MSG winMessage = { 0 };
    while (GetMessage(&winMessage, NULL, 0, 0)) {
        if (request_thread_exit)
            break;
        if (winMessage.message == WM_HOTKEY) {
            lock_guard<mutex> g(mutex_hotkey_seq);
            key_stroke_queue.push(make_pair(winMessage.wParam, winMessage.time));
        } else {
            DispatchMessage(&winMessage);
        }
    }
    unregister_hotkeys();
    return 0;
}

HotKeyHelper::HotKeyHelper() {
    recv_thead_handle = CreateThread(NULL, 0, thread_entry_hotkey_receiver, 0, 0, &recv_thread_id);
    PostThreadMessage(recv_thread_id, WM_QUIT, 0, 0);
    CloseHandle(recv_thead_handle);
    spliter_thread = make_shared<thread>(thread_entry_hotkey_spliter);
}

HotKeyHelper::~HotKeyHelper() {
    request_thread_exit = true;
    PostThreadMessage(recv_thread_id, WM_QUIT, 0, 0);
    key_stroke_queue.push({});
    spliter_thread->join();
    spliter_thread = nullptr;
}

static mutex _mutex_instance;
static HotKeyHelper * _instance_ptr = nullptr;

HotKeyHelper * HotKeyHelper::instance() {
    if (_instance_ptr == nullptr) {
        lock_guard<mutex> g(_mutex_instance);
        if (_instance_ptr == nullptr) {
            _instance_ptr = new HotKeyHelper();
        }
    }
    return _instance_ptr;
}

void HotKeyHelper::dispose() {
    if (_instance_ptr != nullptr) {
        lock_guard<mutex> g(_mutex_instance);
        if (_instance_ptr != nullptr) {
            delete _instance_ptr;
            _instance_ptr = nullptr;
        }
    }
}