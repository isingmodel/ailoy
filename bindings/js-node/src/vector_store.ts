import type { NDArray } from "ailoy_addon.node";

import { Runtime, generateUUID } from "./runtime";

interface BaseVectorStoreArgs {
  type: string;
  embedding: "bge-m3";
}

interface FAISSArgs extends BaseVectorStoreArgs {
  type: "faiss";
}

interface ChromadbArgs extends BaseVectorStoreArgs {
  type: "chromadb";
  url: string;
  collection?: string;
}

export interface VectorStoreInsertItem {
  document: string;
  metadata?: Record<string, any>;
}

export interface VectorStoreRetrieveItem extends VectorStoreInsertItem {
  id: string;
  similarity: number;
}

export class VectorStore {
  private runtime: Runtime;
  private args: FAISSArgs | ChromadbArgs;
  private vectorstoreComponentName: string;
  private embeddingModelComponentName: string;
  private initialized: boolean;

  constructor(runtime: Runtime, args: FAISSArgs | ChromadbArgs) {
    this.runtime = runtime;
    this.args = args;
    this.vectorstoreComponentName = generateUUID();
    this.embeddingModelComponentName = generateUUID();
    this.initialized = false;
  }

  async initialize(): Promise<void> {
    if (this.initialized) return;

    // Dimension size may be different based on embedding model
    let dimension: number = 1024;

    // Initialize embedding model
    if (this.args.embedding === "bge-m3") {
      dimension = 1024;
      await this.runtime.define(
        "tvm_embedding_model",
        this.embeddingModelComponentName,
        {
          model: "BAAI/bge-m3",
        }
      );
    }

    // Initialize vector store
    if (this.args.type === "faiss") {
      await this.runtime.define(
        "faiss_vector_store",
        this.vectorstoreComponentName,
        {
          dimension,
        }
      );
    } else if (this.args.type === "chromadb") {
      await this.runtime.define(
        "chromadb_vector_store",
        this.vectorstoreComponentName,
        {
          url: this.args.url,
          collection: this.args.collection,
        }
      );
    }

    this.initialized = true;
  }

  async deinitialize(): Promise<void> {
    if (!this.initialized) return;
    await this.runtime.delete(this.embeddingModelComponentName);
    await this.runtime.delete(this.vectorstoreComponentName);
    this.initialized = false;
  }

  async insert(item: VectorStoreInsertItem): Promise<void> {
    const embedding = await this.embedding(item.document);
    await this.runtime.callMethod(this.vectorstoreComponentName, "insert", {
      embedding: embedding,
      document: item.document,
      metadata: item.metadata,
    });
  }

  async retrieve(
    query: string,
    topK: number = 5
  ): Promise<Array<VectorStoreRetrieveItem>> {
    const embedding = await this.embedding(query);
    const resp: { results: Array<VectorStoreRetrieveItem> } =
      await this.runtime.callMethod(this.vectorstoreComponentName, "retrieve", {
        query_embedding: embedding,
        k: topK,
      });
    return resp.results;
  }

  async clear(): Promise<void> {
    await this.runtime.callMethod(this.vectorstoreComponentName, "clear");
  }

  async embedding(text: string): Promise<NDArray> {
    const resp: { embedding: NDArray } = await this.runtime.callMethod(
      this.embeddingModelComponentName,
      "infer",
      {
        prompt: text,
      }
    );
    return resp.embedding;
  }
}
