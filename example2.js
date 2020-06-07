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