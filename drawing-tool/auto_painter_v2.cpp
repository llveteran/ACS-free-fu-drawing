#include "common.hpp"
#if SEL_VERSION == 2

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "HotKeyHelper.h"

#define CFG_FILE "fu_cfg.txt"
#define TPLT_WIDTH 600
//const float DOT_SIZE_W = 41.f / 664.f;
//const float DOT_SIZE_H = 56.f / 747.f;

#define DRAW_MODE_FULL 0
#define DRAW_MODE_MIN 1

using namespace std;

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
};

struct Point2f {
    float x, y;
    Point2f(): x(.0f), y(.0f) {}
    Point2f(float x_, float y_): x(x_), y(y_) {}
};

typedef vector<Point2f> draw_task;
map<int, draw_task> rune_tasks;

struct Config {
    string tplt_path;
    Point region_lt;
    Point region_rb;
    int draw_mode;
} cfg;

void write_config() {
    ofstream fout;
    fout.open(CFG_FILE, ios::out);
    fout << cfg.tplt_path << endl
        << cfg.region_lt.x << endl
        << cfg.region_lt.y << endl
        << cfg.region_rb.x << endl
        << cfg.region_rb.y << endl
        << cfg.draw_mode << endl;
    fout.close();
}

void create_default_config() {
    cfg.tplt_path = "./free_fu/";
    cfg.region_lt = { 0, 0 };
    cfg.region_rb = { 1, 1 };
    cfg.draw_mode = DRAW_MODE_FULL;

    write_config();
}

void read_config() {
    ifstream fin;
    fin.open(CFG_FILE, ios::in);
    if (!fin) {
        create_default_config();
        return;
    }

    string s;
    getline(fin, s);
    cfg.tplt_path = s;
    getline(fin, s);
    cfg.region_lt.x = atoi(s.c_str());
    getline(fin, s);
    cfg.region_lt.y = atoi(s.c_str());
    getline(fin, s);
    cfg.region_rb.x = atoi(s.c_str());
    getline(fin, s);
    cfg.region_rb.y = atoi(s.c_str());
    getline(fin, s);
    cfg.draw_mode = atoi(s.c_str());
}

bool create_task_from_8x8_template(uint8_t * tplt_data, draw_task & task, int stride) {
    // tplt: black->drawed
    const int N = 8;

    task.clear();
    float grid = 1.f / float(N);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            auto v = tplt_data[(y * N + x) * stride];
            if (v == 0) {
                float nx = float(x) / float(N);
                float ny = float(y) / float(N);
                if (cfg.draw_mode == DRAW_MODE_FULL) {
                    task.emplace_back(nx + 0.25f * grid, ny + 0.3333f * grid);
                    task.emplace_back(nx + 0.50f * grid, ny + 0.3333f * grid);
                    task.emplace_back(nx + 0.75f * grid, ny + 0.3333f * grid);
                    task.emplace_back(nx + 0.25f * grid, ny + 0.6666f * grid);
                    task.emplace_back(nx + 0.50f * grid, ny + 0.6666f * grid);
                    task.emplace_back(nx + 0.75f * grid, ny + 0.6666f * grid);
                } else /*if (cfg.draw_mode == DRAW_MODE_MIN)*/ {
                    task.emplace_back(nx + 0.50f * grid, ny + 0.6666f * grid);
                }
            }
        }
    }
    return true;
}

bool load_fix_template(const string & fname, draw_task & dst) {
    dst.clear();
    if (fname.size() < 5) {
        return false;
    }

    const string ext = fname.substr(int(fname.size()) - 3);
    if (ext == string("png")) {
        return false;
    } else if (ext == string("bmp")) {
        // free fu 8x8
        // stbi_load(fname)
        int iw, ih, ic;
        auto img_ptr = stbi_load(fname.c_str(), &iw, &ih, &ic, 0);
        if (iw != 8 || ih != 8) {
            return false;
        }
        return create_task_from_8x8_template(img_ptr, dst, ic);
    } else {
        return false;
    }

    return true;
}

void _click_at_coord(int x, int y) {
    SetCursorPos(x, y);
    sleep_ms(10);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    sleep_ms(20);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    sleep_ms(10);
}

void execute_drawing(const draw_task & task) {
    sleep_ms(300);
    int region_w = cfg.region_rb.x - cfg.region_lt.x;
    int region_h = cfg.region_rb.y - cfg.region_lt.y;
    if (region_w <= 42 || region_h <= 42) {
        return;
    }
    for (int i = 0; i < task.size(); i++) {
        const Point2f & pt = task[i];
        int x = cfg.region_lt.x + int(roundf(pt.x * (float)region_w));
        int y = cfg.region_lt.y + int(roundf(pt.y * (float)region_h));
        _click_at_coord(x, y);
    }
}

void hotkey_dispatcher(const KeySeq & ks) {
    POINT pt = { 0,0 };
    if (ks.size() == 1) {
        if (ks[0] == SPECIAL_KEY_1) {
            GetCursorPos(&pt);
            cfg.region_lt = Point(pt.x, pt.y);
            write_config();
            return;
        } else if (ks[0] == SPECIAL_KEY_2) {
            GetCursorPos(&pt);
            cfg.region_rb = Point(pt.x, pt.y);
            write_config();
            return;
        }
    }

    int idx = 0;
    for (int i = 0; i < ks.size(); i++) {
        idx += int(std::pow(10.0f, float(ks.size()) - i - 1) * float(ks[i]));
    }

    if (rune_tasks.count(idx) > 0) {
        cout << "\rPress Alt+Number to draw: " << idx << "      ";
        execute_drawing(rune_tasks[idx]);
    }
}

int main() {
    read_config();

    vector<string> fu_files;
    listdir(R"(./free_fu/*)", fu_files);
    int load_idx = 1;

    for (int i = 0; i < fu_files.size();i++) {
        const string path = string("./free_fu/") + fu_files[i];
        
        draw_task task;
        if (!load_fix_template(path, task)) {
            continue;
        }
        rune_tasks[load_idx] = task;
        cout << load_idx << ": " << fu_files[i] << endl;

        load_idx++;
    }

    HotKeyHelper::register_func(
        0,
        hotkey_dispatcher
    );

    cout << "Press 'q' to termiante." << endl;
    cout << "Press Alt+Ctrl+1/2 to set region left-top / right-bottom" << endl;
    cout << "Press Alt+Number to draw: ";
    while (_getch() != 'q') {
        // pass
    }
    HotKeyHelper::dispose();

    return 0;
}

#endif // SEL_VERSION==2
