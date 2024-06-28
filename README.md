# TunableParser
Since 1.69, R* changed the way that they register tunables in tunables_processing.

Their new code looks like this: ``Global_262145 = unk_0x367E5E33E7F0DD1A(joaat("CASH_MULTIPLIER"), 1f);``

As you can see, they simply return the value of the tunable instead of passing a pointer to it as an argument, which sadly makes it no longer possible for us to get the tunables with their hashes by hooking a few natives.

Since I still wanted to work with tunables by using their hash instead of their global index, I created this simple tool. Keep in mind that this was made really quickly, so I didn't account for error handling and stuff like that.

It parses the decompiled tunables_processing file and creates a map with the tunable hashes as a key and their global indices as a value. This isn't pretty but I guess it's the easiest way to keep using the hashes. The values get stored in an output.txt file.

## How to use
1) Decompile the tunables_processing file
2) Drag in on the TunableParser.exe file or run a console command: ``TunableParser.exe "path/to/tunables_processing.c"``
3) Store the content of the output.txt file in a map or something similar
4) Now you can keep using hashes to access tunables

## Code Example
```cpp
std::unordered_map<uint32_t, uint64_t> tunables{
  //Data from output.txt
};

void* get_global(uint64_t index) {
  //Returns a script global ptr
}

template <typename T>
T* get_tunable(uint32_t hash) {
  return (T*)get_global(tunables[hash]);
}

*get_tunable<BOOL>(CONSTEXPR_JOAAT("TURN_SNOW_ON_OFF")) = TRUE;
```
