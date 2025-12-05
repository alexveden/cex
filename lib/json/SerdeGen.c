#include "SerdeGen.h"


Exception
SerdeGen_create(SerdeGen_c* self, IAllocator allc)
{
    uassert(self);
    uassert(allc->meta.is_arena && "Expected arena allocator");

    *self = (SerdeGen_c){
        .allc = allc,
        .types = hm$new(self->types, allc),
    };

    return EOK;
}


Exception
SerdeGen_process_code(SerdeGen_c* self, char* code, usize code_len)
{
    uassert(self);

    if (!code) { return "code is NULL"; }
    if (code_len == 0) { code_len = str.len(code); }

    mem$arena(256 * 1024, _)
    {
        arr$(cex_token_s) items = arr$new(items, _);

        CexParser_c lx = CexParser.create(code, code_len, true);
        cex_token_s t;
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__error) {
                log$error(CexParser$err_fmt(&lx, NULL));
                return lx.error;
            }

            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            e$ret(SerdeGen.process_decl(self, &lx, d));
        }
    }

    return EOK;
}

Exception
_SerdeGen__process_field_attr(
    SerdeGen_c* self,
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
    // FIX: clone to str_s (new str.slice.clone_s() ??????????? )
    field->type = t.value;
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
SerdeGen_process_decl(SerdeGen_c* self, CexParser_c* lx, cex_decl_s* d)
{
    uassert(self);
    uassert(lx);
    uassert(d);

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
                    e$goto(_SerdeGen__process_field_attr(self, &sp, field, t), fail);
                    arr$push(stype->fields, field);
                } else if (t.type == CexTkn__comment_multi || t.type == CexTkn__comment_single) {
                    continue;
                } else if (t.type == CexTkn__rbrace) {
                    t = CexParser.next_token(&sp);
                    if (t.type != CexTkn__eof) { goto fail; }
                    break;
                }
            }

            if (hm$getp(self->types, stype->name)) {
                return e$raise(Error.exists, "Duplicate serde$$struct type name: %s", stype->name);
            }

            if(!hm$set(self->types, stype->name, stype)) {return Error.memory;}
            // hm$set(self->types, stype->name, stype);

            break;
        }
    }
    return EOK;

fail:
    log$error("Error processing serde$$struct\n%s %S\n%S\n", d->ret_type, d->name, d->body);
    return Error.integrity;
}

const struct __cex_namespace__SerdeGen SerdeGen = {
    // Autogenerated by CEX
    // clang-format off

    .create = SerdeGen_create,
    .process_code = SerdeGen_process_code,
    .process_decl = SerdeGen_process_decl,

    // clang-format on
};
