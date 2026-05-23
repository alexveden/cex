/* NOLINTBEGIN */                                                                          \
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
/* NOLINTEND */                                                                            \

/*
## Credits

CEX contains some code and ideas from the following projects, all of them licensed under MIT license (or Public Domain):

1. [nob.h](https://github.com/tsoding/nob.h) - by Tsoding / Alexey Kutepov, MIT/Public domain, great idea of making self-contained build system, great youtube channel btw
2. [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h) - MIT/Public domain, by Sean Barrett, CEX arr$/hm$ are refactored versions of STB data structures, great idea 
3. [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h) - MIT/Public domain, by Sean Barrett, I refactored it, fixed all UB warnings from UBSAN, added CEX specific formatting
4. [minirent.h](https://github.com/tsoding/minirent) - Alexey Kutepov, MIT license, WIN32 compatibility lib 
5. [subprocess.h](https://github.com/sheredom/subprocess.h) - by Neil Henning, public domain, used in CEX as a daily driver for `os$cmd` and process communication
6. [utest.h](https://github.com/sheredom/utest.h) - by Neil Henning, public domain, CEX test$ runner borrowed some ideas of macro magic for making declarative test cases
7. [c3-lang](https://github.com/c3lang/c3c) - I borrowed some ideas about language features from C3, especially `mem$scope`, `mem$`/`tmem$` global allocators, scoped macros too.


## License

```

MIT License

Copyright (c) 2024-2026 Aleksandr Vedeneev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```
*/
