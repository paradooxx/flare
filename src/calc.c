#include "calc.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

typedef struct {
    const char *src;
    int pos;
    bool has_error;
    bool has_operator;
    bool has_number;
} MathParser;

static void SkipWhitespace(MathParser *p) {
    while (p->src[p->pos] && isspace((unsigned char)p->src[p->pos])) {
        p->pos++;
    }
}

static char Peek(MathParser *p) {
    SkipWhitespace(p);
    return p->src[p->pos];
}

static char Get(MathParser *p) {
    SkipWhitespace(p);
    return p->src[p->pos++];
}

static double ParseExpression(MathParser *p);

static double ParseFactor(MathParser *p) {
    SkipWhitespace(p);
    char c = Peek(p);

    // Unary plus
    if (c == '+') {
        Get(p);
        return ParseFactor(p);
    }

    // Unary minus
    if (c == '-') {
        Get(p);
        p->has_operator = true;
        return -ParseFactor(p);
    }

    // Parentheses
    if (c == '(') {
        Get(p);
        double val = ParseExpression(p);
        if (Peek(p) == ')') {
            Get(p);
        } else {
            p->has_error = true;
        }
        return val;
    }

    // Identifiers: functions or constants
    if (isalpha((unsigned char)c)) {
        char ident[32] = { 0 };
        int idx = 0;
        while (isalpha((unsigned char)Peek(p)) && idx < 31) {
            ident[idx++] = (char)tolower((unsigned char)Get(p));
        }
        ident[idx] = '\0';

        if (strcmp(ident, "pi") == 0) {
            p->has_number = true;
            return 3.14159265358979323846;
        }
        if (strcmp(ident, "e") == 0) {
            p->has_number = true;
            return 2.71828182845904523536;
        }

        // Functions: sqrt, abs, sin, cos, tan, log, ln
        if (Peek(p) == '(') {
            Get(p);
            double arg = ParseExpression(p);
            if (Peek(p) == ')') {
                Get(p);
            } else {
                p->has_error = true;
            }

            p->has_operator = true;
            if (strcmp(ident, "sqrt") == 0) return arg >= 0 ? sqrt(arg) : 0;
            if (strcmp(ident, "abs") == 0) return fabs(arg);
            if (strcmp(ident, "sin") == 0) return sin(arg);
            if (strcmp(ident, "cos") == 0) return cos(arg);
            if (strcmp(ident, "tan") == 0) return tan(arg);
            if (strcmp(ident, "log") == 0 || strcmp(ident, "log10") == 0) return arg > 0 ? log10(arg) : 0;
            if (strcmp(ident, "ln") == 0) return arg > 0 ? log(arg) : 0;
        }

        p->has_error = true;
        return 0;
    }

    // Numeric literals
    if (isdigit((unsigned char)c) || c == '.') {
        char *endptr = NULL;
        double val = strtod(&p->src[p->pos], &endptr);
        if (endptr && endptr != &p->src[p->pos]) {
            p->pos = (int)(endptr - p->src);
            p->has_number = true;

            // Handle percentage (e.g. 20%)
            if (Peek(p) == '%') {
                Get(p);
                p->has_operator = true;
                val /= 100.0;
            }
            return val;
        }
    }

    p->has_error = true;
    return 0;
}

static double ParsePower(MathParser *p) {
    double left = ParseFactor(p);
    while (Peek(p) == '^') {
        Get(p);
        p->has_operator = true;
        double right = ParseFactor(p);
        left = pow(left, right);
    }
    return left;
}

static double ParseTerm(MathParser *p) {
    double left = ParsePower(p);
    while (true) {
        char op = Peek(p);
        if (op == '*' || op == 'x' || op == 'X') {
            Get(p);
            p->has_operator = true;
            left *= ParsePower(p);
        } else if (op == '/') {
            Get(p);
            p->has_operator = true;
            double right = ParsePower(p);
            if (right != 0.0) {
                left /= right;
            } else {
                p->has_error = true;
                return 0;
            }
        } else if (op == '%') {
            Get(p);
            p->has_operator = true;
            double right = ParsePower(p);
            if (right != 0.0) {
                left = fmod(left, right);
            }
        } else {
            break;
        }
    }
    return left;
}

static double ParseExpression(MathParser *p) {
    double left = ParseTerm(p);
    while (true) {
        char op = Peek(p);
        if (op == '+') {
            Get(p);
            p->has_operator = true;
            left += ParseTerm(p);
        } else if (op == '-') {
            Get(p);
            p->has_operator = true;
            left -= ParseTerm(p);
        } else {
            break;
        }
    }
    return left;
}

static void FormatResult(double val, char *buf, int maxLen) {
    if (fabs(val - round(val)) < 1e-9 && fabs(val) < 1e14) {
        snprintf(buf, maxLen, "%.0f", round(val));
    } else {
        snprintf(buf, maxLen, "%.6f", val);
        char *dot = strchr(buf, '.');
        if (dot) {
            char *end = buf + strlen(buf) - 1;
            while (end > dot && *end == '0') {
                *end = '\0';
                end--;
            }
            if (end == dot) {
                *end = '\0';
            }
        }
    }
}

bool Calc_Evaluate(const char *expr, double *out_result, char *out_str, int max_len) {
    if (!expr || strlen(expr) == 0) return false;

    // Reject queries that start with typical app search words
    MathParser p = { .src = expr, .pos = 0, .has_error = false, .has_operator = false, .has_number = false };
    double res = ParseExpression(&p);
    SkipWhitespace(&p);

    // Valid if expression parsed completely with at least one number and an operation or constant
    if (!p.has_error && p.has_number && p.has_operator && p.src[p.pos] == '\0') {
        if (out_result) *out_result = res;
        if (out_str && max_len > 0) {
            FormatResult(res, out_str, max_len);
        }
        return true;
    }
    return false;
}

bool Calc_CopyToClipboard(const char *text) {
    if (!text || strlen(text) == 0) return false;
    if (!OpenClipboard(NULL)) return false;
    EmptyClipboard();

    size_t len = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hMem) {
        void *p = GlobalLock(hMem);
        if (p) {
            memcpy(p, text, len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
    }
    CloseClipboard();
    return true;
}
