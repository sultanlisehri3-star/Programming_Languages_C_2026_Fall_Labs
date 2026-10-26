// Sehri Sultanli 251ADB093
// https://github.com/<sultanlisehri3-star>/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude AI (prompt: i made calc.c, and opened files, please now say to me what i need to write codes" with the lab handout and calc-tests.zip attached.), conversation: <https://claude.ai/share/1df4e7af-3af4-406d-94cb-bcd2ddc6ad55>; C standard library man pages (strtod, pow, opendir, stat, mkdir). Details in comments where used.

// AI (Claude): prompt "i made calc.c, and opened files, please now say to me what i need to write codes" with the lab handout and calc-tests.zip attached." + the lab text and sample tests.
//   Claude proposed the overall structure (read the file once, recursive descent
//   parser with one function per precedence level, results buffered until the end).
//  And i reviewed each part and if it is needed, i can explain it.

#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STUDENT_NAME "sehri"
#define STUDENT_LASTNAME "sultanli"
#define STUDENT_ID "251adb093"

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "calc: out of memory\n");
        exit(1);
    }
    return q;
}

// Large files: output is collected in a buffer whose capacity doubles, so
// appending N bytes costs O(N) in total and nothing is copied repeatedly.
typedef struct {
    char *data;
    size_t len, cap;
} Buf;

static void buf_append(Buf *b, const char *s, size_t n) {
    if (b->len + n > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (cap < b->len + n) cap *= 2;
        b->data = xrealloc(b->data, cap);
        b->cap = cap;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
}

static char *copy_n(const char *s, size_t n) {
    char *r = xrealloc(NULL, n + 1);
    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

static char *path_basename(const char *path) {
    size_t n = strlen(path);
    while (n > 1 && path[n - 1] == '/') n--;
    size_t start = n;
    while (start > 0 && path[start - 1] != '/') start--;
    return copy_n(path + start, n - start);
}

static int ends_with_txt(const char *s) {
    size_t n = strlen(s);
    return n >= 4 && strcmp(s + n - 4, ".txt") == 0;
}

static char *join_path(const char *dir, const char *name) {
    size_t a = strlen(dir), b = strlen(name);
    char *r = xrealloc(NULL, a + b + 2);
    memcpy(r, dir, a);
    r[a] = '/';
    memcpy(r + a + 1, name, b + 1);
    return r;
}

// Integer vs float: every values remind its own type. An operation uses
// integer arithmetic only if BOTH operands are ints; otherwise both are
// converted to double. So 7 / 2 == 3 but 7 / 2.0 == 3.5, with no global mode.
typedef struct {
    int is_float;
    long long i;
    double d;
} Num;

static double as_double(Num n) { return n.is_float ? n.d : (double)n.i; }

typedef enum {
    T_INT, T_FLOAT, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_POW,
    T_LP, T_RP, T_END, T_BAD
} TokType;

typedef struct {
    TokType type;
    size_t pos;  // 1-based position in the whole file
    Num val;
} Token;

typedef struct {
    Num val;
    size_t start;   // where this operand starts
    size_t op_pos;  // where the ** after it is
} PowItem;

typedef struct {
    // Position tracking: the whole file is in ONE buffer, so a position is just
    // (pointer - base) + 1. It automatically counts every '\n' and every
    // character of comment lines, with no separate counter that could drift.
    const char *base;
    const char *p;
    const char *end;   // end of the current line
    Token cur;         // one-token lookahead
    size_t err_pos;    // 0 = no error

    PowItem *stk;      // explicit stack for ** chains (no deep C recursion)
    size_t sp, cap;

    char *scratch;     // copy of a float literal so strtod stops at its end
    size_t scratch_cap;
} Parser;

// An invalid character becomes T_BAD and is an error only when the parser has
// to use it, so an earlier error in the line (e.g. "10 / 0 &") comes first.
static Token lex(Parser *ps) {
    Token t;
    memset(&t, 0, sizeof t);
    while (ps->p < ps->end && (*ps->p == ' ' || *ps->p == '\t')) ps->p++;
    t.pos = (size_t)(ps->p - ps->base) + 1;

    // The end-of-line token sits at the '\n' (or one past the last character if
    // the file has no final newline): exactly where "missing operand" and
    // "missing )" must be reported.
    if (ps->p >= ps->end) {
        t.type = T_END;
        return t;
    }

    char c = *ps->p;
    if (c >= '0' && c <= '9') {
        const char *s = ps->p, *q = s;
        while (q < ps->end && *q >= '0' && *q <= '9') q++;
        // A float needs digits '.' digits. For "5." or ".5" the integer is
        // lexed alone and the lone '.' becomes T_BAD, so the error is at the
        // '.'. The same way, the 'e' in 1.5e3 is just an invalid character.
        if (q + 1 < ps->end && *q == '.' && q[1] >= '0' && q[1] <= '9') {
            q++;
            while (q < ps->end && *q >= '0' && *q <= '9') q++;
            size_t n = (size_t)(q - s);
            if (n + 1 > ps->scratch_cap) {
                ps->scratch_cap = n + 1;
                ps->scratch = xrealloc(ps->scratch, ps->scratch_cap);
            }
            memcpy(ps->scratch, s, n);
            ps->scratch[n] = '\0';
            t.val.is_float = 1;
            t.val.d = strtod(ps->scratch, NULL);
            t.type = isfinite(t.val.d) ? T_FLOAT : T_BAD;
        } else {
            unsigned long long v = 0;  // unsigned: wrap-around is defined
            for (const char *r = s; r < q; r++) v = v * 10ULL + (unsigned long long)(*r - '0');
            t.val.i = (long long)v;
            t.type = T_INT;
        }
        ps->p = q;
        return t;
    }

    ps->p++;
    switch (c) {
        case '+': t.type = T_PLUS; break;
        case '-': t.type = T_MINUS; break;
        case '/': t.type = T_SLASH; break;
        case '(': t.type = T_LP; break;
        case ')': t.type = T_RP; break;
        case '*':
            if (ps->p < ps->end && *ps->p == '*') {
                ps->p++;
                t.type = T_POW;
            } else {
                t.type = T_STAR;
            }
            break;
        default:
            t.type = T_BAD;
    }
    return t;
}

static void advance(Parser *ps) { ps->cur = lex(ps); }

// Error positions chosen by the rules:
//   division by zero       -> start of the right operand (rhs_pos)
//   inf / NaN float result -> the operator (op_pos)
//   negative int exponent  -> start of the exponent (rhs_pos)
static int fail(Parser *ps, size_t pos) {
    ps->err_pos = pos;
    return 0;
}

static int apply_arith(Parser *ps, char op, Num a, Num b, size_t op_pos, size_t rhs_pos, Num *r) {
    if (!a.is_float && !b.is_float) {
        // Unsigned types so integer overflow can never be undefined behaviour.
        unsigned long long ua = (unsigned long long)a.i, ub = (unsigned long long)b.i;
        r->is_float = 0;
        r->d = 0;
        switch (op) {
            case '+': r->i = (long long)(ua + ub); break;
            case '-': r->i = (long long)(ua - ub); break;
            case '*': r->i = (long long)(ua * ub); break;
            default:
                if (b.i == 0) return fail(ps, rhs_pos);
                r->i = (b.i == -1) ? (long long)(0ULL - ua) : a.i / b.i;
        }
        return 1;
    }
    double x = as_double(a), y = as_double(b), v;
    if (op == '/' && y == 0.0) return fail(ps, rhs_pos);
    switch (op) {
        case '+': v = x + y; break;
        case '-': v = x - y; break;
        case '*': v = x * y; break;
        default: v = x / y;
    }
    if (!isfinite(v)) return fail(ps, op_pos);
    r->is_float = 1;
    r->d = v;
    r->i = 0;
    return 1;
}

static int apply_pow(Parser *ps, Num a, Num b, size_t op_pos, size_t rhs_pos, Num *r) {
    if (!a.is_float && !b.is_float) {
        if (b.i < 0) return fail(ps, rhs_pos);
        // Exponentiation by squaring: fast even for 1 ** 9000000000.
        unsigned long long result = 1, base = (unsigned long long)a.i, e = (unsigned long long)b.i;
        while (e) {
            if (e & 1) result *= base;
            e >>= 1;
            if (e) base *= base;
        }
        r->is_float = 0;
        r->i = (long long)result;
        r->d = 0;
        return 1;
    }
    double v = pow(as_double(a), as_double(b));
    if (!isfinite(v)) return fail(ps, op_pos);
    r->is_float = 1;
    r->d = v;
    r->i = 0;
    return 1;
}

//   expr    := term  { ('+' | '-') term }
//   term    := power { ('*' | '/') power }
//   power   := primary [ '**' power ]
//   primary := NUMBER | '(' expr ')'
//
// Precedence comes from the call order: expr calls term and term calls power,
// so the operands of '+' are whole products and the operands of '*' are whole
// powers. The tighter an operator binds, the deeper its function is.
// Values are computed while parsing (no syntax tree), so memory does not grow
// with the input.
//
// Every syntax error is reported at the token the parser was looking at
// (ps->cur) when it could not continue.

static int parse_expr(Parser *ps, Num *out);

static int parse_primary(Parser *ps, Num *out) {
    if (ps->cur.type == T_INT || ps->cur.type == T_FLOAT) {
        *out = ps->cur.val;
        advance(ps);
        return 1;
    }
    if (ps->cur.type == T_LP) {
        advance(ps);
        if (!parse_expr(ps, out)) return 0;
        // Not ')' here: at the line end this is "missing )", otherwise it is
        // whatever token stopped the parser.
        if (ps->cur.type != T_RP) return fail(ps, ps->cur.pos);
        advance(ps);
        return 1;
    }
    // Where an operand is required: an operator ("7 + * 3"), a unary -
    // ("-5"), an early ')' ("()"), a bad character, or the line end ("3 *").
    return fail(ps, ps->cur.pos);
}

// '**' is right-associative: the whole chain a ** b ** c is collected and
// folded from right, so 2 ** 3 ** 2 groups as 2 ** The textbook
// version recurses into power() for the exponent; an explicit stack gives the
// same grouping and the same error order, but cannot overflow the C stack on
// a line with millions of '**'.
static int parse_power(Parser *ps, Num *out) {
    size_t base = ps->sp;
    Num first;
    if (!parse_primary(ps, &first)) return 0;
    if (ps->cur.type != T_POW) {
        *out = first;
        return 1;
    }

    PowItem item;
    item.val = first;
    item.start = 0;
    item.op_pos = 0;
    for (;;) {
        item.op_pos = ps->cur.pos;
        if (ps->sp == ps->cap) {
            ps->cap = ps->cap ? ps->cap * 2 : 64;
            ps->stk = xrealloc(ps->stk, ps->cap * sizeof *ps->stk);
        }
        ps->stk[ps->sp++] = item;
        advance(ps);
        item.start = ps->cur.pos;
        if (!parse_primary(ps, &item.val)) return 0;
        if (ps->cur.type != T_POW) break;
    }
    Num acc = item.val;
    size_t rhs_start = item.start;
    while (ps->sp > base) {
        PowItem *left = &ps->stk[--ps->sp];
        if (!apply_pow(ps, left->val, acc, left->op_pos, rhs_start, &acc)) return 0;
        rhs_start = left->start;
    }
    *out = acc;
    return 1;
}

// The loop (instead of recursion) makes '*' and '/' left-associative:
// 8 / 4 / 2 is (8 / 4) / 2 because acc already holds the left part.
static int parse_term(Parser *ps, Num *out) {
    Num acc;
    if (!parse_power(ps, &acc)) return 0;
    while (ps->cur.type == T_STAR || ps->cur.type == T_SLASH) {
        char op = ps->cur.type == T_STAR ? '*' : '/';
        size_t op_pos = ps->cur.pos;
        advance(ps);
        size_t rhs_pos = ps->cur.pos;
        Num rhs;
        if (!parse_power(ps, &rhs)) return 0;
        if (!apply_arith(ps, op, acc, rhs, op_pos, rhs_pos, &acc)) return 0;
    }
    *out = acc;
    return 1;
}

static int parse_expr(Parser *ps, Num *out) {
    Num acc;
    if (!parse_term(ps, &acc)) return 0;
    while (ps->cur.type == T_PLUS || ps->cur.type == T_MINUS) {
        char op = ps->cur.type == T_PLUS ? '+' : '-';
        size_t op_pos = ps->cur.pos;
        advance(ps);
        size_t rhs_pos = ps->cur.pos;
        Num rhs;
        if (!parse_term(ps, &rhs)) return 0;
        if (!apply_arith(ps, op, acc, rhs, op_pos, rhs_pos, &acc)) return 0;
    }
    *out = acc;
    return 1;
}

static int eval_line(Parser *ps, const char *line_start, const char *line_end, Buf *out) {
    ps->p = line_start;
    ps->end = line_end;
    ps->sp = 0;
    advance(ps);
    Num v;
    if (!parse_expr(ps, &v)) return 0;
    if (ps->cur.type != T_END) return fail(ps, ps->cur.pos);

    char tmp[64];
    int n;
    if (v.is_float) n = snprintf(tmp, sizeof tmp, "%.15g\n", v.d);
    else n = snprintf(tmp, sizeof tmp, "%lld\n", v.i);
    buf_append(out, tmp, (size_t)n);
    return 1;
}

// Results stay in the memory till the end because one error replaces ALL results.
// The file is scanned once and every line is parsed in place so the total time is linear in the file size.
static Buf evaluate_file(const char *data, size_t len) {
    Buf out = {NULL, 0, 0};
    Parser ps;
    memset(&ps, 0, sizeof ps);
    ps.base = data;

    size_t pos = 0;
    while (pos < len) {
        const char *nl = memchr(data + pos, '\n', len - pos);
        size_t line_end = nl ? (size_t)(nl - data) : len;

        size_t s = pos;
        while (s < line_end && (data[s] == ' ' || data[s] == '\t')) s++;
        // Blank and comment lines are skipped, but their characters still count
        // in positions because positions come from the buffer offset.
        if (s < line_end && data[s] != '#') {
            if (!eval_line(&ps, data + s, data + line_end, &out)) {
                char tmp[64];
                int n = snprintf(tmp, sizeof tmp, "ERROR:%zu\n", ps.err_pos);
                out.len = 0;
                buf_append(&out, tmp, (size_t)n);
                break;
            }
        }
        pos = line_end + 1;
    }
    free(ps.stk);
    free(ps.scratch);
    return out;
}

// The whole file is read once into one buffer that doubles when full.
static int read_file(const char *path, char **data, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "calc: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }
    size_t cap = 1 << 16, n = 0;
    char *buf = xrealloc(NULL, cap);
    for (;;) {
        if (n == cap) {
            cap *= 2;
            buf = xrealloc(buf, cap);
        }
        size_t got = fread(buf + n, 1, cap - n, f);
        n += got;
        if (got == 0) break;
    }
    if (ferror(f)) {
        fprintf(stderr, "calc: cannot read '%s': %s\n", path, strerror(errno));
        fclose(f);
        free(buf);
        return -1;
    }
    fclose(f);
    *data = buf;
    *len = n;
    return 0;
}

static int ensure_dir(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        fprintf(stderr, "calc: '%s' exists and is not a folder\n", dir);
        return -1;
    }
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "calc: cannot create folder '%s': %s\n", dir, strerror(errno));
        return -1;
    }
    return 0;
}

static int process_file(const char *in_path, const char *base, const char *out_dir) {
    char *data = NULL;
    size_t len = 0;
    if (read_file(in_path, &data, &len) != 0) return -1;

    Buf out = evaluate_file(data, len);
    free(data);

    int rc = -1;
    if (ensure_dir(out_dir) == 0) {
        size_t nlen = strlen(base) + sizeof "_" STUDENT_NAME "_" STUDENT_LASTNAME "_" STUDENT_ID ".txt";
        char *name = xrealloc(NULL, nlen);
        snprintf(name, nlen, "%s_" STUDENT_NAME "_" STUDENT_LASTNAME "_" STUDENT_ID ".txt", base);
        char *path = join_path(out_dir, name);
        FILE *f = fopen(path, "wb");
        if (!f) {
            fprintf(stderr, "calc: cannot write '%s': %s\n", path, strerror(errno));
        } else {
            size_t w = out.len ? fwrite(out.data, 1, out.len, f) : 0;
            if (w != out.len || fclose(f) != 0) fprintf(stderr, "calc: error writing '%s'\n", path);
            else rc = 0;
        }
        free(path);
        free(name);
    }
    free(out.data);
    return rc;
}

static void usage(void) {
    fprintf(stderr, "usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
}

static int process_dir(const char *dir, const char *out_dir) {
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "calc: cannot open folder '%s': %s\n", dir, strerror(errno));
        return -1;
    }
    int rc = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strlen(e->d_name) <= 4 || !ends_with_txt(e->d_name)) continue;
        char *path = join_path(dir, e->d_name);
        struct stat st;
        // S_ISREG skips subfolders (even ones named "x.txt").
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            char *base = copy_n(e->d_name, strlen(e->d_name) - 4);
            if (process_file(path, base, out_dir) != 0) rc = -1;
            free(base);
        }
        free(path);
    }
    closedir(d);
    return rc;
}

int main(int argc, char **argv) {
    const char *dir = NULL, *out_dir = NULL, *input = NULL;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "-d") == 0 || strcmp(a, "--dir") == 0) {
            if (++i >= argc) { fprintf(stderr, "calc: option '%s' needs a value\n", a); usage(); return 2; }
            dir = argv[i];
        } else if (strcmp(a, "-o") == 0 || strcmp(a, "--output-dir") == 0) {
            if (++i >= argc) { fprintf(stderr, "calc: option '%s' needs a value\n", a); usage(); return 2; }
            out_dir = argv[i];
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "calc: unknown option '%s'\n", a);
            usage();
            return 2;
        } else if (!input) {
            input = a;
        } else {
            fprintf(stderr, "calc: more than one input file given\n");
            usage();
            return 2;
        }
    }
    if ((!dir && !input) || (dir && input)) {
        usage();
        return 2;
    }

    char *default_out = NULL;
    char *base = path_basename(dir ? dir : input);
    if (!dir && ends_with_txt(base) && strlen(base) > 4) base[strlen(base) - 4] = '\0';
    if (!out_dir) {
        size_t n = strlen(base) + sizeof "_" STUDENT_NAME "_" STUDENT_ID;
        default_out = xrealloc(NULL, n);
        snprintf(default_out, n, "%s_" STUDENT_NAME "_" STUDENT_ID, base);
        out_dir = default_out;
    }

    int rc;
    if (dir) {
        struct stat st;
        if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "calc: '%s' is not a readable folder\n", dir);
            rc = -1;
        } else if (ensure_dir(out_dir) != 0) {
            rc = -1;
        } else {
            rc = process_dir(dir, out_dir);
        }
    } else {
        rc = process_file(input, base, out_dir);
    }

    free(base);
    free(default_out);
    return rc == 0 ? 0 : 1;
}