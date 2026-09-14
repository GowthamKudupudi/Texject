# Texject
It's not just a fast C++ JSON parser!

With its intuitive C++ subscript operator, Texject goes beyond JSON to provide an extensible data language and C++ framework for structured data, making it
easy to read, access, modify, and persist data. It is thread safe!

The name Texject is coined from "text object."

Texjects are JSON-like data files with extensions such as `.txj`, `.obj.txj`, `.arr.txj`, `.set.txj`, `.oob.txj`, `.num.txj`, `.str.txj`, and more.

- `example.txj`:
```
{
	firstName: "John",
	lastName: "Doe"
}
```
- this can also be written into the file `example.obj.txj` as:
```
firstName: "John",
lastName: "Doe"
```
- supported containers:
  - `obj`: Object, order of key value pairs not preserver, enclosure: `[]`
  - `oob`: OrderedOBject, order of pairs is preserverd, enclosure: `{}`
  - `arr`: Array, simple array of Texjects, enclosure: `[]`
  - `set`: Set, duplicate members are discarded, enclosure: `{}`
- Baic types: numbers, strings, boolean, binaray, xml, time stamps.
- When serialized to JSON, maximum features are put into JSON and remaining
  that can not be JSONed are cleanly discarded.
- Its fast, lite and thread safe!

## Parsing simple json:
- simple.json:
```JSON
{"employees": [
	{"firstName": "John", "lastName": "Doe"}, 
	{"firstName": "Anna", "lastName": "Smith"},
	{"firstName": "Peter", "lastName": "Jones"}
 ],
 "employeeCount": 3,
 "arePermanent": true
}
```
- C++:
```C++
#include <Texject.h>

int main () {
	Txj_ comp("file://tests/data/simple.json");
	Txj_& employees= comp["employees"];
	Txj_& Emp1= employees[0];
	cout<< "Employee 1: "<< Emp1["firstName"]<< " "<< Emp1["lastName"]<< endl;
	return 0;
}
```
- Output:
```
Employee 1: John Doe
```
### Print back in JSON
- C++:
```C++
//1st argument should be given 'true' for JSON string else it gives Texject
string Emp2= employees[1].stringify(true);
cout<< "Employee 2: "<< Emp2<< endl;
```
- Output:
```
Employee 2: {"firstName":"Anna","lastName":"Smith"}
```
### Pretty print JSON
```C++
string Emp3= employees[2].prettyString(true);
cout<< "Employee 3: "<< Emp3<< endl;
```
- Output:
```
Employee 3: {
	"firstName": "Peter",
	"lastName": "Jones"
}
```
### Modify and save it
- C++:
```C++
// adding new employee if employeeCount is 3 and update employee count
if ((int)comp["employeeCount"]==3) {
	employees[employees.size].init(
		"{firstName: \"Gowtham\", lastName: \"Kudupdui\"}");
	comp["employeeCount"]= employees.size;
}
// save the modified `comp` texject to its file
comp.save();
```
- `./tests/data/simple.json`:
```JSON
{
	"employees": [
		{"firstName": "John", "lastName": "Doe"}, 
		{"firstName": "Anna", "lastName": "Smith"},
		{"firstName": "Peter", "lastName": "Jones"},
		{"firstName": "Gowtham", "lastName": "Kudupudi"}
	],
	"employeeCount": 4,
	"arePermanent": true
}
```

## The Texject:
- `Employee.oob.txj`; oob: OrderedOBject; the order of members preserved
```txj
name: "Gowtham",
"id": 1729,
isProgrammer: true,

#comment1: "It's a table; '|' is inheritance operator",
favLang: [
	"C++", "Javascript", "lisp"
],
testScore: [[
	    8,            7,      6
], [
	    7,            8,      3
], [
	    9,            5,      8
]] | [favLang],

#comment2: "its an oob enclosed in [] where as obj(object) is enclosed in {}"
"address": [
	"town": "KAKINADA",
	country: "Bharath"
],

"biography": "
	He is smart, brilliant, genius, empathetic, creative, connective, patient,
	handsome, valient, romantic;) After all he made JSON with multi line string!
",

#comment3: "its an obj; enclosed in {}; order not preserved"
physiology: {
	iris: "blue",
	height: "6 foot"
},

#comment2: "its a set! And comments are not stringified",
sports: {"cricket", "badminton", "tt"}
```
- C++:
```CPP
Txj_ emp("file://tests/data/Employee.oob.txj");
cout<< emp["name"]<< "'s C++ test 1 score: "<< emp["testScore"][0]["C++"]<<
endl;
int C++test2Score= emp["testScore"][1]["C++"];
emp["testScore"][1]["C++"]= ++langScore;
emp.save(); // saves to the file
emp["sports"][]= "tt"; //its already in set, so no effect! 
cout<< "Sports he play: "<< emp["sports"]<< endl;
```
- Output on 1st run:
```
Gowtham's C++ test 1 score: 7
Sports he play: {"cricket", "badminton", "tt"}
```
- Output on 2nd run:
```
Gowtham's C++ test 1 score: 8
Sports he play: {"cricket", "badminton", "tt"}
```
- Texject can be stingified to JSON using `.stringfy(true)` or 
  `.prettyString(true)` upon which it removes ` | [favlang]` and
  quotes all strings.

## Build and Install
```
git --depth=1 -b clang clone https://github.com/gowthamkudupudi/ferrybase.git
git --depth=1 clone https://github.com/gowthamkudupudi/logger.git
git --depth=1 clone https://github.com/gowthamkudupudi/Texject.git
cd Texject
mkdir build
cmake -G "Unix Makefiles" -DBUILD_TESTING=1 -B build
make -j`nproc`

# Run unit tests. If any fail, raise an issue.
./build/testUnits

# to install to /usr/local/
sudo make install
```

## Linker option
`-ltxj`


I recursively hacked it to extract as many features as I could and I will continue till it can!
