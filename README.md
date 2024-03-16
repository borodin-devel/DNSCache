Open a terminal and navigate to your project's directory.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

To run tests:

`./DNSCacheTest`

Alternatively, you can use CTest to run the tests:

`ctest`
