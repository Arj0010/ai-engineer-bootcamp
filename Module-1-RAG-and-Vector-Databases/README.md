# Module 1: RAG and Vector Databases

**Duration:** 2-2.5 hours
**Focus:** Production RAG systems, embeddings, vector databases

## Learning Objectives
- Understand RAG fundamentals and when to use it
- Master embeddings and semantic search
- Compare vector database options (Chroma, FAISS, Pinecone)
- Implement chunking strategies for production
- Build a working RAG system

---

## 1. RAG Fundamentals (30 mins)

### What is RAG?

**Retrieval-Augmented Generation (RAG)** combines:
- **Retrieval**: Finding relevant information from a knowledge base
- **Generation**: Using LLMs to create responses based on retrieved context

```
User Query → Embed Query → Search Vector DB → Retrieve Top-K Docs →
LLM (Query + Context) → Generated Answer
```

### Why RAG?

**Problem RAG Solves:**
- LLMs have knowledge cutoff dates
- LLMs hallucinate when lacking information
- Fine-tuning is expensive and slow to update

**RAG Benefits:**
- Dynamic knowledge updates (just update vector DB)
- Source attribution (know where answers come from)
- Cost-effective vs fine-tuning
- Works with proprietary/private data

### When to Use RAG?

| Use RAG When | Don't Use RAG When |
|-------------|-------------------|
| You have a knowledge base | You need reasoning on general knowledge |
| Data changes frequently | Task requires style/tone changes (use fine-tuning) |
| Need source citations | Simple Q&A without context |
| Domain-specific info needed | Task is classification/extraction only |

**Connection to LAIPath:** Your workflow orchestration in LAIPath likely handles similar patterns - retrieving context and passing it to LLM APIs. RAG is the formalized pattern for this.

---

## 2. Embeddings and Semantic Search (40 mins)

### What are Embeddings?

Embeddings convert text into dense numerical vectors that capture semantic meaning.

```python
# Example: Similar concepts have similar vectors
"king" → [0.2, 0.8, 0.1, ...]
"queen" → [0.21, 0.79, 0.12, ...]  # Similar to king
"car" → [-0.5, 0.1, 0.9, ...]      # Different from king
```

**Key Concept:** Vector distance = semantic similarity

### Embedding Models

| Model | Dimensions | Best For | Speed |
|-------|-----------|----------|-------|
| `sentence-transformers/all-MiniLM-L6-v2` | 384 | Fast, general | Fast |
| `sentence-transformers/all-mpnet-base-v2` | 768 | Quality, general | Medium |
| `text-embedding-ada-002` (OpenAI) | 1536 | High quality | API call |
| `text-embedding-3-small` (OpenAI) | 1536 | Better quality | API call |

**For Production (like LAIPath):**
- Use OpenAI embeddings for best quality
- Use sentence-transformers for cost savings and privacy
- Consider latency: local models are faster but API models are better

### Semantic Search Process

```python
# 1. Create embeddings for documents (one-time)
docs = ["Python is a programming language", "Dogs are animals"]
doc_embeddings = embed_model.encode(docs)

# 2. Embed user query
query = "What is Python?"
query_embedding = embed_model.encode(query)

# 3. Find similar documents using cosine similarity
from sklearn.metrics.pairwise import cosine_similarity
similarities = cosine_similarity([query_embedding], doc_embeddings)
# Returns: [[0.85, 0.12]] - Python doc is most similar
```

**Similarity Metrics:**
- **Cosine Similarity**: Most common, measures angle between vectors (0-1)
- **Euclidean Distance**: Measures straight-line distance
- **Dot Product**: Fast, works if vectors are normalized

---

## 3. Vector Databases (45 mins)

### Why Vector Databases?

Regular databases (SQL, MongoDB) can't efficiently search by vector similarity.

**Vector DBs optimize for:**
- Fast similarity search (ANN - Approximate Nearest Neighbors)
- Storing millions/billions of vectors
- Filtering by metadata while searching

### Vector Database Comparison

#### **FAISS** (Facebook AI Similarity Search)

```python
import faiss
import numpy as np

# Create index
dimension = 384
index = faiss.IndexFlatL2(dimension)  # L2 distance

# Add vectors
vectors = np.random.random((1000, dimension)).astype('float32')
index.add(vectors)

# Search
query_vector = np.random.random((1, dimension)).astype('float32')
distances, indices = index.search(query_vector, k=5)
```

**Pros:**
- Extremely fast (C++ implementation)
- Works offline, no external service
- Free and open source

**Cons:**
- In-memory only (requires large RAM)
- No built-in persistence (must save/load manually)
- No metadata filtering

**Use Case:** Fast prototyping, small datasets (<1M vectors)

---

#### **Chroma** (ChromaDB)

```python
import chromadb
from chromadb.config import Settings

# Initialize
client = chromadb.Client(Settings(
    persist_directory="./chroma_db"
))

# Create collection
collection = client.create_collection(name="my_docs")

# Add documents (Chroma handles embedding)
collection.add(
    documents=["Python is great", "AI is awesome"],
    metadatas=[{"source": "blog"}, {"source": "paper"}],
    ids=["doc1", "doc2"]
)

# Query
results = collection.query(
    query_texts=["programming languages"],
    n_results=2,
    where={"source": "blog"}  # Metadata filtering!
)
```

**Pros:**
- Simple API, beginner-friendly
- Built-in persistence
- Metadata filtering
- Can use custom or built-in embeddings

**Cons:**
- Slower than FAISS for very large datasets
- Less mature than Pinecone

**Use Case:** MVPs, prototypes, moderate scale (<10M vectors)

---

#### **Pinecone** (Managed Service)

```python
import pinecone

# Initialize
pinecone.init(api_key="your-key", environment="us-west1-gcp")
index = pinecone.Index("my-index")

# Upsert vectors
index.upsert(vectors=[
    ("id1", [0.1, 0.2, ...], {"category": "tech"}),
    ("id2", [0.5, 0.3, ...], {"category": "sports"})
])

# Query
results = index.query(
    vector=[0.1, 0.2, ...],
    top_k=5,
    filter={"category": "tech"},
    include_metadata=True
)
```

**Pros:**
- Production-ready, managed service
- Scales to billions of vectors
- Low latency, high availability
- Advanced features (hybrid search, namespaces)

**Cons:**
- Costs money (pay per vector/query)
- Requires internet connection
- Vendor lock-in

**Use Case:** Production systems at scale (like Wednesday Solutions projects)

---

### Comparison Table

| Feature | FAISS | Chroma | Pinecone |
|---------|-------|--------|----------|
| **Setup** | Easy | Easy | Medium |
| **Persistence** | Manual | Built-in | Managed |
| **Metadata Filter** | No | Yes | Yes |
| **Scale** | 1M vectors | 10M vectors | Billions |
| **Cost** | Free | Free | Paid |
| **Latency** | Fastest | Fast | Fast |
| **Production Ready** | No | Medium | Yes |

**For LAIPath-style Production:**
- Start with Chroma for MVP
- Migrate to Pinecone when scaling
- Use FAISS for offline/edge deployments

---

## 4. Document Chunking Strategies (30 mins)

### Why Chunking?

**Problem:** LLMs have context limits (4K, 8K, 16K tokens)
**Solution:** Split documents into chunks that fit in context

### Chunking Strategies

#### **1. Fixed-Size Chunking**

```python
def fixed_size_chunk(text, chunk_size=500, overlap=50):
    chunks = []
    start = 0
    while start < len(text):
        end = start + chunk_size
        chunks.append(text[start:end])
        start = end - overlap  # Overlap prevents context loss
    return chunks

text = "Long document..." * 1000
chunks = fixed_size_chunk(text, chunk_size=500, overlap=50)
```

**Pros:** Simple, predictable sizes
**Cons:** Can split sentences/paragraphs awkwardly

---

#### **2. Semantic Chunking** (RECOMMENDED)

```python
from langchain.text_splitter import RecursiveCharacterTextSplitter

splitter = RecursiveCharacterTextSplitter(
    chunk_size=1000,
    chunk_overlap=200,
    separators=["\n\n", "\n", ". ", " ", ""]  # Try paragraph, then sentence
)

chunks = splitter.split_text(long_document)
```

**Pros:** Respects document structure
**Cons:** Variable chunk sizes

---

#### **3. Document-Aware Chunking**

For structured documents (PDFs, HTML):

```python
def chunk_by_section(markdown_text):
    sections = markdown_text.split("\n## ")  # Split by H2 headers
    chunks = []
    for section in sections:
        if len(section) > 2000:
            # Sub-chunk large sections
            sub_chunks = semantic_chunk(section, size=1000)
            chunks.extend(sub_chunks)
        else:
            chunks.append(section)
    return chunks
```

**Use Case:** Documentation, manuals, wikis

---

### Chunking Best Practices

1. **Chunk Size Guidelines:**
   - Small (200-500 tokens): Precise retrieval, may lack context
   - Medium (500-1000 tokens): Balanced (RECOMMENDED)
   - Large (1000-2000 tokens): More context, may be less precise

2. **Always Use Overlap:**
   - 10-20% overlap prevents information loss at boundaries
   - Example: 1000 token chunks with 200 token overlap

3. **Add Metadata:**
   ```python
   chunks = [
       {
           "text": chunk_text,
           "metadata": {
               "source": "doc.pdf",
               "page": 5,
               "section": "Introduction",
               "chunk_id": "doc_chunk_1"
           }
       }
   ]
   ```

4. **Test Retrieval Quality:**
   - Try different chunk sizes
   - Check if retrieved chunks answer questions
   - Monitor chunk size distribution

---

## 5. Building a Production RAG System (30 mins)

### RAG Architecture

```
┌─────────────┐
│  Documents  │
└──────┬──────┘
       │ 1. Ingest
       ▼
┌──────────────┐
│   Chunking   │
└──────┬───────┘
       │ 2. Chunk
       ▼
┌──────────────┐
│  Embeddings  │
└──────┬───────┘
       │ 3. Embed
       ▼
┌──────────────┐
│  Vector DB   │
└──────┬───────┘
       │ 4. Store
       │
       │ [Query Time]
       │
       │ 5. Query → Embed Query
       │ 6. Search Vector DB
       ▼
┌──────────────┐
│  Retrieval   │ → Top K chunks
└──────┬───────┘
       │ 7. Format Context
       ▼
┌──────────────┐
│   LLM API    │
└──────┬───────┘
       │ 8. Generate
       ▼
┌──────────────┐
│   Response   │
└──────────────┘
```

### Key Components

#### **1. Document Loader**
```python
from langchain.document_loaders import (
    TextLoader, PyPDFLoader, UnstructuredMarkdownLoader
)

# Load various formats
pdf_loader = PyPDFLoader("document.pdf")
docs = pdf_loader.load()
```

#### **2. Chunking**
```python
from langchain.text_splitter import RecursiveCharacterTextSplitter

splitter = RecursiveCharacterTextSplitter(
    chunk_size=1000,
    chunk_overlap=200
)
chunks = splitter.split_documents(docs)
```

#### **3. Embedding + Vector Store**
```python
from langchain.embeddings import HuggingFaceEmbeddings
from langchain.vectorstores import Chroma

embeddings = HuggingFaceEmbeddings(
    model_name="sentence-transformers/all-mpnet-base-v2"
)

vectorstore = Chroma.from_documents(
    documents=chunks,
    embedding=embeddings,
    persist_directory="./chroma_db"
)
```

#### **4. Retrieval**
```python
# Simple retrieval
retriever = vectorstore.as_retriever(
    search_type="similarity",
    search_kwargs={"k": 4}  # Top 4 chunks
)

relevant_docs = retriever.get_relevant_documents("What is Python?")
```

#### **5. LLM Integration**
```python
from openai import OpenAI

client = OpenAI(api_key="your-key")

# Format context
context = "\n\n".join([doc.page_content for doc in relevant_docs])

# Generate answer
response = client.chat.completions.create(
    model="gpt-4",
    messages=[
        {"role": "system", "content": "Answer based on the context."},
        {"role": "user", "content": f"Context:\n{context}\n\nQuestion: {query}"}
    ]
)

answer = response.choices[0].message.content
```

---

## 6. Production RAG Considerations

### Performance Optimization

**1. Caching:**
```python
from functools import lru_cache

@lru_cache(maxsize=1000)
def get_embedding(text: str):
    return embedding_model.encode(text)
```

**2. Batch Processing:**
```python
# Bad: Embed one at a time
for doc in docs:
    embedding = model.encode(doc)

# Good: Batch embed
embeddings = model.encode(docs, batch_size=32)
```

**3. Async for Multiple Retrievals:**
```python
import asyncio

async def async_retrieve(query):
    return await vectorstore.asimilarity_search(query)

# Retrieve from multiple sources in parallel
results = await asyncio.gather(
    async_retrieve(query, source="docs"),
    async_retrieve(query, source="wiki")
)
```

### Quality Improvements

**1. Reranking:**
```python
from sentence_transformers import CrossEncoder

reranker = CrossEncoder('cross-encoder/ms-marco-MiniLM-L-6-v2')

# Get top 20 from vector search
initial_results = vectorstore.similarity_search(query, k=20)

# Rerank to get best 4
scores = reranker.predict([(query, doc.page_content) for doc in initial_results])
top_4 = sorted(zip(scores, initial_results), reverse=True)[:4]
```

**2. Hybrid Search (Vector + Keyword):**
```python
# Combine dense (vector) and sparse (BM25) search
from rank_bm25 import BM25Okapi

# BM25 for keyword matching
tokenized_docs = [doc.split() for doc in documents]
bm25 = BM25Okapi(tokenized_docs)
keyword_scores = bm25.get_scores(query.split())

# Vector search
vector_scores = vectorstore.similarity_search_with_score(query)

# Combine scores (weighted)
final_scores = 0.7 * vector_scores + 0.3 * keyword_scores
```

**3. Prompt Engineering:**
```python
system_prompt = """You are a helpful assistant. Answer questions based ONLY on the provided context.

Rules:
1. If the context doesn't contain the answer, say "I don't have enough information."
2. Cite the source after each claim [Source: document_name].
3. Be concise but complete.

Context:
{context}

Question: {question}
"""
```

---

## 7. Connection to Your LAIPath Experience

### How LAIPath Maps to RAG

Your LAIPath system likely has:
- **Workflow Orchestration**: Similar to RAG pipeline (retrieve → process → generate)
- **LLM API Integration**: Same as RAG's generation step
- **Adaptive Logic**: Could incorporate retrieval-based decision making

**Interview Story:**
> "In LAIPath, I built a production AI system with workflow orchestration. While I implemented custom logic for context management, I now understand this maps directly to the RAG pattern. The key insight is separating retrieval (finding relevant information) from generation (LLM output), which allows for dynamic knowledge updates without retraining models. If I were to rebuild LAIPath today, I'd leverage RAG patterns with vector databases for more scalable context management."

### How to Discuss RAG in Interview

**If they ask: "Have you built RAG systems?"**

Option 1 (Honest):
> "I haven't built a formal RAG system yet, but I've implemented the core concepts in LAIPath - retrieving context and passing it to LLM APIs. I understand the RAG architecture deeply: chunking documents, creating embeddings, storing in vector databases, and retrieving relevant context for LLM generation. I've built a hands-on RAG system as part of my interview prep to solidify this knowledge."

Option 2 (If you complete the hands-on project):
> "Yes, I've built RAG systems. Most recently, I created an 'AskMyProjects' system that uses Chroma for vector storage and retrieves relevant information from my project documentation. The architecture uses semantic chunking, sentence-transformer embeddings, and OpenAI's GPT-4 for generation. I've also implemented the core RAG patterns in production through LAIPath, where I orchestrate LLM workflows with context retrieval."

---

## 8. Interview Questions on RAG

### Basic Questions

**Q1: What is RAG and why is it useful?**
<details>
<summary>Answer</summary>

RAG (Retrieval-Augmented Generation) combines retrieval from a knowledge base with LLM generation. It's useful because:
1. **Dynamic Knowledge**: Update information without retraining
2. **Reduces Hallucination**: Grounds LLM in factual context
3. **Source Attribution**: Know where answers come from
4. **Cost-Effective**: Cheaper than fine-tuning for knowledge updates

It's essentially a pattern where you retrieve relevant documents using semantic search, then pass them as context to an LLM to generate an answer.
</details>

---

**Q2: Explain the RAG pipeline step-by-step.**
<details>
<summary>Answer</summary>

**Indexing Phase (One-time):**
1. Load documents from sources
2. Chunk documents (e.g., 1000 tokens with 200 overlap)
3. Generate embeddings for each chunk
4. Store embeddings + text in vector database

**Query Phase (Runtime):**
1. User asks a question
2. Embed the question using same model
3. Search vector DB for top K similar chunks
4. Format retrieved chunks as context
5. Send context + question to LLM
6. LLM generates answer based on context
7. Return answer to user

The key is semantic search - finding chunks whose embeddings are closest to the query embedding.
</details>

---

**Q3: What are embeddings and how do they enable semantic search?**
<details>
<summary>Answer</summary>

Embeddings are dense vector representations of text that capture semantic meaning. For example:
- "king" → [0.2, 0.8, 0.1, ...]
- "queen" → [0.21, 0.79, 0.12, ...]

Similar concepts have similar vectors (measured by cosine similarity or distance).

They enable semantic search because:
1. Convert query to embedding
2. Find documents with similar embeddings (using cosine similarity)
3. High similarity = semantically related = relevant to query

This is better than keyword search because it understands meaning:
- Query: "What is Python?"
- Matches: "Python is a programming language" (high similarity)
- Doesn't match: "Snakes are reptiles" (low similarity, despite "python" being a snake)
</details>

---

### Intermediate Questions

**Q4: How do you choose chunk size and overlap? What are the tradeoffs?**
<details>
<summary>Answer</summary>

**Chunk Size Tradeoffs:**
- **Small chunks (200-500 tokens):**
  - Pros: Precise retrieval, more granular
  - Cons: May lack context, need to retrieve more chunks

- **Large chunks (1000-2000 tokens):**
  - Pros: More context, fewer retrievals needed
  - Cons: Less precise, may include irrelevant info, costs more in LLM tokens

**Overlap Purpose:**
- Prevents information loss at chunk boundaries
- Example: Sentence split across chunks gets captured in both
- Typically 10-20% overlap (e.g., 200 tokens for 1000-token chunks)

**My Recommendation:**
- Start with 1000 tokens, 200 overlap (balanced)
- Test retrieval quality on sample questions
- Adjust based on: document structure, query complexity, LLM context limit
</details>

---

**Q5: Compare FAISS, Chroma, and Pinecone. When would you use each?**
<details>
<summary>Answer</summary>

**FAISS:**
- Use for: Fast prototyping, offline/edge deployments, small datasets
- Pros: Fastest, free, no external dependencies
- Cons: In-memory only, no persistence, no metadata filtering
- Scale: Up to 1M vectors

**Chroma:**
- Use for: MVPs, prototypes, moderate scale
- Pros: Easy API, built-in persistence, metadata filtering
- Cons: Slower than FAISS at large scale
- Scale: Up to 10M vectors

**Pinecone:**
- Use for: Production systems, high scale
- Pros: Managed, scales to billions, low latency, production-ready
- Cons: Costs money, vendor lock-in
- Scale: Billions of vectors

**My Approach for Production:**
1. Prototype with Chroma (validate idea quickly)
2. Migrate to Pinecone when scaling or going to production
3. Use FAISS for edge cases or cost-sensitive deployments
</details>

---

**Q6: How would you improve retrieval quality in a RAG system?**
<details>
<summary>Answer</summary>

**5 Key Techniques:**

1. **Reranking**:
   - Get top 20 from vector search
   - Use cross-encoder to rerank to best 4
   - More accurate but slower

2. **Hybrid Search**:
   - Combine vector search (semantic) with BM25 (keyword)
   - Weighted combination: 70% vector + 30% keyword
   - Catches both semantic and exact matches

3. **Better Chunking**:
   - Use semantic chunking (respect paragraphs/sections)
   - Add chunk context (e.g., section headers in each chunk)
   - Experiment with sizes

4. **Query Enhancement**:
   - Rephrase query with LLM (HyDE - Hypothetical Document Embeddings)
   - Generate multiple query variations
   - Expand abbreviations/acronyms

5. **Metadata Filtering**:
   - Filter by date, source, category before/after retrieval
   - Example: "Recent papers only" → filter by date > 2023

**Monitoring:**
- Track retrieval metrics (precision@k, recall@k)
- User feedback on answer quality
- Log failed retrievals
</details>

---

### Advanced Questions

**Q7: You have 10,000 PDF documents to index. Walk me through your production pipeline.**
<details>
<summary>Answer</summary>

**Architecture:**

1. **Document Processing (Batch Job)**:
   ```python
   # Process in parallel
   from multiprocessing import Pool

   def process_pdf(pdf_path):
       loader = PyPDFLoader(pdf_path)
       docs = loader.load()
       chunks = chunker.split_documents(docs)
       return chunks

   with Pool(8) as p:
       all_chunks = p.map(process_pdf, pdf_paths)
   ```

2. **Embedding (Batched)**:
   ```python
   # Batch embed for efficiency
   embeddings = model.encode(
       [chunk.page_content for chunk in all_chunks],
       batch_size=64,
       show_progress_bar=True
   )
   ```

3. **Vector DB Upload (Batched)**:
   ```python
   # Pinecone has 100-item batch limit
   for i in range(0, len(chunks), 100):
       batch = chunks[i:i+100]
       index.upsert(vectors=batch)
   ```

4. **Error Handling**:
   - Retry failed PDFs (corrupted files)
   - Log progress to resume if interrupted
   - Validate embeddings (check for NaN, zero vectors)

5. **Monitoring**:
   - Track processing time per PDF
   - Monitor embedding API costs
   - Verify vector DB index size

**Optimization:**
- Use GPU for local embedding models
- Cache embeddings to avoid re-processing
- Implement incremental updates (only new/changed PDFs)

**Timeline Estimate**: 10K PDFs × 5 pages avg × 3 chunks/page = 150K chunks
- Embedding: ~2 hours (local GPU) or $15 (OpenAI API)
- Upload: ~30 mins
- Total: ~3 hours
</details>

---

**Q8: How would you handle multi-lingual documents in RAG?**
<details>
<summary>Answer</summary>

**Approach:**

1. **Multi-lingual Embedding Models**:
   ```python
   # Use models trained on multiple languages
   from sentence_transformers import SentenceTransformer

   model = SentenceTransformer('paraphrase-multilingual-mpnet-base-v2')
   # Supports 50+ languages in same vector space
   ```

2. **Language Detection**:
   ```python
   from langdetect import detect

   lang = detect(query)
   # Filter chunks by language if needed
   ```

3. **Translation (Optional)**:
   - Option A: Translate documents to English before indexing
   - Option B: Translate query to match document language
   - Option C: Use multilingual embeddings (no translation needed)

4. **LLM Selection**:
   - Use multilingual models (GPT-4, Claude support 50+ languages)
   - Or route to language-specific models

**Best Practice**: Use multilingual embeddings (no translation) for:
- Lower latency (no translation API call)
- Preserves nuance (translation loses meaning)
- Cheaper (no translation costs)

Only translate if:
- Your LLM doesn't support the language
- You need English-only output
</details>

---

**Q9: A user complains RAG is returning irrelevant results. How do you debug?**
<details>
<summary>Answer</summary>

**Debugging Checklist:**

1. **Check Retrieved Chunks**:
   ```python
   # Log what was retrieved
   for doc in retrieved_docs:
       print(f"Score: {doc.metadata['score']}")
       print(f"Content: {doc.page_content[:200]}...")
   ```
   - Are chunks actually relevant to the query?
   - Are scores too low (< 0.5 similarity)?

2. **Inspect Embeddings**:
   ```python
   # Check query embedding
   query_emb = model.encode(query)
   print(f"Query embedding norm: {np.linalg.norm(query_emb)}")

   # Compare with document embeddings
   similarities = cosine_similarity([query_emb], doc_embeddings)
   print(f"Top similarities: {sorted(similarities[0], reverse=True)[:5]}")
   ```
   - Is embedding model working? (non-zero, normalized)

3. **Test Chunking**:
   - Are chunks too large/small?
   - Do they contain complete thoughts?
   - Is overlap sufficient?

4. **Check Metadata Filters**:
   - Are filters too restrictive?
   - Example: Filtering by date might exclude relevant older docs

5. **Embedding Model Mismatch**:
   - Are you using same model for indexing and querying?
   - If you changed models, re-index everything

6. **Query Quality**:
   - Is query too vague? ("Tell me about stuff")
   - Try query expansion or rephrasing

**Solution Based on Root Cause:**
- Low similarity scores → Try different embedding model or hybrid search
- Chunks are relevant but answer is wrong → Improve LLM prompt
- No relevant chunks exist → Add more documents or improve chunking
- High similarity but wrong content → Reranking or better chunking
</details>

---

**Q10: Design a RAG system for a customer support chatbot with 100K support tickets.**
<details>
<summary>Answer</summary>

**Requirements Analysis:**
- 100K tickets = likely 300K+ chunks (3 per ticket avg)
- Need fast retrieval (<500ms)
- High accuracy (customer-facing)
- Filter by: product, date, resolution status

**Architecture:**

```
                       ┌──────────────┐
                       │  User Query  │
                       └──────┬───────┘
                              │
                              ▼
                    ┌────────────────────┐
                    │ Intent Classifier  │ (Is it support-related?)
                    └────────┬───────────┘
                             │
                             ▼
                  ┌──────────────────────┐
                  │  Query Enhancement   │ (Expand abbreviations)
                  └──────────┬───────────┘
                             │
                             ▼
         ┌───────────────────────────────────┐
         │      Vector Search (Pinecone)     │
         │  Filter: product, date range      │
         │  Retrieve: Top 20                 │
         └───────────────┬───────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │  Reranker (Cross-    │
              │  Encoder)            │
              │  Top 4 most relevant │
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │  LLM (GPT-4)         │
              │  System: Support bot │
              │  Context: Top 4      │
              └──────────┬───────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │  Response + Sources  │
              │  "See Ticket #12345" │
              └──────────────────────┘
```

**Implementation Details:**

1. **Data Ingestion**:
   ```python
   # Structure for each ticket
   {
       "id": "ticket_12345",
       "title": "App crashes on login",
       "description": "...",
       "resolution": "...",
       "metadata": {
           "product": "Mobile App",
           "status": "resolved",
           "date": "2024-01-15",
           "category": "bug"
       }
   }

   # Chunk strategy: Keep ticket as atomic unit (don't split)
   # Each ticket = 1 chunk (if <2000 tokens)
   ```

2. **Vector DB (Pinecone)**:
   - Create index with metadata filters
   - Use namespaces for different products
   - Enable hybrid search

3. **Retrieval Strategy**:
   ```python
   # First: Vector search with filters
   results = index.query(
       vector=query_embedding,
       top_k=20,
       filter={
           "product": user_product,
           "status": "resolved"  # Only show resolved tickets
       }
   )

   # Second: Rerank
   reranked = reranker.rank(query, results)[:4]
   ```

4. **LLM Prompt**:
   ```python
   system_prompt = """You are a customer support assistant.
   Answer based on previous support tickets.

   Instructions:
   1. Provide clear, step-by-step solutions
   2. Cite ticket numbers: [Ticket #12345]
   3. If multiple solutions exist, list all options
   4. If no relevant ticket exists, say: "I'll escalate this to our team."

   Previous Similar Tickets:
   {context}

   Customer Question: {query}
   """
   ```

5. **Monitoring & Improvement**:
   - Track: resolution rate, customer satisfaction, retrieval quality
   - A/B test: different chunk sizes, embedding models, rerankers
   - Collect feedback: thumbs up/down on answers
   - Retrain: Fine-tune retriever on positive examples

**Scalability:**
- Pinecone handles 300K vectors easily
- Add caching for common queries
- Implement rate limiting per user
- Consider streaming responses for long answers

**Estimated Costs (Monthly):**
- Pinecone: ~$50 (300K vectors)
- OpenAI Embeddings: ~$30 (100K queries)
- GPT-4 Responses: ~$200 (100K queries)
- Total: ~$280/month
</details>

---

## 9. Hands-On Exercise

See `hands-on-rag.py` for a complete working RAG system.

**Task:** Run the code and understand each step:
1. Document loading and chunking
2. Embedding generation
3. Vector store creation
4. Query and retrieval
5. LLM integration

**Extension Ideas:**
- Add more documents
- Try different chunk sizes
- Implement reranking
- Add metadata filtering

---

## 10. Key Takeaways

1. **RAG = Retrieval + Generation**: Find relevant docs, pass to LLM
2. **Embeddings capture semantics**: Similar meaning = similar vectors
3. **Vector DBs enable fast search**: Use Chroma for MVP, Pinecone for production
4. **Chunking matters**: 1000 tokens with 200 overlap is a good start
5. **Production considerations**: Reranking, hybrid search, monitoring

**For Interview:**
- Connect RAG to your LAIPath orchestration experience
- Emphasize production thinking (scaling, monitoring, cost)
- Show you understand tradeoffs (FAISS vs Pinecone, chunk sizes)
- Be ready to design a RAG system end-to-end

---

## Next Steps

1. Complete `hands-on-rag.py` (30 mins)
2. Review interview questions (30 mins)
3. Move to Module 2: LangChain (30-60 mins)
4. Tomorrow: Build the AskMyProjects RAG system (Module 5)

Good luck!
