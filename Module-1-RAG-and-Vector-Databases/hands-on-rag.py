"""
Hands-On RAG System
===================
Complete working RAG system using Chroma and sentence-transformers.

This demonstrates:
1. Document loading and chunking
2. Embedding generation
3. Vector store creation with Chroma
4. Semantic search and retrieval
5. LLM integration for answer generation

Requirements:
    pip install chromadb sentence-transformers openai langchain

Author: AI Engineer Bootcamp
Duration: 30 minutes to understand and run
"""

import os
from typing import List, Dict
import chromadb
from chromadb.config import Settings
from sentence_transformers import SentenceTransformer
from openai import OpenAI

# ============================================================================
# PART 1: Document Chunking
# ============================================================================

def chunk_text(text: str, chunk_size: int = 1000, overlap: int = 200) -> List[str]:
    """
    Split text into overlapping chunks.

    Args:
        text: Input text to chunk
        chunk_size: Target size of each chunk (characters)
        overlap: Number of characters to overlap between chunks

    Returns:
        List of text chunks

    Why overlapping?
    - Prevents information loss at boundaries
    - Ensures context continuity
    - 20% overlap is typically optimal
    """
    chunks = []
    start = 0

    while start < len(text):
        # Get chunk from start to chunk_size
        end = start + chunk_size
        chunk = text[start:end]

        # Try to end at sentence boundary for better semantic coherence
        if end < len(text):
            # Find last period, question mark, or exclamation
            last_period = max(
                chunk.rfind('.'),
                chunk.rfind('?'),
                chunk.rfind('!')
            )
            if last_period > chunk_size * 0.5:  # At least 50% through chunk
                chunk = chunk[:last_period + 1]
                end = start + last_period + 1

        chunks.append(chunk.strip())

        # Move start position with overlap
        start = end - overlap

    return chunks


def semantic_chunk(text: str, max_chunk_size: int = 1000) -> List[str]:
    """
    More advanced chunking that respects document structure.

    This splits on:
    1. Paragraphs (\\n\\n)
    2. Sentences (. ? !)
    3. Arbitrary boundaries (as fallback)

    Better than fixed chunking for structured documents.
    """
    # Split by double newline (paragraphs)
    paragraphs = text.split('\n\n')

    chunks = []
    current_chunk = ""

    for para in paragraphs:
        # If adding this paragraph exceeds max size
        if len(current_chunk) + len(para) > max_chunk_size:
            if current_chunk:
                chunks.append(current_chunk.strip())
                current_chunk = para
            else:
                # Paragraph itself is too large, split it
                chunks.extend(chunk_text(para, max_chunk_size, 200))
        else:
            current_chunk += "\n\n" + para if current_chunk else para

    if current_chunk:
        chunks.append(current_chunk.strip())

    return chunks


# ============================================================================
# PART 2: Sample Documents (Replace with your own data)
# ============================================================================

SAMPLE_DOCUMENTS = [
    {
        "id": "doc_python",
        "text": """Python is a high-level, interpreted programming language known for its
        simplicity and readability. Created by Guido van Rossum and first released in 1991,
        Python emphasizes code readability with its notable use of significant indentation.

        Python supports multiple programming paradigms, including structured, object-oriented,
        and functional programming. It features dynamic typing and garbage collection.

        Popular use cases include web development (Django, Flask), data science (pandas, numpy),
        machine learning (scikit-learn, TensorFlow, PyTorch), automation, and scripting.

        Python's extensive standard library and vibrant ecosystem of third-party packages make
        it one of the most popular programming languages in the world.""",
        "metadata": {"category": "programming", "language": "english"}
    },
    {
        "id": "doc_machine_learning",
        "text": """Machine Learning (ML) is a subset of artificial intelligence that enables
        systems to learn and improve from experience without being explicitly programmed.
        The core idea is to build algorithms that can receive input data and use statistical
        analysis to predict outputs.

        There are three main types of machine learning:
        1. Supervised Learning: Training with labeled data (e.g., classification, regression)
        2. Unsupervised Learning: Finding patterns in unlabeled data (e.g., clustering)
        3. Reinforcement Learning: Learning through trial and error with rewards

        Common ML algorithms include linear regression, decision trees, random forests,
        support vector machines, neural networks, and deep learning models.

        Applications span computer vision, natural language processing, recommendation systems,
        fraud detection, autonomous vehicles, and healthcare diagnostics.""",
        "metadata": {"category": "ai", "language": "english"}
    },
    {
        "id": "doc_rag",
        "text": """Retrieval-Augmented Generation (RAG) is an AI framework that combines
        information retrieval with text generation. RAG enhances large language models by
        providing them with relevant context from external knowledge bases.

        The RAG process works in two phases:

        Indexing Phase:
        - Documents are split into chunks
        - Each chunk is converted to embeddings (vector representations)
        - Embeddings are stored in a vector database

        Query Phase:
        - User query is converted to an embedding
        - Similar document chunks are retrieved using semantic search
        - Retrieved context is provided to the LLM
        - LLM generates an answer based on the context

        Benefits of RAG:
        - Reduces hallucinations by grounding responses in factual data
        - Enables dynamic knowledge updates without retraining
        - Provides source attribution for transparency
        - More cost-effective than fine-tuning for knowledge updates

        RAG is widely used in chatbots, question-answering systems, and enterprise search.""",
        "metadata": {"category": "ai", "language": "english"}
    },
    {
        "id": "doc_vector_databases",
        "text": """Vector databases are specialized databases designed to store and efficiently
        search high-dimensional vectors (embeddings). Unlike traditional databases that use
        exact matching, vector databases find similar vectors using distance metrics.

        Key Features:
        - Approximate Nearest Neighbor (ANN) search for fast retrieval
        - Support for metadata filtering
        - Horizontal scalability for billions of vectors
        - Multiple distance metrics (cosine similarity, Euclidean, dot product)

        Popular Vector Databases:

        1. Pinecone: Fully managed cloud service, scales to billions of vectors, production-ready
        2. Chroma: Open-source, easy to use, good for prototyping and moderate scale
        3. FAISS: Facebook's library, extremely fast, in-memory, best for offline use
        4. Weaviate: Open-source with GraphQL API, supports hybrid search
        5. Milvus: Open-source, Kubernetes-native, enterprise-grade

        Vector databases are essential for RAG systems, recommendation engines,
        semantic search, and similarity-based applications.""",
        "metadata": {"category": "databases", "language": "english"}
    },
    {
        "id": "doc_laipath",
        "text": """LAIPath is a production AI system that demonstrates advanced workflow
        orchestration and LLM API integration. The system showcases adaptive logic that
        dynamically adjusts based on user inputs and system state.

        Key Features:
        - Workflow orchestration with multiple AI agents
        - Integration with various LLM APIs (OpenAI, Anthropic, etc.)
        - Adaptive decision-making based on context
        - Production-ready error handling and monitoring
        - Scalable architecture for handling concurrent requests

        The system's architecture separates concerns into:
        - API Layer: Handles external LLM calls with retry logic
        - Orchestration Layer: Manages workflow state and transitions
        - Logic Layer: Implements adaptive algorithms for decision-making

        LAIPath represents modern production AI engineering practices including proper
        logging, monitoring, error handling, and testing.""",
        "metadata": {"category": "projects", "language": "english"}
    }
]


# ============================================================================
# PART 3: RAG System Implementation
# ============================================================================

class SimpleRAG:
    """
    A simple but complete RAG system.

    Components:
    1. Embedding model (sentence-transformers)
    2. Vector database (Chroma)
    3. LLM for generation (OpenAI GPT-4)
    """

    def __init__(
        self,
        embedding_model_name: str = "sentence-transformers/all-mpnet-base-v2",
        persist_directory: str = "./chroma_db",
        openai_api_key: str = None
    ):
        """
        Initialize RAG system.

        Args:
            embedding_model_name: HuggingFace model for embeddings
            persist_directory: Where to save Chroma database
            openai_api_key: OpenAI API key for generation
        """
        print("Initializing RAG system...")

        # 1. Load embedding model
        print(f"Loading embedding model: {embedding_model_name}")
        self.embedding_model = SentenceTransformer(embedding_model_name)

        # 2. Initialize Chroma client
        print(f"Initializing Chroma database at: {persist_directory}")
        self.chroma_client = chromadb.Client(Settings(
            persist_directory=persist_directory,
            anonymized_telemetry=False
        ))

        # 3. Create or get collection
        self.collection_name = "rag_documents"
        try:
            self.collection = self.chroma_client.create_collection(
                name=self.collection_name,
                metadata={"description": "RAG document collection"}
            )
            print(f"Created new collection: {self.collection_name}")
        except:
            self.collection = self.chroma_client.get_collection(self.collection_name)
            print(f"Using existing collection: {self.collection_name}")

        # 4. Initialize OpenAI client
        self.openai_api_key = openai_api_key or os.getenv("OPENAI_API_KEY")
        if self.openai_api_key:
            self.openai_client = OpenAI(api_key=self.openai_api_key)
            print("OpenAI client initialized")
        else:
            self.openai_client = None
            print("WARNING: No OpenAI API key provided. Generation will not work.")

        print("RAG system initialized!\n")

    def add_documents(self, documents: List[Dict], chunk_size: int = 1000):
        """
        Add documents to the RAG system.

        Args:
            documents: List of dicts with 'id', 'text', 'metadata'
            chunk_size: Size of chunks for splitting
        """
        print(f"Adding {len(documents)} documents to RAG system...")

        all_chunks = []
        all_ids = []
        all_metadatas = []

        for doc in documents:
            doc_id = doc["id"]
            text = doc["text"]
            metadata = doc.get("metadata", {})

            # Chunk the document
            chunks = semantic_chunk(text, max_chunk_size=chunk_size)
            print(f"  {doc_id}: {len(chunks)} chunks")

            # Create unique IDs for each chunk
            for i, chunk in enumerate(chunks):
                chunk_id = f"{doc_id}_chunk_{i}"
                all_ids.append(chunk_id)
                all_chunks.append(chunk)

                # Add chunk metadata
                chunk_metadata = metadata.copy()
                chunk_metadata.update({
                    "doc_id": doc_id,
                    "chunk_index": i,
                    "total_chunks": len(chunks)
                })
                all_metadatas.append(chunk_metadata)

        # Generate embeddings (batched for efficiency)
        print(f"Generating embeddings for {len(all_chunks)} chunks...")
        embeddings = self.embedding_model.encode(
            all_chunks,
            show_progress_bar=True,
            batch_size=32
        )

        # Add to Chroma
        print("Adding to Chroma database...")
        self.collection.add(
            ids=all_ids,
            documents=all_chunks,
            embeddings=embeddings.tolist(),
            metadatas=all_metadatas
        )

        print(f"Successfully added {len(all_chunks)} chunks!\n")

    def retrieve(
        self,
        query: str,
        top_k: int = 4,
        filter_metadata: Dict = None
    ) -> List[Dict]:
        """
        Retrieve relevant documents for a query.

        Args:
            query: User query
            top_k: Number of documents to retrieve
            filter_metadata: Optional metadata filters

        Returns:
            List of retrieved documents with metadata and scores
        """
        print(f"Retrieving documents for query: '{query}'")

        # Embed the query
        query_embedding = self.embedding_model.encode(query)

        # Search in Chroma
        results = self.collection.query(
            query_embeddings=[query_embedding.tolist()],
            n_results=top_k,
            where=filter_metadata  # Metadata filtering
        )

        # Format results
        retrieved_docs = []
        for i in range(len(results['ids'][0])):
            doc = {
                'id': results['ids'][0][i],
                'text': results['documents'][0][i],
                'metadata': results['metadatas'][0][i],
                'distance': results['distances'][0][i] if 'distances' in results else None
            }
            retrieved_docs.append(doc)
            print(f"  [{i+1}] {doc['id'][:30]}... (distance: {doc['distance']:.4f})")

        print()
        return retrieved_docs

    def generate_answer(
        self,
        query: str,
        retrieved_docs: List[Dict],
        model: str = "gpt-4"
    ) -> str:
        """
        Generate answer using LLM based on retrieved context.

        Args:
            query: User query
            retrieved_docs: Documents retrieved from vector DB
            model: OpenAI model to use

        Returns:
            Generated answer
        """
        if not self.openai_client:
            return "ERROR: No OpenAI API key configured"

        # Format context from retrieved documents
        context_parts = []
        for i, doc in enumerate(retrieved_docs, 1):
            context_parts.append(f"[Document {i}]\n{doc['text']}\n")
        context = "\n".join(context_parts)

        # Create prompt
        system_prompt = """You are a helpful AI assistant. Answer the user's question based ONLY on the provided context.

Rules:
1. If the context doesn't contain enough information, say "I don't have enough information to answer that."
2. Be concise but complete in your answers.
3. You can reference the document numbers when citing sources (e.g., "According to Document 1...").
4. Do not make up information beyond what's in the context."""

        user_prompt = f"""Context:
{context}

Question: {query}

Answer:"""

        print("Generating answer with GPT-4...")

        # Call OpenAI API
        try:
            response = self.openai_client.chat.completions.create(
                model=model,
                messages=[
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ],
                temperature=0.7,
                max_tokens=500
            )

            answer = response.choices[0].message.content
            return answer

        except Exception as e:
            return f"ERROR generating answer: {str(e)}"

    def query(
        self,
        query: str,
        top_k: int = 4,
        filter_metadata: Dict = None,
        generate: bool = True
    ) -> Dict:
        """
        Complete RAG pipeline: retrieve + generate.

        Args:
            query: User query
            top_k: Number of documents to retrieve
            filter_metadata: Optional metadata filters
            generate: Whether to generate answer with LLM

        Returns:
            Dict with retrieved_docs and answer (if generate=True)
        """
        # Step 1: Retrieve
        retrieved_docs = self.retrieve(query, top_k, filter_metadata)

        result = {"retrieved_docs": retrieved_docs}

        # Step 2: Generate (optional)
        if generate and self.openai_client:
            answer = self.generate_answer(query, retrieved_docs)
            result["answer"] = answer

        return result


# ============================================================================
# PART 4: Demo and Testing
# ============================================================================

def demo_rag_system():
    """
    Demonstrate the RAG system with sample queries.
    """
    print("=" * 80)
    print("RAG SYSTEM DEMO")
    print("=" * 80)
    print()

    # Initialize RAG system
    rag = SimpleRAG(
        embedding_model_name="sentence-transformers/all-mpnet-base-v2",
        persist_directory="./chroma_db_demo"
    )

    # Add sample documents
    rag.add_documents(SAMPLE_DOCUMENTS, chunk_size=1000)

    # Test queries
    test_queries = [
        "What is Python used for?",
        "Explain how RAG works",
        "What are vector databases?",
        "Tell me about LAIPath"
    ]

    print("=" * 80)
    print("TESTING RETRIEVAL (without generation)")
    print("=" * 80)
    print()

    for query in test_queries:
        print(f"\nQUERY: {query}")
        print("-" * 80)

        result = rag.query(query, top_k=3, generate=False)

        print("\nRetrieved Documents:")
        for i, doc in enumerate(result['retrieved_docs'], 1):
            print(f"\n[{i}] {doc['id']}")
            print(f"Category: {doc['metadata'].get('category', 'N/A')}")
            print(f"Distance: {doc['distance']:.4f}")
            print(f"Text Preview: {doc['text'][:200]}...")

        print("\n" + "=" * 80)

    # Test with generation (if API key is available)
    if rag.openai_client:
        print("\n\n" + "=" * 80)
        print("TESTING FULL RAG (with generation)")
        print("=" * 80)
        print()

        query = "What is RAG and why is it useful?"
        print(f"QUERY: {query}")
        print("-" * 80)

        result = rag.query(query, top_k=3, generate=True)

        print("\nGENERATED ANSWER:")
        print(result['answer'])
        print("\n" + "=" * 80)
    else:
        print("\n\nNOTE: Set OPENAI_API_KEY environment variable to test generation.")


def demo_retrieval_only():
    """
    Demo retrieval without OpenAI (works without API key).
    """
    print("=" * 80)
    print("RAG RETRIEVAL DEMO (No API Key Required)")
    print("=" * 80)
    print()

    # Initialize RAG system
    rag = SimpleRAG(persist_directory="./chroma_db_demo")

    # Add documents
    rag.add_documents(SAMPLE_DOCUMENTS)

    # Interactive query loop
    print("\n" + "=" * 80)
    print("INTERACTIVE RETRIEVAL")
    print("Type your questions (or 'quit' to exit)")
    print("=" * 80)

    while True:
        query = input("\nYour question: ").strip()

        if query.lower() in ['quit', 'exit', 'q']:
            print("Goodbye!")
            break

        if not query:
            continue

        print("\n" + "-" * 80)
        retrieved_docs = rag.retrieve(query, top_k=3)

        print("\nTop 3 Retrieved Chunks:\n")
        for i, doc in enumerate(retrieved_docs, 1):
            print(f"[{i}] {doc['id']}")
            print(f"    Distance: {doc['distance']:.4f}")
            print(f"    Category: {doc['metadata'].get('category', 'N/A')}")
            print(f"    Text: {doc['text'][:300]}...")
            print()


# ============================================================================
# PART 5: Advanced Examples
# ============================================================================

def demo_metadata_filtering():
    """
    Demonstrate metadata filtering in retrieval.
    """
    print("=" * 80)
    print("METADATA FILTERING DEMO")
    print("=" * 80)
    print()

    rag = SimpleRAG(persist_directory="./chroma_db_demo")
    rag.add_documents(SAMPLE_DOCUMENTS)

    query = "Tell me about AI"

    print(f"Query: {query}")
    print("\n1. Without filtering:")
    print("-" * 80)
    results = rag.retrieve(query, top_k=3)
    for doc in results:
        print(f"  - {doc['id']} (category: {doc['metadata']['category']})")

    print("\n2. Filtering by category='ai':")
    print("-" * 80)
    results = rag.retrieve(query, top_k=3, filter_metadata={"category": "ai"})
    for doc in results:
        print(f"  - {doc['id']} (category: {doc['metadata']['category']})")

    print("\n3. Filtering by category='programming':")
    print("-" * 80)
    results = rag.retrieve(query, top_k=3, filter_metadata={"category": "programming"})
    for doc in results:
        print(f"  - {doc['id']} (category: {doc['metadata']['category']})")


def demo_chunking_strategies():
    """
    Compare different chunking strategies.
    """
    sample_text = """
    Python is a programming language. It is widely used for web development, data science,
    and machine learning. Python has a simple syntax that is easy to learn.

    Machine learning is a subset of AI. It enables systems to learn from data. Common
    algorithms include neural networks, decision trees, and support vector machines.

    Deep learning uses neural networks with many layers. It excels at image recognition,
    natural language processing, and other complex tasks.
    """

    print("=" * 80)
    print("CHUNKING STRATEGIES COMPARISON")
    print("=" * 80)
    print()

    print("Original Text:")
    print(sample_text)
    print("\n" + "=" * 80)

    print("\n1. Fixed-size chunking (200 chars, 50 overlap):")
    print("-" * 80)
    chunks = chunk_text(sample_text, chunk_size=200, overlap=50)
    for i, chunk in enumerate(chunks, 1):
        print(f"[Chunk {i}] {chunk}\n")

    print("\n2. Semantic chunking (max 200 chars):")
    print("-" * 80)
    chunks = semantic_chunk(sample_text, max_chunk_size=200)
    for i, chunk in enumerate(chunks, 1):
        print(f"[Chunk {i}] {chunk}\n")


# ============================================================================
# MAIN
# ============================================================================

if __name__ == "__main__":
    import sys

    print("""
    ╔════════════════════════════════════════════════════════════════╗
    ║                    HANDS-ON RAG SYSTEM                         ║
    ║                  AI Engineer Bootcamp                          ║
    ╚════════════════════════════════════════════════════════════════╝

    Choose a demo to run:
    1. Full RAG demo (retrieval + generation) - Requires OpenAI API key
    2. Retrieval only (interactive) - No API key required
    3. Metadata filtering demo
    4. Chunking strategies comparison
    """)

    choice = input("Enter your choice (1-4): ").strip()

    if choice == "1":
        demo_rag_system()
    elif choice == "2":
        demo_retrieval_only()
    elif choice == "3":
        demo_metadata_filtering()
    elif choice == "4":
        demo_chunking_strategies()
    else:
        print("Invalid choice. Running demo 2 (retrieval only)...")
        demo_retrieval_only()


# ============================================================================
# EXERCISES FOR YOU
# ============================================================================

"""
EXERCISES TO TRY:

1. Add Your Own Documents:
   - Replace SAMPLE_DOCUMENTS with your own data
   - Try adding 10-20 documents on different topics
   - Test retrieval quality

2. Experiment with Chunk Sizes:
   - Try chunk_size=500, 1000, 2000
   - Compare retrieval quality
   - Which works best for your documents?

3. Implement Reranking:
   - Install: pip install sentence-transformers
   - Use CrossEncoder to rerank top 10 to top 3
   - Code hint:
     from sentence_transformers import CrossEncoder
     reranker = CrossEncoder('cross-encoder/ms-marco-MiniLM-L-6-v2')
     scores = reranker.predict([(query, doc) for doc in top_10])

4. Add Hybrid Search:
   - Combine vector search with keyword search (BM25)
   - Install: pip install rank_bm25
   - Weighted combination: 0.7 * vector + 0.3 * bm25

5. Build a Simple UI:
   - Use Gradio or Streamlit
   - Create a chat interface
   - Display retrieved sources

6. Production Enhancements:
   - Add caching for embeddings
   - Implement batch processing
   - Add error handling and retries
   - Monitor retrieval quality metrics

CONNECTION TO LAIPATH:
- The orchestration you built is similar to RAG pipeline
- Think about how you could add retrieval to LAIPath workflows
- How would you scale this system for production?

For interview: Run this code, understand each part, be ready to explain
the architecture and make improvements.
"""
