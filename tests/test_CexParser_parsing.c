#define CEX_LOG_LVL 8
#include "src/all.c"

#define $code(...) cex$stringize(__VA_ARGS__)

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(test_token_next_entity)
{
    // clang-format off
    char* code = 
        "#include <gfoo>\n"
        "// foo\n"
        "#define BAR 2\n"
        "#define ca$ma(a, b, c) 2\n"
        "#define ca$ma() 2\n"
        "#define __test$ 2\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);
        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__preproc);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_const);
        tassert_eq(arr$len(items), 2);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_func);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_func);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_const);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__eof);
        tassert_eq(arr$len(items), 0);
    }
    return EOK;
}

test$case(test_extern_vars)
{
    // clang-format off
    char* code = 
        "extern int \nglobal_var;\n"
        "extern int add(int a, int b);  // Optional 'extern'\n"
        "extern int __attribute__\n((visibility(\"hidden\"))) internal_var;\n"
        "int \nglobal_var = 2;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_decl);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_decl);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: %s children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_def);
        tassert_eq(arr$len(items), 4);
    }
    return EOK;
}

test$case(test_extern_funcs)
{
    // clang-format off
    char* code = 
        "int add(int a, int b); \n"
        "int __attribute__\n((foo))\nglobal_var;\n"
        "extern int __attribute__\n((foo)) add(int a, int b); \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__global_misc);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %zu\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 7);
    }
    return EOK;
}

test$case(test_funcs_decl_def)
{
    // clang-format off
    char* code = 
        "int add(int a, int b); \n"
        "int add(int a, int b) { return 0; } \n"
        "__attribute__((inline)) int add(int a, int b) { return 0; } \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);
        tassert_eq(arr$len(items), 6);
    }
    return EOK;
}

test$case(test_struct_def)
{
    // clang-format off
    char* code = 
        "typedef struct cex_token_s { CexTkn_e type;  str_s value;} cex_token_s;\n"
        "struct cex_token_s { CexTkn_e type;  str_s value;};\n"
        "enum cex_token_s { A, B, C};\n"
        "union cex_token_s { int a; int b;};\n"
        "struct cex_token_s __attribute__((packed, aligned(16), used))) { CexTkn_e type;  str_s value;};\n"
        "struct cex_token_s my_foo(int a);\n"
        "struct B;\n"
        "struct cex_token_s { CexTkn_e type;  str_s value;} foo = {0};\n"
        "extern struct cex_token_s;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 6);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 3);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_def);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_decl);
        tassert_eq(arr$len(items), 4);
    }
    return EOK;
}

test$case(test_cex_struct_def)
{
    // clang-format off
    char* code = 
        "CEX_NAMESPACE struct __cex_namespace__io io;\n"
        "CEX_NAMESPACE_DEF struct __cex_namespace__io io = { };"
        "struct __cex_namespace__io { void            (*fclose)(FILE** file); };\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_decl);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_def);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_struct);
        tassert_eq(arr$len(items), 4);
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}


test$case(test_funcs_decl_parse)
{
    // clang-format off
    char* code = 
        "/** my doc */ int add(int a, int b); \n"
        "int add(int a, int b) { return 0; } \n"
        "static inline int add(int a, int b) { return 0; } \n"
        "__attribute__((foo)) int add2(int a, int b) { return 0; } \n"
        "extern int add2(int __attribute__((foo)) a, int restrict * b) { return 0; } \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 5);


        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_decl);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body.buf, NULL);
        tassert_eq(d->docs, str$s("/** my doc */"));
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);
        tassert_eq(d->is_static, false);
        tassert_eq(d->is_inline, false);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);
        tassert_eq(d->is_static, true);
        tassert_eq(d->is_inline, true);


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__func_def);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add2"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__func_def);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add2"));
        tassert_eq(d->args, "int a, int* b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_funcs_def_parse_args)
{
    // clang-format off
    char* code = 
        "int add(int a, int* b); \n"
        "int add(int a, int b) { return 0; } \n"
        "int add(int a, int restrict b) { return 0; } \n"
        "int add(int a, int foo(bar) b) { return 0; } \n"
        "int add(int a, int foo const* b) { return 0; } \n"
        "int add(int a, int /*foo const*/ * b) { return 0; } \n"
        "int add_va(int a, ...) { return 0; } \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);


        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_decl);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int* b");
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, "foo", _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, "foo", _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int const* b");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, "foo", _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int* b");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, "foo", _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add_va"));
        tassert_eq(d->args, "int a,...");

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_funcs_def_multiline)
{
    // clang-format off
    char* code = 
    "static char*\n"
    "cexy_target_make(\n"
        "IAllocator allocator\n"
    ")\n"
    "{}\n";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);


        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("cexy_target_make"));
        tassert_eq(d->args, "IAllocator allocator");

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_funcs_decl_parse_ret_type)
{
    // clang-format off
    char* code = 
        "int add(int a, int b) { return 0; } \n"
        "int * add(int a, int b) { return 0; } \n"
        "struct foo * add(int a, int b) { return 0; } \n"
        "void add() { return 0; } \n"
        "char * * add() { return 0; } \n"
        "const char * * add() { return 0; } \n"
        "[[nodiscard]] int add() { return 0; } \n"
        "noreturn int add() { return 0; } \n"
        "_Noreturn int add() { return 0; } \n"
        "__int128 add(int a, int b) { return 0; } \n"
        "__attribute__((foo)) int add(int a, int b) { return 0; } \n"
        "int (*func())() {}\n"
        "static arr$(char*) str_split(const char* s, const char* split_by, IAllocator allc){return 0;}\n"
        "static /* comment */ arr$(char*) str_split(const char* s, const char* split_by, IAllocator allc){return 0;}\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int*");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "struct foo*");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "void");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "char**");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "const char**");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "__int128");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d == NULL);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("str_split"));
        tassert_eq(d->ret_type, "arr$(char*)");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("str_split"));
        tassert_eq(d->ret_type, "arr$(char*)");

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}



test$case(test_funcs_decl_parse_macros)
{
    // clang-format off
    char* code = 
        "/** my doc */ \n#define my$macro() asdadkja \n"
        "/// my doc \n#define my_macro(a, b, ...) a+b\n"
        "#define my$CONST 220981\n"
        "// some comment\n#define my$CONST 220981\n"
        "/* some comment */\n#define my$CONST 220981\n"
        "/*! some comment */  \n#define my$CONST 220981\n"
        "  #  define __my$ asd\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_func);


        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_func);
        tassert_eq(d->name, str$s("my$macro"));
        tassert_eq(d->args, "");
        tassert_eq(d->body, str$s(" asdadkja "));
        tassert_eq(d->docs, str$s("/** my doc */"));
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_func);
        tassert_eq(d->name, str$s("my_macro"));
        tassert_eq(d->args, "a, b, ...");
        tassert_eq(d->body, str$s(" a+b"));
        tassert_eq(d->docs, str$s("/// my doc"));


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__macro_const);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_const);
        tassert_eq(d->name, str$s("my$CONST"));
        tassert_eq(d->args, "");
        tassert_eq(d->body, str$s(" 220981"));
        tassert_eq(d->docs.buf, NULL);


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__macro_const);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_const);
        tassert_eq(d->name, str$s("my$CONST"));
        tassert_eq(d->args, "");
        tassert_eq(d->body, str$s(" 220981"));
        tassert_eq(d->docs.buf, NULL);

        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__macro_const);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_const);
        tassert_eq(d->name, str$s("my$CONST"));
        tassert_eq(d->args, "");
        tassert_eq(d->body, str$s(" 220981"));
        tassert_eq(d->docs.buf, NULL);

        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__macro_const);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_const);
        tassert_eq(d->name, str$s("my$CONST"));
        tassert_eq(d->args, "");
        tassert_eq(d->body, str$s(" 220981"));
        tassert_eq(d->docs, str$s("/*! some comment */"));

        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__macro_const);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_const);
        tassert_eq(d->name, str$s("__my$"));
        tassert_eq(d->args, "");
        tassert_eq(d->body, str$s(" asd"));
        tassert_eq(d->docs.buf, NULL);

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_funcs_decl_with_paren_attrs)
{
    // clang-format off
    char* code = 
        "int add(int a, int b) { return 0; } \n"
        "arr$(char*) add(int a, int [[attr]] b) { return 0; } \n"
        "int add(int a[1299], arr$(foo*) b); \n"
        "int add(int a[1299], arr$(foo*) b) __attribute__((sdod)); \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");
        tassert_eq(d->args, "int a, int b");
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->ret_type, "arr$(char*)");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_decl);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a[1299], arr$(foo*) b");
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_decl);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a[1299], arr$(foo*) b");
        tassert_eq(d->ret_type, "int");

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_extra_ignore_keywords)
{
    // clang-format off
    char* code = 
        "foo int add(int a, int b) { return 0; } \n"
        "int add(int a, int foo b) { return 0; } \n"
        "int add(int a, int foo(1,2,3) b) { return 0; } \n"
        "foo(bar) int add(int a, int bar(1,2,3) b) { return 0; } \n"
        "foo(bar) int add(int a, int bar(1,2,3) b) foo(ok, d) { return 0; } \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);

        char* extra_ignore = "(foo|bar)";

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, extra_ignore, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");
        tassert_eq(d->args, "int a, int b");
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, extra_ignore, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, extra_ignore, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, extra_ignore, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, extra_ignore, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->ret_type, "int");

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_cex_struct_def_decl)
{
    // clang-format off
    char* code = 
        "CEX_NAMESPACE struct __cex_namespace__io io;\n"
        "CEX_NAMESPACE_DEF struct __cex_namespace__io io = { };"
        "struct __cex_namespace__io { void            (*fclose)(FILE** file); };\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_decl);
        tassert_eq(arr$len(items), 5);
        tassert(d == NULL);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_def);
        tassert_eq(arr$len(items), 7);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__cex_module_def);
        tassert_eq(d->name, str$s("io"));
        tassert_eq(d->args, "");
        tassert_eq(d->ret_type, "");
        tassert_gt(d->body.len, 0);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_struct);
        tassert_eq(arr$len(items), 4);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__cex_module_struct);
        tassert_eq(d->name, str$s("io"));
        tassert_eq(d->args, "");
        tassert_eq(d->ret_type, "");
        tassert_gt(d->body.len, 0);
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_cex_decl_multiline)
{
    // clang-format off
    char* code = 
        "struct \n__cex_namespace__io \n{\n void            (*fclose)(FILE** file); \n};\n"
        "struct \n__cex_namespace__io \n{\n void            (*fclose)(FILE** file); \n};\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_struct);
        tassert(d != NULL);
        tassert_eq(d->line, 1);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_struct);
        tassert(d != NULL);
        tassert_eq(d->line, 6);
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_struct_decl_with_attr)
{
    // clang-format off
    char* code = $code(
        typedef struct
        {
            struct
            {
                u32 magic : 16;   // used for sanity checks
                u32 elsize : 8;   // maybe multibyte strings in the future?
                u32 nullterm : 8; // always zero to prevent usage of direct buffer
            } header;
            u32 length;
            u32 capacity;
            const Allocator_i* allocator;
        } __attribute__((packed)) sbuf_head_s;
    );
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert(d != NULL);

        tassert_eq(d->type, CexTkn__typedef);
        tassert_eq(d->name, str$s("sbuf_head_s"));
        tassert_eq(d->args, "");
        tassert_eq(d->ret_type, "typedef struct");
        tassert_gt(d->body.len, 0);
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}


test$case(test_simple_typedef_decl)
{
    // clang-format off
    char* code = 
        "typedef char* sbuf_c;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert(d != NULL);
        tassert_eq(d->name, str$s("sbuf_c"));
        tassert_eq(d->args, "");
        tassert_eq(d->ret_type, "typedef char*");
        tassert_eq(d->body.buf, NULL);

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_unnamed_types)
{
    // clang-format off
    char* code = $code(
        enum
        {
            CEXDS_SH_NONE,
            CEXDS_SH_DEFAULT,
            CEXDS_SH_STRDUP,
            CEXDS_SH_ARENA
        };
    );
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert(d == NULL);
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_unnamed_types_typedef)
{
    // clang-format off
    char* code = 
        "typedef int CEXDS_SIPHASH_2[sizeof(size_t) == 8 ? 1 : -1];";
    // clang-format on
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _)
    {
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug(
            "Entity:  type: %d type_str: '%s' children: %zu\n%S\n",
            t.type,
            CexTkn_str[t.type],
            arr$len(items),
            t.value
        );
        tassert_eq(t.type, CexTkn__typedef);
        tassert(d != NULL);
        tassert_eq(d->name, str$s("CEXDS_SIPHASH_2"));
        tassert_eq(d->args, "");
        tassert_eq(d->ret_type, "typedef int");
        tassert_eq(d->body, str$s("[sizeof(size_t) == 8 ? 1 : -1]"));
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_multiline_single_docstr)
{
    // clang-format off
    char* code = 
        "/// A \n"
        "/// B \n"
        "/// C \n"
        "typedef struct sbuf_c\n"
        "{} sbuf_c;\n"
    ;
    // clang-format on
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _)
    {
        cex_decl_s* d = NULL;
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser.decl_parse(&lx, t, items, NULL, _);
        log$debug(
            "Entity:  type: %d type_str: '%s' children: %zu\n%S\n",
            t.type,
            CexTkn_str[t.type],
            arr$len(items),
            t.value
        );
        tassert_eq(t.type, CexTkn__typedef);
        tassert(d != NULL);

        tassert_eq(d->type, CexTkn__typedef);
        tassert_eq(d->name, str$s("sbuf_c"));
        tassert_eq(d->args, "");
        tassert_eq(d->ret_type, "typedef struct");
        tassert_gt(d->body.len, 0);
        tassert_eq(d->docs, str$s("/// A \n/// B \n/// C"));
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_struct_def_with_serde_attr)
{
    // clang-format off
    char* code = 
        "json$$struct(.name = \"MyStock\");\n"
        "/// My Doc\n"
        "typedef struct my_struct { CexTkn_e type;  str_s value;} my_struct1;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__typedef);
        tassert_eq(d->name, str$s("my_struct1")); // using last name after typedef scope
        tassert_eq(d->ret_type, "typedef struct");
        tassert_eq(d->body, str$s("{ CexTkn_e type;  str_s value;}"));
        tassert_eq(d->args, "");
        tassert_eq(d->docs, str$s("/// My Doc"));
        tassert_eq(d->attr_count, 1);
        tassert_eq(d->attr[0], str$s("json$$struct(.name = \"MyStock\");"));
    }
    return EOK;
}

test$case(test_struct_def_with_serde_attr_no_args)
{
    // clang-format off
    char* code = 
        "json$$struct();\n"
        "typedef struct my_struct { CexTkn_e type;  str_s value;} my_struct1;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);

        auto d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__typedef);
        tassert_eq(d->name, str$s("my_struct1")); // using last name after typedef scope
        tassert_eq(d->attr_count, 1);
        tassert_eq(d->attr[0], str$s("json$$struct();"));
        tassert_eq(d->ret_type, "typedef struct");
        tassert_eq(d->body, str$s("{ CexTkn_e type;  str_s value;}"));
        tassert_eq(d->args, "");
    }
    return EOK;
}

test$case(test_struct_def_with_serde_attr_no_parens)
{
    // clang-format off
    char* code = 
        "json$$struct\n"
        "typedef struct my_struct { CexTkn_e type;  str_s value;} my_struct1;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__error);
        tassert_eq(arr$len(items), 0);
        tassert_eq(lx.error, "cex$$attribute requires ()");

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d == NULL);
    }
    return EOK;
}

test$case(test_struct_no_typedef)
{
    // clang-format off
    char* code = 
        "struct my_struct { CexTkn_e type;  str_s value;};\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__typedef);
        tassert_eq(d->name, str$s("my_struct")); // using last name after typedef scope
        tassert_eq(d->ret_type, "struct");
        tassert_eq(d->body, str$s("{ CexTkn_e type;  str_s value;}"));
        tassert_eq(d->args, "");
    }
    return EOK;
}

test$case(test_struct_no_name)
{
    // clang-format off
    char* code = 
        "struct { CexTkn_e type;  str_s value;};\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);

        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d == NULL);
    }

    return EOK;
}

test$case(test_struct_with_many_attributes)
{
    // clang-format off
    char* code = 
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "typedef struct my_struct { CexTkn_e type;  str_s value;} my_struct1;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);

        auto d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__typedef);
        tassert_eq(d->name, str$s("my_struct1")); // using last name after typedef scope
        tassert_eq(d->attr_count, 7);
        tassert_eq(d->attr[0], str$s("json$$struct();"));
        tassert_eq(d->ret_type, "typedef struct");
        tassert_eq(d->body, str$s("{ CexTkn_e type;  str_s value;}"));
        tassert_eq(d->args, "");
    }
    return EOK;
}

test$case(test_struct_with_many_attributes_too_many)
{
    // clang-format off
    char* code = 
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "json$$struct();\n"
        "typedef struct my_struct { CexTkn_e type;  str_s value;} my_struct1;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);

        auto d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d == NULL);
        tassert_eq(lx.error, "Too many cex$$attributes");
    }
    return EOK;
}

test$case(test_attr_for_func)
{
    // clang-format off
    char* code = 
        "some$$attr();\n"
        "int foo(int a);"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);

        auto d = CexParser.decl_parse(&lx, t, items, NULL, _);
        tassert(d != NULL);
        tassert_eq(lx.error, EOK);
        tassert_eq(d->attr_count, 1);
        tassert_eq(d->attr[0], str$s("some$$attr();"));
    }
    return EOK;
}

test$case(test_attr_requires_semicolon)
{
    // clang-format off
    char* code = 
        "some$$attr()\n"
        "int foo(int a);"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %zu\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__error);
        tassert_eq(lx.error, "cex$$attribute missing semicolon");
    }
    return EOK;
}
test$main();
