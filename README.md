# sccvgm.hpp - SCC VGM Driver

This program is a C++ single header format emulator that plays the VGM of the AY-3-8910 (PSG) and Sound Creative Chip (SCC).

Some of the code included in the VGS-Zero implementation has been adapted to the MIT license for easy use.

The PSG and SCC emulator core parts use the EMU2149 and EMU2212 published by Digital Sound Antiques.

- EMU2149: https://github.com/digital-sound-antiques/emu2149
- EMU2212: https://github.com/digital-sound-antiques/emu2212

For example, it is intended for use in cases where you want to play SCC sound source BGM created by [Furnace](https://tildearrow.org/furnace/) in your own games.

## How to Use

Just add [sccvgm.hpp](sccvgm.hpp) to your project and `#include` it.

### 1. Include

```c++
#include "sccvgm.hpp"
```

### 2. Make an Instance

```c++
scc::VgmDriver* scc = new scc::VgmDriver();
```

### 3. Load VGM File

Call `scc::VgmDriver::load` with the data loaded on-memory and the data size.

```c++
if (scc->load(vgmData, vgmSize)) {
    puts("success!");
} else {
    puts("failed!");
}
```

VGM files must be __Version 1.61 or later__.

### 4. Render Sampling Data

You can call `scc::VgmDriver::render` to get sampled data of the size you want.

```c++
scc->render(samplingBuffer, samplingNumber);
```

- The quantization unit is fixed at 16 bits (2 bytes).
- The `samplingNumber` is the size of `samplingBuffer` divided by 2.

## Example

We provide an [example](./example/) implementation of exporting SCC VGM files in wav format.

## License

[MIT](LICENSE.txt)
