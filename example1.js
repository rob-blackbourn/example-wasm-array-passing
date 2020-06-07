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