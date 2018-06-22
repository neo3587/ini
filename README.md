# INI
A simple INI file parser for C++

# Notes
You need to download the oi_map library from the sumbodule folder

# Usage Example
```c++
#include "ini.hpp"
#include <iostream>
#include <string>



int main(int argc, char* argv[]) {

  neo::ini<> ini; // neo::ini<> == sections and keys, neo::ini<false> == only keys
  ini.parse_file("test.ini"); // istreams are also allowed
  
  // ini works like a map of maps that keeps the insertion order, 
  // you can use arithmetic types (short, int, float, double, ...)
  // and strings to write/read the values of each key
  int val = ini["section 0"]["key 0"];
  std::string str = ini["section 0]["key 1"];
  ini["section 0"]["key 3"] = 123.45;
  ini["section 0"]["key 4"] = "StringValue";
    
  std::cout << ini.to_string() << std::endl;
  
  ini.to_file("test_out.ini"); // ostreams are also allowed

  return 0;
}


```
