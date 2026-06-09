# WebOS Bootloader

The bootloader is the first WASM module loaded by the host runtime. It is responsible for:

1. **Hardware initialization** — Setting up the GPU and display
2. **Kernel loading** — Loading the kernel WASM module via the dynamic linker
3. **Control transfer** — Passing execution to the kernel

## Boot Sequence

```
Host Runtime
  └─> bootloader.wasm (main)
        ├─> GPU initialization (gpu_init.c)
        ├─> Load kernel.wasm (loader.c)
        └─> Transfer control to kernel
```

## Building

```bash
make
```

Output: `public/wasm/bootloader.wasm`

## Module Type

The bootloader is tagged as `wli` (library interface) since it provides boot services to the kernel.
