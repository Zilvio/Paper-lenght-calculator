# Paper-lenght-calculator
A simple program that sums the total lenght of technical boards chosen for calculating the price of printing said boards.

# REQUIREMENTS
Download and install [MSYS2](https://www.msys2.org/), open the MINGW-64 terminal to download POPPLER libraries through this line.
```bash
pacman -S mingw-w64-x86_64-poppler
```
Once poppler is downloaded, extract all of the libraries inside `poppler\Library\bin` and put them in the same folder where the executable is stored.

To compile and generate the executable, copy this line inside the MYSYS terminal.
```
g++ -std=c++17 main.cpp -o main.exe $(pkg-config --cflags --libs poppler-cpp)
```
