const fs = require('fs')

async function main() {
  const buf = fs.readFileSync('./example4.wasm')

  const res = await WebAssembly.instantiate(buf, {
    env: {
      someFunction: function (i) {
        console.log(`someFunction called with ${i}`)
      }
    }
  })
  
  const { callSomeFunction } = res.instance.exports

  callSomeFunction(42)
}


main().then(() => console.log('Done'))