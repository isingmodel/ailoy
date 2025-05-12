import { BrowserWindow, ipcMain, dialog } from "electron";
import * as fs from "fs/promises";
import * as ailoy from "ailoy-js-node";

let runtime: ailoy.Runtime | undefined = undefined;
let executor: ailoy.ReflectiveExecutor | undefined = undefined;
let vectorstore: ailoy.VectorStore | undefined = undefined;

export const initializeComponents = async (mainWindow: BrowserWindow) => {
  if (runtime === undefined) {
    runtime = new ailoy.Runtime();
    await runtime.start();
  }

  if (vectorstore === undefined) {
    vectorstore = new ailoy.VectorStore(runtime, {
      type: "faiss",
      embedding: "bge-m3",
    });
    await vectorstore.initialize();
  }

  if (executor === undefined) {
    mainWindow.webContents.send(
      "indicate-loading",
      "Loading AI model...",
      false
    );
    executor = new ailoy.ReflectiveExecutor(runtime, {
      model: { name: "qwen3-8b" },
    });
    await executor.initialize();
    mainWindow.webContents.send("indicate-loading", "", true);
  }
};

export const registerIpcHandlers = async (mainWindow: BrowserWindow) => {
  ipcMain.handle("open-file", async () => {
    const result = await dialog.showOpenDialog({
      properties: ["openFile"],
      filters: [{ name: "Text Files", extensions: ["txt"] }],
    });

    if (!result.canceled && result.filePaths.length > 0) {
      mainWindow.webContents.send("file-selected");

      const content = await fs.readFile(result.filePaths[0], "utf-8");

      await vectorstore.clear();

      // split texts into chunks
      const { chunks }: { chunks: Array<string> } = await runtime.call(
        "split_text",
        {
          text: content,
          chunk_size: 500,
          chunk_overlap: 200,
        }
      );

      let chunkIdx = 0;
      for (const chunk of chunks) {
        await vectorstore.insert({
          document: chunk,
          metadata: null,
        });
        mainWindow.webContents.send(
          "text-load-progress",
          chunkIdx + 1,
          chunks.length
        );
        chunkIdx += 1;
      }

      return content;
    }
    return null;
  });

  ipcMain.handle("retrieve-similar-documents", async (event, query: string) => {
    const results = await vectorstore.retrieve(query, 5);
    return results;
  });

  ipcMain.handle("infer-language-model", async (event, message: string) => {
    for await (const resp of executor.run(message)) {
      mainWindow.webContents.send("assistant-answer", resp);
    }
  });
};

export const removeIpcHandlers = () => {
  ipcMain.removeHandler("open-file");
  ipcMain.removeHandler("submit-query");
};

export const destroyComponents = async () => {
  if (vectorstore !== undefined) {
    await vectorstore.deinitialize();
    vectorstore = undefined;
  }
  if (executor !== undefined) {
    await executor.deinitialize();
    executor = undefined;
  }
  if (runtime !== undefined) {
    await runtime.stop();
    runtime = undefined;
  }
};
