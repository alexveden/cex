#pragma once

#if !defined(cex$enable_minimal)
#include "cex_base.h"
#include "sbuf.h"
#include "ds.h"

#define _CexTknList                                                                                \
    X(eof)                                                                                         \
    X(unk)                                                                                         \
    X(error)                                                                                       \
    X(ident)                                                                                       \
    X(number)                                                                                      \
    X(string)                                                                                      \
    X(char)                                                                                        \
    X(comment_single)                                                                              \
    X(comment_multi)                                                                               \
    X(preproc)                                                                                     \
    X(lparen)                                                                                      \
    X(rparen)                                                                                      \
    X(lbrace)                                                                                      \
    X(rbrace)                                                                                      \
    X(lbracket)                                                                                    \
    X(rbracket)                                                                                    \
    X(bracket_block)                                                                               \
    X(brace_block)                                                                                 \
    X(paren_block)                                                                                 \
    X(star)                                                                                        \
    X(minus)                                                                                       \
    X(plus)                                                                                        \
    X(dot)                                                                                         \
    X(comma)                                                                                       \
    X(eq)                                                                                          \
    X(colon)                                                                                       \
    X(question)                                                                                    \
    X(eos)                                                                                         \
    X(typedef)                                                                                     \
    X(func_decl)                                                                                   \
    X(func_def)                                                                                    \
    X(macro_const)                                                                                 \
    X(macro_func)                                                                                  \
    X(var_decl)                                                                                    \
    X(var_def)                                                                                     \
    X(cex_module_struct)                                                                           \
    X(cex_module_decl)                                                                             \
    X(cex_module_def)                                                                              \
    X(global_misc)                                                                                 \
    X(count)

typedef enum CexTkn_e
{
#define X(name) CexTkn__##name,
    _CexTknList
#undef X
} CexTkn_e;

extern const char* CexTkn_str[];

typedef struct cex_token_s
{
    CexTkn_e type;
    str_s value;
} cex_token_s;

typedef struct CexParser_c
{
    char* content;     // full content
    char* cur;         // current cursor in the source
    char* content_end; // last pointer of the source
    u32 line;          // current cursor line relative to content beginning
    u32 col;           // current cursor column relative to start of the line
    bool fold_scopes;  // count all {} / () / [] as a single token CexTkn_*_block
} CexParser_c;

typedef struct cex_decl_s
{
    str_s name;       // function/macro/const/var name
    str_s docs;       // reference to closest /// or /** block
    str_s body;       // reference to code {} if applicable
    sbuf_c ret_type;  // refined return type
    sbuf_c args;      // refined argument list
    const char* file; // decl file (must be set externally)
    u32 line;         // decl line in the source code
    CexTkn_e type;    // decl type (typedef, func, macro, etc)
    bool is_static;   // decl is a static func
    bool is_inline;   // decl is a inline func
} cex_decl_s;


CEX_NAMESPACE struct __cex_namespace__CexParser CexParser;

struct __cex_namespace__CexParser {
    // Autogenerated by CEX
    // clang-format off

    CexParser_c     (*create)(char* content, u32 content_len, bool fold_scopes);
    void            (*decl_free)(cex_decl_s* decl, IAllocator alloc);
    cex_decl_s*     (*decl_parse)(CexParser_c* lx, cex_token_s decl_token, arr$(cex_token_s) children, char* ignore_keywords_pattern, IAllocator alloc);
    cex_token_s     (*next_entity)(CexParser_c* lx, arr$(cex_token_s)* children);
    cex_token_s     (*next_token)(CexParser_c* lx);
    void            (*reset)(CexParser_c* lx);

    // clang-format on
};

#endif
