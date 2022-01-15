#pragma once
#include <vector>
#include <thread>
#include <map>

typedef std::vector<unsigned int> KeySeq;
typedef void(*HotKeyProcessFunc) (const KeySeq &);

#define SPECIAL_KEY_1 100
#define SPECIAL_KEY_2 101

class HotKeyHelper {
public:
    static HotKeyHelper * instance();
    static void dispose();

    static void register_func(const int id, HotKeyProcessFunc func);
    static void unregister_func(const int id);

private:
    HotKeyHelper();
    ~HotKeyHelper();
};
