# rag

A Retrieval-Augmented Generation (RAG) pipeline written from scratch in C — no third-party data-structure, JSON, or vector-store libraries. The only external dependencies are `libcurl` (HTTP calls to OpenAI) and `OpenSSL` (SHA-256 hashing) plus `libuuid` for chunk IDs. Everything else — linked lists, hash maps, a JSON parser, a file-based key/value store, text chunkers, and cosine similarity — is implemented by hand as a learning exercise in systems programming.

## Why

This project exists to learn C by building the pieces most people would normally pull in as dependencies: a hash map, a JSON parser, a simple on-disk store, and a chunking/embedding/retrieval pipeline, wired together into an end-to-end RAG system.

## How it works

1. **Ingest** — read a `.txt`/`.pdf` file, split it into chunks using a configurable strategy, compute an embedding and a SHA-256 hash for each chunk, and persist the chunks to disk.
2. **Retrieve** — embed the user's query, scan the stored chunks in parallel, and rank them by cosine similarity against a threshold.
3. **Generate** — stuff the top-`n` retrieved chunks into a prompt and call an OpenAI chat model to produce the final answer.

```
file → parser → chunker → embedder → db (ingestion)
query → embedder → similarity search over db → top-k chunks → LLM → answer (retrieval + generation)
```

## From-scratch components

| Component | Files | What it does |
|---|---|---|
| Linked list | `src/ds.c/.h` | Singly linked list (`ll_t`) used as the hash map's bucket storage. |
| Hash map | `src/ds.c/.h` | String-keyed hash map (`hash_map_t`) with chaining via linked lists; backs both the JSON object model and the DB's key index. |
| JSON parser | `src/json.c/.h` | Hand-rolled recursive-descent parser/tokenizer producing objects, lists, strings, numbers, and booleans — used to parse OpenAI API responses. |
| File parser | `src/parser.c/.h` | Loads raw file contents (`.txt`, `.pdf`) into memory for chunking. |
| Chunkers | `src/chunk.c/.h` | Splits documents into chunks. Implements **fixed-size** and **sliding-window** strategies (semantic/structural chunking are stubbed for future work); each chunk tracks its own metadata, content hash, embedding vector, and links to prev/next/parent/children chunks. |
| Vector math | `src/math.c/.h` | Cosine similarity between two 1536-dimensional embedding vectors. |
| Key/value store | `src/db.c/.h` | A minimal on-disk store (`./.db`) that persists chunks by hash, with thread-safe bulk insert/retrieve/delete via a mutex-guarded hash map index. |
| OpenAI client | `src/openai.c/.h` | Raw `libcurl` HTTP calls to the embeddings (`text-embedding-3-small`) and chat completions (`gpt-4.1-mini`) endpoints, with hand-written JSON request bodies. |

## Pipeline stages

- `src/pipeline/ingestion.c` — orchestrates parsing, chunking, embedding, and multi-threaded DB insertion.
- `src/pipeline/retrieval.c` — multi-threaded similarity search over the stored chunks against a query embedding.
- `src/pipeline/generation.c` — calls the OpenAI chat endpoint with the retrieved context to produce the final answer.
- `src/pipeline/pipeline.c` — wires retrieval and generation together into a single end-to-end run.

## Building

Requires `gcc`, `make`, and Homebrew-installed `openssl@3` and `curl` (the Makefile locates them via `brew --prefix`).

```bash
make          # build build/main
make tests    # build and run the unit tests (tests/test_*.c)
make clean    # remove build/ and .db/
```

## Running

```bash
export OPENAI_API_KEY=sk-...
make run ARGS="path/to/file.txt \"your question about the file\""
```

On first run (no `.db` directory present) the given file is ingested — parsed, chunked, embedded, and stored. The query is then embedded, matched against stored chunks by cosine similarity, and the top matches are sent to the LLM to generate an answer. Subsequent runs reuse the existing `.db` instead of re-ingesting.

## Project layout

```
src/
  ds.c/.h            linked list + hash map
  json.c/.h          JSON tokenizer/parser
  parser.c/.h        file loading
  chunk.c/.h         chunking strategies + chunk model
  math.c/.h          cosine similarity
  db.c/.h            on-disk chunk store
  openai.c/.h        OpenAI HTTP client
  pipeline/          ingestion, retrieval, generation, and top-level orchestration
tests/               unit tests for ds, db, json
resources/           sample input files for manual testing
```

## Status

This is a work-in-progress learning project, not a production system. Known rough edges (see `notes` and inline `TODO`/`FIXME` comments):
- Only fixed-size and sliding-window chunking are implemented; semantic and structural chunking are planned.
- The chunk store keeps prev/next/parent/child links as either full pointers or hashes (`chunk_ref_t`), but retrieval by hash currently requires fetching whole chunks rather than a proper on-disk index.
- No HTTP API layer yet — the pipeline is driven from the CLI (`src/main.c`).
