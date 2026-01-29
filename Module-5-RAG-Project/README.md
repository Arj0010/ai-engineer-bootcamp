# Module 5: Hands-On RAG Project

**Duration:** 2 hours
**Goal:** Build "AskMyProjects" - a RAG system over your portfolio

## Why This Project?

1. **Hands-on RAG experience** you can reference in the interview
2. **Demo-able code** to show technical competence
3. **Reinforces your project knowledge** by encoding it in a system
4. **Shows you can build production systems quickly** - learned RAG this week, built a system overnight

---

## Project Overview

### What We're Building

A RAG system that can answer questions like:
- "What production AI systems has Arjun built?"
- "How does LAIPath handle user progress?"
- "What's Arjun's experience with LLM APIs?"
- "Tell me about the PlanLift architecture."

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    AskMyProjects RAG                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 Document Ingestion                   │   │
│  │  ┌──────────┐   ┌──────────┐   ┌──────────────┐    │   │
│  │  │ Resume   │   │ Project  │   │ README       │    │   │
│  │  │ .txt     │   │ Desc.    │   │ files        │    │   │
│  │  └────┬─────┘   └────┬─────┘   └──────┬───────┘    │   │
│  │       │              │                │             │   │
│  │       └──────────────┼────────────────┘             │   │
│  │                      ▼                              │   │
│  │              ┌──────────────┐                       │   │
│  │              │   Chunker    │                       │   │
│  │              └──────┬───────┘                       │   │
│  └─────────────────────┼───────────────────────────────┘   │
│                        ▼                                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Embedding & Storage                     │   │
│  │  ┌──────────────┐        ┌──────────────────────┐   │   │
│  │  │  Embedding   │───────▶│   ChromaDB           │   │   │
│  │  │  Model       │        │   Vector Store       │   │   │
│  │  └──────────────┘        └──────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                    Query Pipeline                    │   │
│  │                                                      │   │
│  │   ┌───────┐    ┌──────────┐    ┌───────────────┐   │   │
│  │   │ Query │───▶│ Retrieval│───▶│ LLM Synthesis │   │   │
│  │   └───────┘    └──────────┘    └───────────────┘   │   │
│  │                                         │           │   │
│  │                                         ▼           │   │
│  │                              ┌──────────────────┐   │   │
│  │                              │     Answer       │   │   │
│  │                              └──────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Step 1: Setup (10 mins)

### Install Dependencies

```bash
pip install openai chromadb langchain langchain-community langchain-openai sentence-transformers tiktoken
```

### Set API Key

```bash
# Windows
set OPENAI_API_KEY=your-key-here

# Linux/Mac
export OPENAI_API_KEY=your-key-here
```

### Create Project Structure

```bash
mkdir askmyprojects
cd askmyprojects
```

---

## Step 2: Create Your Knowledge Base (15 mins)

Create a folder `documents/` with files describing your projects:

### documents/laipath.txt

```text
# LAIPath - AI-Powered Adaptive Learning Platform

## Overview
LAIPath is a production AI system that generates personalized learning paths using LLM APIs. Built with Node.js and Express, it uses controlled workflows and reflection mechanisms to adapt to user progress.

## Architecture
- Orchestration Engine: Manages multi-step LLM interactions
- LLM Integration Layer: Structured prompting with output validation
- Progress Tracking: Monitors user interactions and progress signals
- Adaptive Logic: Reflection mechanism for self-correction

## Key Technical Decisions
1. Structured Prompting: Used explicit output formats (JSON) instead of free-form text
2. Custom Orchestration: Built workflow management from scratch for full control
3. Reflection Mechanism: System evaluates outputs and triggers refinement when needed
4. Progress Signals: Tracks adherence to guide future recommendations

## Production Challenges Solved
- LLM output unpredictability: Solved with structured prompts + validation
- Workflow complexity: Custom orchestration engine with state management
- Quality control: Reflection mechanism for self-improvement
- Error handling: Graceful degradation with retry logic

## Technology Stack
- Backend: Node.js, Express
- AI: LLM APIs with structured prompting
- Orchestration: Custom workflow engine
- Status: In testing with real users

## Results
- Successfully orchestrates multiple LLM calls per session
- Maintains coherent, personalized learning experiences
- Handles production load with reliability
```

### documents/planlift.txt

```text
# PlanLift - AI-Powered 2D to 3D Architectural Visualization

## Overview
PlanLift converts 2D architectural blueprints into 3D visualizations using AI. A full-stack AI product MVP demonstrating backend-to-model integration.

## Architecture
- File Upload: Handles various blueprint formats
- Processing Pipeline: Validates and normalizes input
- AI Model Integration: External models for 2D-to-3D conversion
- Output Generation: Renders viewable 3D output

## Technical Highlights
1. Full-Stack Implementation: Backend APIs, AI integration, output delivery
2. External Model Integration: Abstraction layer for model switching
3. Production Considerations: File validation, progress tracking, error handling

## Technology Stack
- Backend APIs for file processing
- AI model orchestration
- Output rendering pipeline

## Trade-offs
- Used external AI models vs. training custom: Prioritized MVP speed over custom accuracy
- Would fine-tune on architectural data for production scale

## Results
- Functional MVP demonstrating 2D-to-3D pipeline
- Shows full-stack AI product development capability
```

### documents/skills.txt

```text
# Arjun's Technical Skills and Experience

## Production AI Systems
- Built LAIPath: Production AI learning platform with LLM orchestration
- Built PlanLift: Full-stack AI product for architectural visualization
- Experience: LLM API integration, workflow orchestration, structured prompting

## LLM and AI Experience
- LLM API Integration: OpenAI APIs, structured prompting, output parsing
- Workflow Orchestration: Multi-step LLM interactions, state management
- Reflection Mechanisms: Self-correction patterns for quality control
- Prompt Engineering: Structured prompts with explicit output formats

## Deep Learning
- Architectures: Conv1D, BiLSTM for sequence processing
- Applications: Drug likeness prediction (88% accuracy)
- Experience: Model training, inference pipelines

## Machine Learning Fundamentals
- Dimensionality Reduction: PCA for feature analysis
- Clustering: K-means and hierarchical clustering
- Classification: Various algorithms, 88% accuracy on real datasets

## Currently Learning
- RAG (Retrieval Augmented Generation)
- Vector Databases: ChromaDB, FAISS, Pinecone
- LangChain and LangGraph frameworks
- Multi-agent systems

## Technical Stack
- Languages: Python, JavaScript, Node.js
- Frameworks: Express, LangChain (learning)
- Databases: Vector databases (ChromaDB)
- AI: LLM APIs, Hugging Face models
```

---

## Step 3: Build the RAG System (45 mins)

### askmyprojects.py

```python
"""
AskMyProjects - RAG System for Portfolio Q&A
Built for AI Engineer Interview at Wednesday Solutions
"""

import os
from pathlib import Path
from typing import List

# ChromaDB for vector storage
import chromadb
from chromadb.utils import embedding_functions

# LangChain components
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain_openai import ChatOpenAI, OpenAIEmbeddings
from langchain_community.vectorstores import Chroma
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.output_parsers import StrOutputParser
from langchain_core.runnables import RunnablePassthrough


class AskMyProjects:
    """RAG system for answering questions about my projects."""

    def __init__(self, documents_dir: str = "documents"):
        self.documents_dir = Path(documents_dir)
        self.vectorstore = None
        self.retriever = None
        self.llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0)

        # Initialize the system
        self._load_and_index_documents()
        self._setup_qa_chain()

    def _load_documents(self) -> List[str]:
        """Load all documents from the documents directory."""
        documents = []

        for file_path in self.documents_dir.glob("*.txt"):
            print(f"Loading: {file_path.name}")
            with open(file_path, "r", encoding="utf-8") as f:
                content = f.read()
                documents.append({
                    "content": content,
                    "source": file_path.name
                })

        print(f"Loaded {len(documents)} documents")
        return documents

    def _chunk_documents(self, documents: List[dict]) -> List[dict]:
        """Split documents into chunks for embedding."""
        text_splitter = RecursiveCharacterTextSplitter(
            chunk_size=500,          # Characters per chunk
            chunk_overlap=50,        # Overlap between chunks
            separators=["\n\n", "\n", ".", " "]
        )

        chunks = []
        for doc in documents:
            doc_chunks = text_splitter.split_text(doc["content"])
            for i, chunk in enumerate(doc_chunks):
                chunks.append({
                    "content": chunk,
                    "source": doc["source"],
                    "chunk_id": i
                })

        print(f"Created {len(chunks)} chunks")
        return chunks

    def _load_and_index_documents(self):
        """Load documents, chunk them, and create vector index."""
        # Load raw documents
        documents = self._load_documents()

        # Chunk documents
        chunks = self._chunk_documents(documents)

        # Create embeddings and store in ChromaDB
        embeddings = OpenAIEmbeddings()

        texts = [chunk["content"] for chunk in chunks]
        metadatas = [{"source": chunk["source"], "chunk_id": chunk["chunk_id"]}
                     for chunk in chunks]

        # Create vector store
        self.vectorstore = Chroma.from_texts(
            texts=texts,
            embedding=embeddings,
            metadatas=metadatas,
            collection_name="my_projects",
            persist_directory="./chroma_db"
        )

        # Create retriever
        self.retriever = self.vectorstore.as_retriever(
            search_type="similarity",
            search_kwargs={"k": 4}  # Return top 4 relevant chunks
        )

        print("Vector index created successfully!")

    def _setup_qa_chain(self):
        """Set up the RAG question-answering chain."""
        # Define the prompt template
        template = """You are an AI assistant helping answer questions about Arjun's projects and experience.

Use the following context to answer the question. If you cannot find the answer in the context, say "I don't have that information."

Context:
{context}

Question: {question}

Answer the question based on the context. Be specific and cite the relevant projects when appropriate."""

        self.prompt = ChatPromptTemplate.from_template(template)

        # Build the chain
        self.chain = (
            {"context": self.retriever | self._format_docs, "question": RunnablePassthrough()}
            | self.prompt
            | self.llm
            | StrOutputParser()
        )

    def _format_docs(self, docs) -> str:
        """Format retrieved documents into a single context string."""
        return "\n\n---\n\n".join([doc.page_content for doc in docs])

    def query(self, question: str) -> str:
        """Query the RAG system with a question."""
        return self.chain.invoke(question)

    def query_with_sources(self, question: str) -> dict:
        """Query and return both answer and source documents."""
        # Retrieve relevant documents
        docs = self.retriever.invoke(question)

        # Format context
        context = self._format_docs(docs)

        # Generate answer
        answer = self.chain.invoke(question)

        # Extract sources
        sources = list(set([doc.metadata["source"] for doc in docs]))

        return {
            "question": question,
            "answer": answer,
            "sources": sources,
            "num_chunks_used": len(docs)
        }


def main():
    """Demo the AskMyProjects RAG system."""
    print("=" * 60)
    print("AskMyProjects - RAG System Demo")
    print("=" * 60)
    print()

    # Initialize the system
    print("Initializing RAG system...")
    rag = AskMyProjects()
    print()

    # Demo questions
    questions = [
        "What production AI systems has Arjun built?",
        "How does LAIPath handle LLM unpredictability?",
        "What is Arjun's experience with LLM APIs?",
        "Tell me about the PlanLift architecture.",
        "What deep learning architectures has Arjun used?",
    ]

    print("=" * 60)
    print("Answering Questions")
    print("=" * 60)

    for q in questions:
        print(f"\nQ: {q}")
        print("-" * 40)

        result = rag.query_with_sources(q)

        print(f"A: {result['answer']}")
        print(f"\nSources: {', '.join(result['sources'])}")
        print(f"Chunks used: {result['num_chunks_used']}")
        print()

    # Interactive mode
    print("=" * 60)
    print("Interactive Mode - Ask anything about my projects!")
    print("Type 'quit' to exit")
    print("=" * 60)

    while True:
        question = input("\nYour question: ").strip()

        if question.lower() == 'quit':
            print("Goodbye!")
            break

        if not question:
            continue

        result = rag.query_with_sources(question)
        print(f"\n{result['answer']}")
        print(f"\n(Sources: {', '.join(result['sources'])})")


if __name__ == "__main__":
    main()
```

---

## Step 4: Advanced Features (30 mins)

### Add Hybrid Search

Combine semantic search with keyword matching:

```python
def hybrid_query(self, question: str, keyword_boost: float = 0.3) -> str:
    """Hybrid search combining semantic and keyword matching."""
    # Semantic search
    semantic_docs = self.retriever.invoke(question)

    # Simple keyword search (production would use BM25)
    keywords = question.lower().split()
    all_docs = self.vectorstore.get()

    keyword_scores = {}
    for i, text in enumerate(all_docs['documents']):
        score = sum(1 for kw in keywords if kw in text.lower())
        if score > 0:
            keyword_scores[i] = score

    # Combine results (simplified - production would use proper fusion)
    # For now, just use semantic results
    context = self._format_docs(semantic_docs)

    return self.chain.invoke(question)
```

### Add Query Expansion

Improve retrieval by expanding the query:

```python
def expand_query(self, question: str) -> str:
    """Expand query with related terms for better retrieval."""
    expansion_prompt = f"""Given this question about a software engineer's portfolio:
"{question}"

Generate 2-3 related search queries that might help find relevant information.
Return only the queries, one per line."""

    response = self.llm.invoke(expansion_prompt)

    # Combine original and expanded queries
    expanded = f"{question}\n{response.content}"
    return expanded
```

### Add Reranking

Rerank results for better relevance:

```python
def query_with_rerank(self, question: str) -> str:
    """Query with result reranking for improved relevance."""
    # Get more candidates than we need
    docs = self.vectorstore.similarity_search(question, k=8)

    # Rerank using LLM
    rerank_prompt = f"""Given the question: "{question}"

Rank these text chunks by relevance (1 = most relevant):

{chr(10).join([f'{i+1}. {doc.page_content[:200]}...' for i, doc in enumerate(docs)])}

Return the numbers in order of relevance, comma-separated."""

    response = self.llm.invoke(rerank_prompt)

    # Parse ranking and select top 4
    try:
        rankings = [int(x.strip()) - 1 for x in response.content.split(',')]
        reranked_docs = [docs[i] for i in rankings[:4] if i < len(docs)]
    except:
        reranked_docs = docs[:4]

    context = self._format_docs(reranked_docs)
    return self.chain.invoke(question)
```

---

## Step 5: Test and Refine (15 mins)

### Test Questions to Try

```python
test_questions = [
    # Direct questions
    "What is LAIPath?",
    "What is PlanLift?",

    # Technical depth
    "How does LAIPath handle workflow orchestration?",
    "What's the architecture of the LAIPath system?",

    # Cross-project
    "What production challenges has Arjun solved?",
    "What AI frameworks has Arjun used?",

    # Specific skills
    "Does Arjun have experience with deep learning?",
    "What about vector databases?",

    # Edge cases
    "Has Arjun worked at Google?",  # Should say no info
    "What's the weather today?",     # Should say not relevant
]
```

### Evaluation Metrics

Track these for interview discussion:

1. **Retrieval Quality**: Are the right chunks being retrieved?
2. **Answer Accuracy**: Is the LLM synthesizing correctly?
3. **Source Attribution**: Are sources correctly identified?
4. **Edge Cases**: How does it handle out-of-scope questions?

---

## Step 6: Interview Demo Script (5 mins)

When showing this in an interview:

### 1. Explain the Architecture (30 seconds)

> "This is a RAG system I built to answer questions about my projects. It has three components: document ingestion with chunking, vector storage using ChromaDB with OpenAI embeddings, and a retrieval-augmented generation pipeline using LangChain."

### 2. Show the Code (1 minute)

> "The key class is `AskMyProjects`. It loads documents, chunks them with RecursiveCharacterTextSplitter, embeds them with OpenAI, and stores in ChromaDB. The query pipeline retrieves top-4 similar chunks and sends them to GPT with the question."

### 3. Demo Live (1-2 minutes)

> "Let me show you it working..."
>
> Run: `python askmyprojects.py`
>
> Ask: "What production AI systems has Arjun built?"
>
> "As you can see, it retrieved relevant chunks from my LAIPath and PlanLift documentation and synthesized an accurate answer."

### 4. Discuss Improvements (30 seconds)

> "With more time, I'd add: hybrid search combining semantic and keyword matching, query expansion for better recall, and a reranking step for improved precision. I'd also add evaluation metrics to track retrieval quality."

---

## Key Talking Points for Interview

### Why These Technical Choices?

1. **ChromaDB**: Lightweight, good for prototyping, easy to set up
2. **OpenAI Embeddings**: High quality, well-supported
3. **RecursiveCharacterTextSplitter**: Preserves semantic boundaries
4. **Top-4 Retrieval**: Balance between context and relevance

### What Would You Change for Production?

1. **Vector DB**: Might use Pinecone for scale, or FAISS for cost
2. **Embeddings**: Consider open-source models for cost
3. **Chunking**: More sophisticated strategies based on content type
4. **Evaluation**: Add automated testing of retrieval quality

### How Does This Relate to LAIPath?

> "LAIPath uses LLM APIs with structured prompting. Adding RAG would let it reference documentation, past sessions, or course materials. The orchestration patterns I built in LAIPath would work well with RAG - the retrieval step becomes another node in the workflow."

---

## Complete File Structure

```
askmyprojects/
├── documents/
│   ├── laipath.txt
│   ├── planlift.txt
│   └── skills.txt
├── chroma_db/           # Created automatically
│   └── ...
├── askmyprojects.py     # Main RAG system
└── README.md            # Project documentation
```

---

## Push to GitHub Before Interview!

```bash
cd askmyprojects
git init
git add .
git commit -m "Add AskMyProjects RAG system - portfolio Q&A"
git remote add origin https://github.com/yourusername/askmyprojects.git
git push -u origin main
```

Now you have a working RAG system you can demo and discuss!
