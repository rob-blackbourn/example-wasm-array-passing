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