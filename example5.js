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