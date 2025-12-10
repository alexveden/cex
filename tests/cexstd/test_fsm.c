#include "src/all.c"
#include "cexstd/fsm/fsm.c"
#include "cexstd/testing/fff.h"

IAllocator allocator;

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(Exc, state_1, fsm_c*, const fsm_event_s*);
FAKE_VALUE_FUNC(Exc, state_2, fsm_c*, const fsm_event_s*);
FAKE_VALUE_FUNC(Exc, state_3, fsm_c*, const fsm_event_s*);
FAKE_VALUE_FUNC(Exc, state_super, fsm_c*, const fsm_event_s*);

Exception
state_1_sideeff(fsm_c* self, const fsm_event_s* e)
{
    switch (e->sig) {
        case FsmSignal__exit:
            return EOK;
        case FsmSignal__user:
            return fsm.trans(self, state_2);
        default:
            return "not expected";
    }
}

Exception
state_super_sideeff(fsm_c* self, const fsm_event_s* e)
{
    switch (e->sig) {
        case 77:
            return EOK;
        case 666:
            return fsm.super(self, e);
        case FsmSignal__user:
            return fsm.trans(self, state_2);
        default:
            return Error.assert;
    }
}

Exception
state_2_sideeff(fsm_c* self, const fsm_event_s* e)
{
    switch (e->sig) {
        case FsmSignal__entry:
            return fsm.trans(self, state_3);
        case FsmSignal__exit:
            return EOK;
        case FsmSignal__user:
            return fsm.super(self, e);
        default:
            return "not expected";
    }
}


Exception mocks_reset()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    RESET_FAKE(state_1);
    RESET_FAKE(state_2);
    RESET_FAKE(state_3);
    RESET_FAKE(state_super);
    return EOK;
}


test$case(test_fsm_create)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };
    tassert_er(EOK, fsm.create(&f, 9, state_1, NULL));
    tassert(f._fsm_type == 9);
    tassert(f._handler_super == NULL);
    tassert(f._handler == state_1);
    tassert_eq(state_1_fake.call_count, 0);

    tassert_er(EOK, fsm.init(&f));
    tassert_eq(state_1_fake.call_count, 1);
    tassert(state_1_fake.arg0_val == &f);
    tassert(state_1_fake.arg1_val == &fsm_event_entry);

    return EOK;
}

test$case(test_fsm_trans)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };
    tassert_er(EOK, fsm.create(&f, 9, state_1, NULL));
    tassert_er(EOK, fsm.trans(&f, state_2));

    // NOTE: fsm.trans() doesn't call anything
    tassert_eq(state_1_fake.call_count, 0);
    tassert_eq(state_2_fake.call_count, 0);

    // NOTE: fsm.trans() just replacing _handler
    tassert(f._handler_super == NULL);
    tassert(f._handler == state_2);

    return EOK;
}

test$case(test_fsm_dispatch)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };

    state_1_fake.custom_fake = state_1_sideeff;

    tassert_er(EOK, fsm.create(&f, 9, state_1, NULL));
    tassert(f._handler_super == NULL);
    tassert(f._handler == state_1);

    fsm_event_s sig1 = (fsm_event_s){ .sig = FsmSignal__user };
    tassert_er(EOK, fsm.dispatch(&f, &sig1));
    tassert(f._handler_super == NULL);
    tassert(f._handler == state_2);

    tassert_eq(state_1_fake.call_count, 2);
    tassert_eq(state_2_fake.call_count, 1);
    tassert(state_1_fake.arg0_history[0] == &f);
    tassert(state_1_fake.arg0_history[1] == &f);
    tassert(state_1_fake.arg1_history[0]->sig == FsmSignal__user);
    tassert(state_1_fake.arg1_history[1]->sig == FsmSignal__exit);

    tassert(state_2_fake.arg0_history[0] == &f);
    tassert(state_2_fake.arg1_history[0]->sig == FsmSignal__entry);


    return EOK;
}

test$case(test_fsm_dispatch_entry_transition)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };

    state_1_fake.custom_fake = state_1_sideeff;
    state_2_fake.custom_fake = state_2_sideeff;

    tassert_er(EOK, fsm.create(&f, 9, state_1, state_super));
    tassert(f._handler_super == state_super);
    tassert(f._handler == state_1);

    fsm_event_s sig1 = (fsm_event_s){ .sig = FsmSignal__user };
    tassert_er(EOK, fsm.dispatch(&f, &sig1));
    tassert(f._handler_super == state_super);
    tassert(f._handler == state_3);

    tassert_eq(state_1_fake.call_count, 2);
    tassert_eq(state_2_fake.call_count, 2);
    tassert_eq(state_3_fake.call_count, 1);
    tassert_eq(state_super_fake.call_count, 0);
    tassert(state_1_fake.arg0_history[0] == &f);
    tassert(state_1_fake.arg0_history[1] == &f);
    tassert(state_1_fake.arg1_history[0]->sig == FsmSignal__user);
    tassert(state_1_fake.arg1_history[1]->sig == FsmSignal__exit);

    tassert(state_2_fake.arg0_history[0] == &f);
    tassert(state_2_fake.arg1_history[0]->sig == FsmSignal__entry);
    tassert(state_2_fake.arg0_history[1] == &f);
    tassert(state_2_fake.arg1_history[1]->sig == FsmSignal__exit);

    tassert(state_3_fake.arg0_history[0] == &f);
    tassert(state_3_fake.arg1_history[0]->sig == FsmSignal__entry);


    return EOK;
}

test$case(test_fsm_super_non_user_sign_ignored)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };


    state_super_fake.custom_fake = state_super_sideeff;
    tassert_er(EOK, fsm.create(&f, 9, state_1, state_super));
    tassert(f._handler_super == state_super);
    tassert(f._handler == state_1);

    // NOTE: fsm.super() ignores all non user signals
    for (EFsmSignal_e sig = 0; sig < FsmSignal__user; sig++) {
        fsm_event_s ev = { .sig = sig };
        tassert_er(EOK, fsm.super(&f, &ev));
    }
    tassert_eq(state_super_fake.call_count, 0);


    return EOK;
}

test$case(test_fsm_dispatch_super_call)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };

    state_super_fake.custom_fake = state_super_sideeff;
    state_2_fake.custom_fake = state_2_sideeff;
    tassert_er(EOK, fsm.create(&f, 9, state_2, state_super));
    tassert(f._handler_super == state_super);
    tassert(f._handler == state_2);


    fsm_event_s sig1 = (fsm_event_s){ .sig = FsmSignal__user };
    tassert_er(EOK, fsm.dispatch(&f, &sig1));
    tassert(f._handler_super == state_super);
    tassert(f._handler == state_2);

    tassert_eq(state_super_fake.call_count, 1);
    tassert_eq(state_2_fake.call_count, 1);

    tassert(state_2_fake.arg0_history[0] == &f);
    tassert(state_2_fake.arg1_history[0]->sig == FsmSignal__user);

    tassert(state_super_fake.arg0_history[0] == &f);
    tassert(state_super_fake.arg1_history[0]->sig == FsmSignal__user);

    return EOK;
}

test$case(test_fsm_dispatch_super_call_from_super_forbidden)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };

    state_super_fake.custom_fake = state_super_sideeff;
    state_2_fake.custom_fake = state_2_sideeff;
    tassert_er(EOK, fsm.create(&f, 9, state_2, state_super));


    fsm_event_s sig1 = (fsm_event_s){ .sig = 666 };
    tassert_er(Error.assert, fsm.super(&f, &sig1));

    return EOK;
}

test$case(test_fsm_dispatch_super_call_transition)
{
    e$ret(mocks_reset());
    fsm_c f = { ._fsm_type = 33 };

    state_super_fake.custom_fake = state_super_sideeff;
    state_2_fake.custom_fake = state_2_sideeff;
    tassert_er(EOK, fsm.create(&f, 9, state_2, state_super));


    fsm_event_s sig1 = (fsm_event_s){ .sig = FsmSignal__user };
    tassert_er(EOK, fsm.super(&f, &sig1));

    return EOK;
}

test$main();
