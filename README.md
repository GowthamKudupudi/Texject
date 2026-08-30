# Texject
Fast C++ JSON parser; serves more than the JSON with subscript operator in
C++. Texject mean text object. These
are stored in files with extension `.txj`, `.obj.txj`, `.arr.txj`, `.set.txj`,
`.num.txj`, `.str.txj` etc.

I doubt a parser could be any faster!

Its a valgrind clean library. No need to worry about freeing up parser.

I recursively hacked it to extract as many features as I could and I will continue till it can!

## code at a glance:
### Employee.txj
```JSON
{
   name: "Gowtham",
   "id": 1729,
   isProgrammer: true,
   favLanguages: {"C++", "Javascript", "lisp"},
   langScores:   [9,      9,         , 8] | {favLanguages},
   "address": {
      "town": "KAKINADA",
      country: "Bharath"
   }
}
```

### main.cpp
```CPP
#include <base/FFJSON.h>

Txj obj("file://Employee.txj");

cout << obj["name"] << endl;
cout << (int)obj["id"] << endl;
cout << obj["address"]["country"] << endl;
if (obj["isProgrammer"]) {
   # append new element to array
   obj["favLanguages"][obj["favLanguages"].size]="lisp";
   cout << obj["favLanguages"] << endl;
   cout << "C++ score: " << obj["langScores"]["C++"] << endl;
}
```

### output:
```
Gowtham
1729
Bhaarath
{"C++", "javascript", "lisp"}
C++ score: 9
```

## Installation
```
git clone https://github.com/necktwi/Texject.git
cd Texject
mkdir build/Linux/x86_64/debug
cmake -G "Unix Makefiles" -D_DEBUG=1 -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DBUILD_TESTING=1 ../../../../
make -j`nproc`
sudo make install
```

## Linker option
`-ltxj`

- It got lot many cool features; look https://github.com/necktwi/Texject/blob/master/nbproject/tests/ffjsonTest.cpp
- FFJSON acronym is "FerryFairJSON"
