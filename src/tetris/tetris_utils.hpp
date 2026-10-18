#pragma once

#include <string_view>

// unlikely to ever be used anywhere else, as it is strongly related to the Tetris logic
namespace TetrisUtils {
    // assumes rot has size >=2 and only contains the characters 0, R, 2, L and, optionally, - and > (to form a little arrow)
    // results are undefined if any of the characters are not those listed here
    // if rot represents an invalid rotation, the returned value will also be invalid
    inline constexpr int rotstr_to_int(std::string_view rot) {
        int fst = 0;
        size_t lst = rot.length() > 2 ? rot.length() - 1 : 1;
        int res = 0;

        char fst_c = rot[fst], lst_c = rot[lst];
        if (fst_c == 'R') {
            res = 1;
        } else if (fst_c == 'L') {
            res = 3;
        } else {
            res = fst_c - '0';
        }
        res *= 10;

        if (lst_c == 'R') {
            res += 1;
        } else if (lst_c == 'L') {
            res += 3;
        } else {
            res += lst_c - '0';
        }
        return res;
    }

    inline constexpr int rotint_to_a_idx(int rotint) {
        switch(rotint) {
            case 0: return 0;
            case rotstr_to_int("0->R"): return 1;
            case rotstr_to_int("2->R"): return 1;
            case rotstr_to_int("2->L"): return 3;
            case rotstr_to_int("0->L"): return 3;
            case rotstr_to_int("L->0"): return 0;
            case rotstr_to_int("R->0"): return 0;
            case rotstr_to_int("R->2"): return 2;
            case rotstr_to_int("L->2"): return 2;
            default:
                return {};
        }
    }
}