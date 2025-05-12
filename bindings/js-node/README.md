# ailoy-js-node

Node.js binding for Ailoy runtime APIs

## Install
```bash
# npm
npm install ailoy-js-node
# yarn
yarn add ailoy-js-node
```

## Quickstart

```typescript
import ailoy from "ailoy-js-node";

// Create a new runtime
const runtime = new ailoy.Runtime();

// Call function
for await (const resp of runtime.runFunction("echo", "hello world!")) {
  console.log(resp);
  // Prints "hello world!"
}

// Define components
await rt.defineComponent("language_model", "lm0", {
  model: "meta-llama/Llama-3.2-1B-Instruct",
});
// Returns true

// Call method and iterate for responses
for await (const resp of runtime.runMethod("lm0", "infer_iterative", {
  conversation: [["user", "Hello World!"]],
})) {
  console.log(resp);
  // Prints "{ token: 'token' }" until finished
}

```

