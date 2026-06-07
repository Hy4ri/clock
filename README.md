# clock

A simple, sleek terminal clock written in C.

## Requirements

The project depends on:

- `gcc`
- `make`
- `pkg-config`
- `X11`
- `Xft`
- `fontconfig`

## Development with Nix

If you are using Nix, you can load a shell containing all build dependencies:

```bash
nix develop
```

## Build & Run

To build the clock:

```bash
make
```

To run it:

```bash
./clock
```

To clean build artifacts:

```bash
make clean
```
