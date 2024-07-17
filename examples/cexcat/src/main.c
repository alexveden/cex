#include "App.c"
#include "cex.c"
#include <stdlib.h>

int
main(int argc, char* argv[])
{
  i32 ret_code = EXIT_SUCCESS;
  App_c app = { 0 };
  var allocator = allocators.heap.create();

  except(err, App.create(&app, argc, argv, allocator))
  {
    if (err != Error.argsparse){
      uptraceback(err, "App.create(&app, argc, argv, allocator))");
    }
    ret_code = EXIT_FAILURE;
    goto shutdown;
  }

  except_traceback(err, App.main(&app, allocator))
  {
    ret_code = EXIT_FAILURE;
    goto shutdown;
  }

shutdown:
  App.destroy(&app, allocator);
  allocators.heap.destroy();
  return ret_code;
}
