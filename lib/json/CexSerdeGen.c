#include "CexSerdeGen.h"
#if defined(CEX_BUILD)


Exception
CexSerdeGen_create(CexSerdeGen_c* self, IAllocator allc, CexSerdeGen_kw* kwargs)
{
    uassert(self);
    uassert(allc->meta.is_arena && "Expected arena allocator");
    u32 def_initial_capacity = 100 * 1024;
    char* def_namespace = "serde";
    char* def_workdir = ".";
    char* def_outdir = NULL;

    if (kwargs) {
        if (kwargs->namespace) { def_namespace = kwargs->namespace; }
        if (kwargs->buf_initial_capacity) { def_initial_capacity = kwargs->buf_initial_capacity; }
        if (kwargs->workdir) { def_workdir = kwargs->workdir; }
        if (kwargs->outdir) { def_outdir = kwargs->outdir; }
    }
    if (!def_outdir) { def_outdir = def_workdir; }

    auto fstats = os.fs.stat(def_workdir);
    if (!fstats.is_valid) {
        return e$raise(Error.not_found, "Working directory not exists: `%s`", def_workdir);
    }
    if (!fstats.is_directory) {
        return e$raise(Error.argument, "Expected directory, got file? `%s`", def_workdir);
    }

    *self = (CexSerdeGen_c){
        .allc = allc,
        .types = hm$new(self->types, allc, .capacity = 256),
        .includes = arr$new(self->includes, allc, .capacity = 16),
        .namespace = def_namespace,
        .c_file_content = sbuf.create(def_initial_capacity, allc),
        .h_file_content = sbuf.create(def_initial_capacity, allc),
        .target = os$path_join(allc, def_workdir, "*.h"),
        .c_out_name = os$path_join(allc, def_outdir, str.fmt(allc, "%s.c", def_namespace)),
        .h_out_name = os$path_join(allc, def_outdir, str.fmt(allc, "%s.h", def_namespace)),
    };

    return EOK;
}


Exception
_CexSerdeGen__process_field_attr(
    CexSerdeGen_c* self,
    CexParser_c* lx,
    serdegen_field_s* field,
    cex_token_s t
)
{
    uassert(t.type == CexTkn__ident);

    (void)lx;
    (void)field;
    (void)t;
    if (str.slice.starts_with(t.value, str$s("serde$$field"))) {
        log$info("Processing %S\n", t.value);
        // FIX: broken stub!
        while ((t = CexParser.next_token(lx)).type) {
            if (t.type == CexTkn__rparen) {
                t = CexParser.next_token(lx);
                e$assert(t.type == CexTkn__ident);
                break;
            }
        }
    }


    cex_token_s prev_t = t;
    field->type = str.sstr(str.slice.clone(t.value, self->allc));
    if (str$eq(field->type, "sbuf_c") || str$eq(field->type, "str_s")) {
        field->flags.is_string = true;
    }

    while ((t = CexParser.next_token(lx)).type) {
        if (t.type == CexTkn__error) { return Error.integrity; }

        switch (t.type) {
            case CexTkn__eos: {
                e$assert(prev_t.type == CexTkn__ident);
                e$except_null (field->name = str.slice.clone(prev_t.value, self->allc)) {
                    return Error.memory;
                }
                goto end;
            } break;
            case CexTkn__star: {
                field->flags.is_ptr = true;
                if (str$eq(field->type, "char")) { field->flags.is_string = true; }
            } break;
            case CexTkn__ident:
                break;
            default: {
                e$assertf(false, "Unsupported token: %s\n", CexTkn_str[t.type]);
            }
        }


        prev_t = t;
    }

end:
    log$info("New field: name=%s, type=%S\n", field->name, field->type);

    return EOK;
}

Exception
_CexSerdeGen_codegen_serialize_field(CexSerdeGen_c* self, cex_codegen_s* cg$var, serdegen_field_s* f)
{
    (void)self;
    (void)f;
    e$assert(f->type.buf && f->type.len != 0);
    cg$pf("jw$key(\"%s\");", f->name);

    serdegen_type_s* field_type = hm$get(self->types, f->type);
    if (field_type) {
        // Project Type
        cg$scope ("e$except_silent (err, %s.%s.serialize(jw, item->%s)) ",
                  self->namespace,
                  field_type->name,
                  f->name) {
            cg$pn("jw->error = err;");
        }
    } else {
        // Primitive type
        cg$pf("jw$val(item->%s);", f->name);
    }
    cg$pn("");

    return EOK;
}

Exception
_CexSerdeGen_codegen_deserialize_field(
    CexSerdeGen_c* self,
    cex_codegen_s* cg$var,
    serdegen_field_s* f
)
{
    (void)self;
    (void)f;
    e$assert(f->type.buf && f->type.len != 0);

    cg$elseif ("str$eq(k, \"%s\")", f->name) {
        serdegen_type_s* field_type = hm$get(self->types, f->type);
        if (field_type) {
            // Project registered type

            // We need preallocate pointer to a new type!
            cg$if ("jr->type != JsonType__null") {
                if (f->flags.is_ptr) {
                    cg$pf("out_item->%s = mem$new(allc, %S);", f->name, f->type);
                    cg$if ("!out_item->%s", f->name) { cg$pn("return Error.memory;"); }
                }

                cg$pf("jr$egoto(jr, %s.%s.deserialize(jr, out_item->%s, allc), fail);",
                          self->namespace,
                          field_type->name,
                          f->name);
            }
            cg$else () { cg$pf("out_item->%s = NULL;", f->name); }
        } else if (f->flags.is_string) {
            if (str$eq(f->type, "char")) {
                uassert(f->flags.is_ptr);
                cg$if ("unlikely(!v.buf)") { cg$pf("out_item->%s = NULL;", f->name); }
                cg$else () { cg$pf("out_item->%s = str.slice.clone(v, allc);", f->name); }

            } else if (str$eq(f->type, "sbuf_c")) {
                cg$if ("unlikely(!v.buf)") { cg$pf("out_item->%s = NULL;", f->name); }
                cg$else () {
                    cg$pf(
                        "out_item->%s = sbuf.create(v.len + sizeof(sbuf_head_s) + 1, allc);",
                        f->name
                    );
                    cg$if ("unlikely(!out_item->%s)", f->name) { cg$pn("return Error.memory;"); }
                    cg$if ("unlikely(sbuf.appendf(&out_item->%s, \"%%S\", v))", f->name) {
                        cg$pn("return Error.memory;");
                    }
                }

            } else if (str$eq(f->type, "str_s")) {
                cg$if ("unlikely(!v.buf)") { cg$pf("out_item->%s = (str_s){0};", f->name); }
                cg$else () { cg$pf("out_item->%s = str.sstr(str.slice.clone(v, allc));", f->name); }

            } else {
                uassertf(false, "field type, not implemented yet: type=%S\n", f->type);
            }

        } else {
            // Primitive type
            cg$pf("jr$egoto(jr, str$convert(v, &out_item->%s), fail);", f->name);
        }
        // cg$pn("");
    }


    return EOK;
}

Exception
_CexSerdeGen_codegen_destroy_field(CexSerdeGen_c* self, cex_codegen_s* cg$var, serdegen_field_s* f)
{
    (void)self;
    (void)f;
    e$assert(f->type.buf && f->type.len != 0);

    serdegen_type_s* field_type = hm$get(self->types, f->type);
    if (field_type) {
        cg$pf("%s.%s.destroy(item->%s, allc);", self->namespace, field_type->name, f->name);
        if (f->flags.is_ptr) { cg$pf("mem$free(allc, item->%s);", f->name); }
    } else if (f->flags.is_string) {
        if (str$eq(f->type, "char")) {
            cg$pf("mem$free(allc, item->%s);", f->name);
        } else if (str$eq(f->type, "sbuf_c")) {
            cg$pf("sbuf.destroy(&item->%s);", f->name);
        } else if (str$eq(f->type, "str_s")) {
            cg$pf("mem$free(allc, item->%s.buf);", f->name);
        } else {
            uassertf(false, "field type, not implemented yet: type=%S\n", f->type);
        }
    } else {
        // Primitive type do nothing
    }

    return EOK;
}

Exception
_CexSerdeGen_generate_type(CexSerdeGen_c* self, cex_codegen_s* cg$var, serdegen_type_s* t)
{
    cg$pn("");
    cg$pn("//");
    cg$pf("// Autogenerated serde for %s type", t->name);
    cg$pn("//");


    //
    // serialize codegen
    //
    cg$func ("Exception %s__%s__serialize(jw_c* jw, %s* item) ", self->namespace, t->name, t->name) {
        cg$if ("!item") {
            cg$scope ("jw$scope(jw, JsonType__null)") { cg$pn("jw$val(NULL);"); }
            cg$pn("return EOK;");
        }

        cg$scope ("jw$scope(jw, JsonType__obj)") {
            for$each (it, t->fields) {
                e$ret(_CexSerdeGen_codegen_serialize_field(self, cg$var, it));
            }
        }

        cg$pn("return jw->error;");
    }

    //
    // print codegen
    //
    cg$func ("Exc %s__%s__print(%s* item, jw_kw* json_writer_kwargs) ",
             self->namespace,
             t->name,
             t->name) {
        cg$pn("jw_c jw;");
        cg$pn("jw_kw kwargs = {.stream = stdout, .indent = 0};");
        cg$if ("json_writer_kwargs") {
            cg$pn("kwargs = *json_writer_kwargs;");
            cg$if ("!kwargs.stream && !kwargs.buf") { cg$pn("kwargs.stream = stdout;"); }
        }
        cg$pn("e$ret(_cex_json__writer__create(&jw, &kwargs));");
        cg$pf("return %s.%s.serialize(&jw, item);", self->namespace, t->name);
    }

    //
    // deserialize codegen
    //
    cg$func ("Exception %s__%s__deserialize(jr_c* jr, %s* out_item, IAllocator allc) ",
             self->namespace,
             t->name,
             t->name) {
        cg$pn("uassert(jr != NULL);");
        cg$pn("uassert(out_item != NULL);");

        cg$scope ("jr$foreach(k, v, jr) ") {
            cg$if ("!k.buf") {
                cg$pn("jr->error = Error.integrity;");
                cg$pn("goto fail;");
            }
            for$each (it, t->fields) {
                e$ret(_CexSerdeGen_codegen_deserialize_field(self, cg$var, it));
            }
        }
        cg$pn("return EOK;");

        cg$dedent();
        cg$pn("fail: ");
        cg$indent();
        cg$pf("%s.%s.destroy(out_item, allc);", self->namespace, t->name);
        cg$pn("return jr->error;");
    }

    //
    // destroy codegen
    //
    cg$func ("void %s__%s__destroy(%s* item, IAllocator allc) ", self->namespace, t->name, t->name) {
        cg$pn("uassert(allc != NULL);");
        cg$if ("item") {
            for$each (it, t->fields) {
                e$ret(_CexSerdeGen_codegen_destroy_field(self, cg$var, it));
            }
            cg$pn("memset(item, 0, sizeof(*item));");
        }
    }

    return EOK;
}

Exception
CexSerdeGen_generate_full(CexSerdeGen_c* self)
{
    cg$init_scope(&self->h_file_content)
    {
        cg$pf("// Autogenerated serde engine by CEX");
        cg$pn("// DO NOT EDIT");
        cg$pn("//");
        cg$pn("#include \"cex.h\"");
        cg$pn("#include \"lib/json/json.h\"");
        for$each (it, self->includes) { cg$pf("#include \"%s\"", it); }
    }
    cg$init_scope(&self->c_file_content)
    {
        cg$pf("// Autogenerated serde engine by CEX");
        cg$pn("// DO NOT EDIT");
        cg$pn("//");
        cg$pf("#include \"%s.h\"", str.fmt(self->allc, self->namespace));

        for$each (it, self->types, arr$len(self->types)) {
            log$info("Type: %s #%d fields\n", it.value->name, arr$len(it.value->fields));
            e$ret(_CexSerdeGen_generate_type(self, cg$var, it.value));
        }
    }


    return EOK;
}

Exception
_CexSerdeGen_process_decl(CexSerdeGen_c* self, CexParser_c* lx, cex_decl_s* d, bool* has_serde)
{
    uassert(self);
    uassert(lx);
    uassert(d);
    uassert(has_serde);

    *has_serde = false;

    if (d->type == CexTkn__typedef && d->attr_count > 0) {
        for$each (attr, d->attr, d->attr_count) {
            if (!str.slice.starts_with(attr, str$s("serde$$struct"))) { continue; }
            log$info("Got item: type=%s Name: %S Attr: %S\n", CexTkn_str[d->type], d->name, attr);

            serdegen_type_s* stype = mem$new(self->allc, serdegen_type_s);
            if (!stype) { return Error.memory; }

            stype->fields = arr$new(stype->fields, self->allc);
            if (!stype->fields) { return Error.memory; }

            stype->name = str.slice.clone(d->name, self->allc);
            if (!stype->name) { return Error.memory; }

            // TODO: parse serde$$struct here for params

            CexParser_c sp = CexParser.create(d->body.buf, d->body.len, false);
            cex_token_s t = CexParser.next_token(&sp);
            e$assert(t.type == CexTkn__lbrace && "Expected { scope start");

            log$info("Type breakdown\n");
            while ((t = CexParser.next_token(&sp)).type) {
                if (t.type == CexTkn__error) {
                    log$error(CexParser$err_fmt(&sp, NULL));
                    return Error.integrity;
                }
                // log$info("Tok: type=%s Name: %S\n", CexTkn_str[t.type], t.value);
                if (t.type == CexTkn__ident) {
                    serdegen_field_s* field = mem$new(self->allc, serdegen_field_s);
                    if (!field) { return Error.memory; }
                    e$goto(_CexSerdeGen__process_field_attr(self, &sp, field, t), fail);
                    arr$push(stype->fields, field);
                } else if (t.type == CexTkn__comment_multi || t.type == CexTkn__comment_single) {
                    continue;
                } else if (t.type == CexTkn__rbrace) {
                    t = CexParser.next_token(&sp);
                    if (t.type != CexTkn__eof) { goto fail; }
                    break;
                }
            }

            if (hm$getp(self->types, str.sstr(stype->name))) {
                return e$raise(Error.exists, "Duplicate serde$$struct type name: %s", stype->name);
            }

            if (!hm$set(self->types, str.sstr(stype->name), stype)) { return Error.memory; }
            // hm$set(self->types, stype->name, stype);

            *has_serde = true;
            break;
        }
    }
    return EOK;

fail:
    log$error("Error processing serde$$struct\n%s %S\n%S\n", d->ret_type, d->name, d->body);
    return Error.integrity;
}

Exception
CexSerdeGen_process_file(CexSerdeGen_c* self, char* path)
{
    uassert(self);
    uassert(path);
    mem$arena(256 * 1024, _)
    {
        char* code = io.file.load(path, _);
        if (!code) { return e$raise(Error.io, "Error reading file: %s", path); }

        arr$(cex_token_s) items = arr$new(items, _);
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        bool has_serializable = false;
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__error) {
                log$error(CexParser$err_fmt(&lx, NULL));
                return lx.error;
            }

            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            bool has_serde = false;
            e$ret(_CexSerdeGen_process_decl(self, &lx, d, &has_serde));
            if (has_serde) { has_serializable = true; }
        }

        if (has_serializable) {
            uassert(str.ends_with(path, ".h"));
            arr$push(self->includes, path);
        }
    }

    return EOK;
}

Exception
CexSerdeGen_run(CexSerdeGen_c* self)
{
    uassert(sbuf.len(&self->c_file_content) == 0 && "Already processed");

    for$each (src_fn, os.fs.find(self->target, true, self->allc)) {
        io.printf("file: %s\n", src_fn);
        e$ret(CexSerdeGen.process_file(self, src_fn));
    }

    if (arr$len(self->types) == 0) {
        log$info(
            "CexSerdeGen: no serde$$struct types found in workdir: `%s`, skipping...",
            self->workdir
        );
        return EOK;
    }

    e$ret(CexSerdeGen.generate_full(self));

    // Check if output file is exists and it's a really CexSerdeGen generated
    mem$scope(tmem$, _)
    {
        if (os.path.exists(self->c_out_name)) {
            char* content = io.file.load(self->c_out_name, _);
            if (!str.starts_with(content, "// Autogenerated serde engine by CEX")) {
                return e$raise(
                    Error.integrity,
                    "File `%s` was not generated by CexSerdeGen, exiting",
                    self->c_out_name
                );
            }
        }
        if (os.path.exists(self->h_out_name)) {
            char* content = io.file.load(self->h_out_name, _);
            if (!str.starts_with(content, "// Autogenerated serde engine by CEX")) {
                return e$raise(
                    Error.integrity,
                    "File `%s` was not generated by CexSerdeGen, exiting",
                    self->h_out_name
                );
            }
        }
    }

    e$ret(io.file.save(self->c_out_name, self->c_file_content));
    e$ret(io.file.save(self->h_out_name, self->h_file_content));

    char* argv[] = { "process", self->c_out_name };
    e$ret(cexy.cmd.process(arr$len(argv), argv, NULL));

    return EOK;
}

const struct __cex_namespace__CexSerdeGen CexSerdeGen = {
    // Autogenerated by CEX
    // clang-format off

    .create = CexSerdeGen_create,
    .generate_full = CexSerdeGen_generate_full,
    .process_file = CexSerdeGen_process_file,
    .run = CexSerdeGen_run,

    // clang-format on
};

#endif
