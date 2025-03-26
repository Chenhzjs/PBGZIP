# Paralleled Blocked GZIP
## build

### MacOS

#### Install gcc

```bash
brew install gcc
```
if gcc's version is 14, then you can use gcc-14 to compile.

#### Install omp

```bash
brew install libomp
sudo ln -s /path/to/libomp/omp.h /usr/local/include/omp.h
```
#### Compile the project

```bash
git clone https://github.com/Chenhzjs/PBGZIP.git
cd PBGZIP
git submodule update --init --recursive

```

You need to compile Intel TBB library first.

```bash
cd third_party
cmake -B build
cd build
make -j 4
sudo make install
```

Then enter the root dir to compile the project.
 
```bash
cmake -B build
cmake --build build
```

Run with the following command.

```bash
./build/deflate raw_data.txt compress.txt 8 -c
```

### Linux

#### Install gcc

```bash
sudo apt install gcc
```
Linux's GNU Compiler usually has openmp.

#### Compile the project

```bash
git clone https://github.com/Chenhzjs/PBGZIP.git
cd PBGZIP
git submodule update --init --recursive

```

You need to compile Intel TBB library first.

```bash
cd third_party
cmake -B build
cd build
make -j 4
sudo make install
```

Then enter the root dir to compile the project.
 
```bash
cmake -B build
cmake --build build
```

Run with the following command.

```bash
./build/deflate raw_data.txt compress.txt 8 -c
```
