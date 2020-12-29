#pragma once
#include <string.h>

extern "C" char *strndup(const char *str, int len);
extern "C" int sscanf(const char *str, const char *fmt, ...);
extern "C" int tolower(int chr);

struct string {
    int len;
    char* s;

    string(const char* _s, int _len = -1) {
        len = ((_len == -1) ? strlen(_s) : _len);
        if (len != 0) {
            s = strndup(_s, len);
        } else {
            s = "";
        }
    }
    
    string(const string& b) {
        string(b.s);
    }

    string() {
        string("");
    }

    ~string() {
        if (len != 0) {
            free(s);
        }
    }

    string& operator=(const string& b) {
        if (this == &b) return *this;
        
        len = b.len;
        s = strndup(b.s, len);
        return *this;
    }

    bool equalsIgnoreCase(const char* b) {
        if (strlen(b) != len) return false;
        for (int i = 0;i<len;i++) {
            if (tolower(b[i]) != tolower(s[i])) return false;
        }
        return true;
    }

    bool operator==(const char* b) {
        return strncmp(b, s, len) == 0;
    }

    bool operator==(char b) {
        return len == 1 && s[0] == b;
    }

    bool empty() {
        return len == 0;
    }

    int asInt() {
        int result = -1;
        if (len > 2 && s[0] == '0' && s[1] == 'x') {
            if (sscanf(s + 2, "%x", &result) <= 0) return -1;
            return result;
        } else {
            for (int i = 0; i<len; i++) {
                if (!(s[i] >= '0' && s[i]<='9')) return -1;
            }
            return atoi(s);
        }
    }
};