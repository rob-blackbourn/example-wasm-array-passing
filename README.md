# How to Pass Arrays Between JavaScript and WebAssembly Modules Written in C

In this blog I'll demonstrate some ways of passing arrays between JavaScript
and WebAssembly. The source code is [here](https://github.com/rob-blackbourn/example-wasm-array-passing).

## Prerequisites

These examples have been tested with [nodejs v12.9.1](https://nodejs.org),
[clang 10.0.0](https://clang.llvm.org/),
and [wabt 1.0.16](https://github.com/WebAssembly/wabt) on Ubuntu 18.04.

I installed `clang` in `/opt/clang`, `wabt` in `/opt/wabt`, and I use
[nodenv](https://github.com/nodenv/nodenv) to manage my `nodejs` environment.

Update your path.

```bash
export PATH=/opt/clang/bin:/opt/wabt/bin:$PATH
```

## No Hello, World?

Sadly the typically hello world example is tricky for WebAssembly; so we'll do
a traditional "add two numbers" instead.

### Write the C code

Start by writing the C code in the file `example1.c`

```c
__attribute__((used)) int addTwoInts (int a, int b) {
    return a + b;
}
```

This looks pretty vanilla apart from `__attribute__((used))`. This appears to
mark the function as something that can be exported from the module.

### Build the wasm module.

Now lets compile.

```bash
clang example1.c \
  --target=wasm32-unknown-unknown-wasm \
  --optimize=3 \
  -nostdlib \
  -Wl,--export-all \
  -Wl,--no-entry \
  -Wl,--allow-undefined \
  --output example1.wasm
```

So many flags! Lets go through them.

`--target=wasm32-unknown-unknown-wasm` tells the compiler/linker to produce
wasm.

`--optimize=3` seems to ne necessary to produce valid wasm. I don't know why,
and I might be wrong.

`-nostdlib` tells the compiler/linker that we don't have a standard library,
which is very sad.

`-Wl,--export-all` tells the linker to export anything it can.

`-Wl,--no-entry` tells the linker that there's no main; this is just a library.

`-Wl,--allow-undefined` tells the linked to allow the code to access functions
and variables that have not been defined. We'll need to provide them when we
instantiate the WebAssembly instance. This won't be used in this example, but
we'll need it later.

`--output example1.wasm` does what it says on the tin.

If all went well you now have an `example.wasm` file.

### Write the JavaScript

Write a JavaScript file `example.js` with the following content.

```javascript
const fs = require('fs')

async function main() {
  // Read the wasm file.
  const buf = fs.readFileSync('./example1.wasm')

  // Create a WebAssembly instance from the wasm.
  const res = await WebAssembly.instantiate(buf, {})
  
  // Get the function to call.
  const { addTwoInts } = res.instance.exports
  
  // Call the function.
  const a = 38, b = 4
  const result = addTwoInts(a, b)
  console.log(`${a} + ${b} =  ${result}`)
}

main().then(() => console.log('Done'))
```

The code comments should make it pretty clear whats happening here. We read the
compiled wasm into a buffer, instantiate the WebAssembly, get the function, run
it, and print out the result.

The `WebAssembly.instantiate` function is a promise, and I have chosen to use
the `await` syntax to make the code more readable, so I make an `async main`
function which I call as a promise on the last line.

Lets run it.

```bash
node example1.js
```

If all went well you should see the following.

```bash
38 + 4 =  42
Done
```

### What just happened?

The bit I want to talk about here is how the wasm got instantiated. The first
argument to `WebAssembly.instantiate` was the `buf` holding the wasm. The second
was an empty `importObject`. The `importObject` can configure the properties
of the instance it creates. This might include the memory it uses, a table of
function references, imported and exported functions, etc. So why does an empty
object work?

We can look at the WebAssembly text (or `wat`) with the `wabt` tool `wasm2wat`.
To produce this we need to do the following.

```bash
wasm2wat example2.wasm -o example2.wat
```

The `example2.wat` file should look like this (edited for readability).

```lisp
(module
  (type (;0;) (func))
  (type (;1;) (func (param i32 i32) (result i32)))
  (func $__wasm_call_ctors (type 0))
  (func $addTwoInts (type 1) (param i32 i32) (result i32)
    local.get 1
    local.get 0
    i32.add)
  (table (;0;) 1 1 funcref)
  (memory (;0;) 2)
  (global (;0;) (mut i32) (i32.const 66560))
  (global (;1;) i32 (i32.const 1024))
  (export "memory" (memory 0))
  (export "addTwoInts" (func $addTwoInts))
```

Near the top you can see our `addTwoInts` function with a satisfyingly small
amount of code which is almost understandable. Then there are mentions of
`table` and `memory`. At the end wel can see `memory` exported along with our
function.

What has happened is that the clang tool-chain has generated a bunch of stuff
that we would otherwise need to put in the `importObject`. You can switch this
off (see [here](https://lld.llvm.org/WebAssembly.html)) and control everything
from the JavaScript side, but I quite like it.

## Passing arrays to wasm

Now we've established our tools work, and seen some of the features clang
provides we can get to arrays.

### Write the C code

Write a file `example2.c` with the following contents.

```c
__attribute__((used)) int sumArrayInt32 (int *array, int length) {
    int total = 0;

    for (int i = 0; i < length; ++i) {
        total += array[i];
    }

    return total;
}
```

Compile it to wasm as we did before.

### Write the JavaScript

Write a javascript file call `example2.js` with the following contents.

```javascript
const fs = require('fs')

async function main() {
  // Load the wasm into a buffer.
  const buf = fs.readFileSync('./example2.wasm')

  // Instantiate the wasm.
  const res = await WebAssembly.instantiate(buf, {})
  
  // Get the function out of the exports.
  const { sumArrayInt32, memory } = res.instance.exports
  
  // Create an array that can be passed to the WebAssembly instance.
  const array = new Int32Array(memory.buffer, 0, 5)
  array.set([3, 15, 18, 4, 2])

  // Call the function and display the results.
  const result = sumArrayInt32(array.byteOffset, array.length)
  console.log(`sum([${array.join(',')}]) = ${result}`)

  // This does the same thing!
  if (result == sumArrayInt32(0, 5)) {
    console.log(`Memory is an integer array starting at 0`)
  }
}

main().then(() => console.log('Done'))
```

The first part is the same as the previous example.

Then we create the array.

```javascript
const array = new Int32Array(memory.buffer, 0, 5)
array.set([3, 15, 18, 4, 2])
```

The wasm instance has a memory buffer which is exposed to JavaScript as
an `ArrayBuffer` in `memory.buffer`. We create our `Int32Array` using the
first available "memory location" `0`, and specify that it is `5` 
integers in length. Note the memory location is in terms of bytes, and
the length in terms of integers. The `set` method copies the JavaScript
array into the memory buffer.

Then we call the function.

```javascript
const result = sumArrayInt32(array.byteOffset, array.length)
```

We pass in the offset in bytes to the array in the memory buffer and
the length (in integers) of the array. This gets passed to the function
we wrote in C.

```c
int sumArrayInt32 (int *array, int length)
```

The result gets passed back as an integer.

## Passing output arrays to wasm

This gives us some of what we want, but what if we want to get an array
back from wasm? The first approach is to pass an output array.

### Write the C code

Write a file `example3.c` with the following contents.

```c
__attribute__((used)) void addArraysInt32 (int *array1, int* array2, int* result, int length)
{
    for (int i = 0; i < length; ++i) {
        result[i] = array1[i] + array2[i];
    }
}
```

The function takes in three arrays, multiplying each element of the two
input arrays and storing the output in the result array. Nothing need be
returned.

### Write the JavaScript

Write a file called `example3.js` with the following content.

```javascript
const fs = require('fs')

async function main() {
  // Load the wasm into a buffer.
  const buf = fs.readFileSync('./example3.wasm')

  // Make an instance.
  const res = await WebAssembly.instantiate(buf, {})
  
  // Get function.
  const { addArraysInt32, memory } = res.instance.exports
  
  // Create the arrays.
  const length = 5

  let offset = 0
  const array1 = new Int32Array(memory.buffer, offset, length)
  array1.set([1, 2, 3, 4, 5])

  offset += length * Int32Array.BYTES_PER_ELEMENT
  const array2 = new Int32Array(memory.buffer, offset, length)
  array2.set([6, 7, 8, 9, 10])

  offset += length * Int32Array.BYTES_PER_ELEMENT
  const result = new Int32Array(memory.buffer, offset, length)

  // Call the function.
  addArraysInt32(
    array1.byteOffset,
    array2.byteOffset,
    result.byteOffset,
    length)
  
  // Show the results.
  console.log(`[${array1.join(", ")}] + [${array2.join(", ")}] = [${result.join(", ")}]`)
}

main().then(() => console.log('Done'))
```

We can see some differences here in the way we create the arrays. The
code to create the first array looks the same, but what's going on for
the others?

```javascript
offset += length * Int32Array.BYTES_PER_ELEMENT
const array2 = new Int32Array(memory.buffer, offset, length)
```

Our first example was easy, as we only had one array. When we create
subsequent arrays we need to lay them out in memory such that they don't
overlap each other. To do this we calculate the number of bytes required
with `length * Int32Array.BYTES_PER_ELEMENT` and add it to the previous
offset to ensure each array has it's own space. Note again how the offset
is in bytes, but the length is in units of integers.

Next we call the function.

```javascript
addArraysInt32(array1.byteOffset, array2.byteOffset, result.byteOffset, length)
```

This maps on to the prototype of our C implementation.

```c
void addArraysInt32 (int *array1, int* array2, int* result, int length)
```

Now we can get array data out of wasm!

But wait, something is missing. What if we want to create an array
inside the wasm module and pass it out? What if our array calculation
needs to create it's own arrays in order to support the calculation?

To do that we need to provided some memory management.

## Understanding wasm memory management

As we have seen, wasm memory is managed in JavaScript through the
[`WebAssembly.Memory`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/WebAssembly/Memory)
object. This has a `buffer` which is an [`ArrayBuffer`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer). The buffer has a `byteLength` property, which gives us the
upper bound of the memory.

It also has a `grow` method which can be used to get more memory.

I couldn't find any documentation on how to get to this information from
inside the wasm instance, but we can solve this problem by calling back
to the JavaScript from wasm.

## Calling JavaScript from wasm

We can call JavaScript from wasm by *importing* a function when we 
instantiate the wasm module.

### Write the C code

Write a file `example4.c` with the following contents.

```c
extern void someFunction(int i);

__attribute__((used)) void callSomeFunction (int i)
{
  someFunction(i);
}
```

Here declare an *external* function `someFunction` that will be
provided by JavaScript, and then export `callSomeFunction` that, when
invoked, will run the imported function.

### Write the JavaScript

Write a file called `example4.js` with the following content.

```javascript
const fs = require('fs')

async function main() {
  // Load the wasm into a buffer.
  const buf = fs.readFileSync('./example4.wasm')

  // Make the instance, importing a function called someFunction which
  // logs its arguments to the console.
  const res = await WebAssembly.instantiate(buf, {
    env: {
      someFunction: function (i) {
        console.log(`someFunction called with ${i}`)
      }
    }
  })
  
  // Get the exported function callSomeFunction from the wasm instance.
  const { callSomeFunction } = res.instance.exports

  // Calling the function should call back into the function we imported.
  callSomeFunction(42)
}

main().then(() => console.log('Done'))
```

Rather than passing in an empty object to `WebAssembly.instantiate` as we
did previously, we now pass an object with an `env` property. This
contains a property `someFunction` with it's implementation (it just
logs its argument to the console).

Calling the function `callSomeFunction` calls back to the function we
provided to the wasm module `someFunction`.

Now we have a way of the wasm module finding out how much memory it has 
and asking for more.

## Passing arrays from wasm to JavaScript

First we need some C code to perform memory management. I wrote a trivial
implementation of `malloc` which can be found [here](https://github.com/rob-blackbourn/example-wasm-array-passing/blob/master/memory-allocation.c).
Copy this file to the work folder as `memory-allocation.c` The code provides
three functions.

```c
void *allocateMemory (unsigned bytes_required);
void freeMemory(void *memory_to_free);
double reportFreeMemory()
```

Obviously it's stupid to write your own `malloc`. Rob bad.

### Write the C code

Write a file `example5.c` with the following contents.

```c
extern void *allocateMemory(unsigned bytes_required);

__attribute__((used)) int* addArrays (int *array1, int* array2, int length)
{
  int* result = allocateMemory(length * sizeof(int));

  for (int i = 0; i < length; ++i) {
    result[i] = array1[i] + array2[i];
  }

  return result;
}
```

The code is similar to the previous example, but instead of taking in a result
array, it creates and returns the array itself.

### Write the JavaScript

Write a file called `example5.js` containing the following code.

```javascript
const fs = require('fs')

/*
 * A simple memory manager.
 */
class MemoryManager {
  // The WebAssembly.Memory object for the instance.
  memory = null

  // Return the buffer length in bytes.
  memoryBytesLength() {
    return this.memory.buffer.byteLength
  }

  // Grow the memory by the requested blocks, returning the new buffer length
  // in bytes.
  grow(blocks) {
    this.memory.grow(blocks)
    return this.memory.buffer.byteLength
  }
}

async function main() {
  // Read the wasm file.
  const buf = fs.readFileSync('./example5.wasm')

  // Create an object to manage the memory.
  const memoryManager = new MemoryManager()

  // Instantiate the wasm module.
  const res = await WebAssembly.instantiate(buf, {
    env: {
      // The wasm module calls this function to grow the memory
      grow: function(blocks) {
        memoryManager.grow(blocks)
      },
      // The wasm module calls this function to get the current memory size.
      memoryBytesLength: function() {
        memoryManager.memoryBytesLength()
      }
    }
  })

  // Get the memory exports from the wasm instance.
  const {
    memory,
    allocateMemory,
    freeMemory,
    reportFreeMemory
  } = res.instance.exports

  // Give the memory manager access to the instances memory.
  memoryManager.memory = memory

  // How many free bytes are there?
  const startFreeMemoryBytes = reportFreeMemory()
  console.log(`There are ${startFreeMemoryBytes} bytes of free memory`)

  // Get the exported array function.
  const {
    addArrays
  } = res.instance.exports

  // Make the arrays to pass into the wasm function using allocateMemory.
  const length = 5
  const bytesLength = length * Int32Array.BYTES_PER_ELEMENT

  const array1 = new Int32Array(memory.buffer, allocateMemory(bytesLength), length)
  array1.set([1, 2, 3, 4, 5])

  const array2 = new Int32Array(memory.buffer, allocateMemory(bytesLength), length)
  array2.set([6, 7, 8, 9, 10])

  // Add the arrays. The result is the memory pointer to the result array.
  result = new Int32Array(
    memory.buffer,
    addArrays(array1.byteOffset, array2.byteOffset, length),
    length)

  console.log(`[${array1.join(", ")}] + [${array2.join(", ")}] = [${result.join(", ")}]`)

  // Show that some memory has been used.
  pctFree = 100 * reportFreeMemory() / startFreeMemoryBytes
  console.log(`Free memory ${pctFree}%`)

  // Free the memory.
  freeMemory(array1.byteOffset)
  freeMemory(array2.byteOffset)
  freeMemory(result.byteOffset)

  // Show that all the memory has been released.
  pctFree = 100 * reportFreeMemory() / startFreeMemoryBytes
  console.log(`Free memory ${pctFree}%`)
}

main().then(() => console.log('Done'))
```

That's a lot of code!

Let's get started with the memory management. The script starts declaring a
`MemoryManagement` class.

```javascript
class MemoryManager {
  // The WebAssembly.Memory object for the instance.
  memory = null

  // Return the buffer length in bytes.
  memoryBytesLength() {
    return this.memory.buffer.byteLength
  }

  // Grow the memory by the requested blocks, returning the new buffer length
  // in bytes.
  grow(blocks) {
    this.memory.grow(blocks)
    return this.memory.buffer.byteLength
  }
}
```

Once the `memory` property has been set it can provide the length in bytes of
the memory, and it can grow the memory for a given number of blocks returning
the new memory length in bytes.

We pass the functions when creating the wasm instance, and assign the memory
object once to instance is created.

```javascript
  const memoryManager = new MemoryManager()

  const res = await WebAssembly.instantiate(buf, {
    env: {
      grow: function(blocks) {
        memoryManager.grow(blocks)
      },
      memoryBytesLength: function() {
        memoryManager.memoryBytesLength()
      }
    }
  })

  memoryManager.memory = res.instance.exports.memory
```

When we create the arrays we use the memory allocator.

```javascript
const array1 = new Int32Array(memory.buffer, allocateMemory(bytesLength), length)
```

The memory gets freed with the complimentary function near the end of the
script.

```javascript
freeMemory(array1.byteOffset)
```

Finally we can call our function and get the results.

```javascript
result = new Int32Array(
  memory.buffer,
  addArrays(array1.byteOffset, array2.byteOffset, length),
  length)
```

Note that what gets returned is the point in memory where the array has been
stored, so we need to wrap it in an `Int32Array` object. And don't forget to
free it!

## Outro

Well that was a lot of code.

Obviously we can wrap this all up in some library code to hide the complexity,
but what have we gained. I can't say for sure, but if the array calculations
run at near native speed this could provide enormous performance gains.

I could (and probably should) have used a `libc` implementation instead of
implementing `malloc`. I decided not to because I think it kept the focus on
the array passing problem, and not on integrating with libraries, which is a
whole other discussion.

Good luck with your coding.