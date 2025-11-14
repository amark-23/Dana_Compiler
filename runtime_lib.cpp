#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cstring>

extern "C" {

    void writeInteger(int n) {
        std::cout << n << std::flush;
    }

    void writeChar(char c) {
        std::cout << c << std::flush;
    }

    void writeByte(char b) {
        std::cout << static_cast<int>(b) << std::flush;
    }

    void writeString(const char* s) {
        std::cout << s << std::flush;
    }

    int readInteger() {
        int i = 0;
        std::cin >> i;
        if (std::cin.fail()) {
             std::cin.clear();
             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
             return 0; 
        }
        return i;
    }

    char readChar() {
        char c = '\0';
        std::cin.get(c);
        if (std::cin.eof()) c = '\0';
        return c;
    }

    char readByte() {
        char c = '\0';
        std::cin.get(c);
        if (std::cin.eof()) c = '\0';
        return c;
    }

    void readString(int n_max_size, char* s_array) {
        if (n_max_size <= 0) return;

        int max_chars_to_read = n_max_size - 1; 
        if (max_chars_to_read < 0) max_chars_to_read = 0;

        int i = 0;
        char c;
        for (i = 0; i < max_chars_to_read; ++i) {
            int next_char = std::cin.peek();
            if (next_char == EOF || next_char == '\n') break;
            std::cin.get(c);
            s_array[i] = c;
        }
        s_array[i] = '\0';

        if (std::cin.peek() == '\n') std::cin.get();
    }

    int dana_strlen(const char* s) {
        return static_cast<int>(std::strlen(s));
    }

    int dana_strcmp(const char* s1, const char* s2) {
        return std::strcmp(s1, s2);
    }

    char* dana_strcpy(char* dest, const char* src) {
        return std::strcpy(dest, src);
    }

    char* dana_strcat(char* dest, const char* src) {
        return std::strcat(dest, src);
    }

    int extend(char b) {
        return static_cast<int>(b);
    }

    char shrink(int i) {
        return static_cast<char>(i & 0xFF);
    }
}