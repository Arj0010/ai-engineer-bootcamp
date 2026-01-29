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
from langchain_text_splitters import RecursiveCharacterTextSplitter
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
        self.llm = ChatOpenAI(
            model="gpt-3.5-turbo",
            temperature=0,
        )

        # Initialize the system
        self._load_and_index_documents()
        self._setup_qa_chain()
    def _load_documents(self) -> List[dict]:
        """Load all markdown documents from the documents directory."""
        documents = []
        
        # Check if directory exists
        if not self.documents_dir.exists():
            print(f"ERROR: Documents directory not found: {self.documents_dir}")
            print(f"Looking in: {self.documents_dir.absolute()}")
            return documents

        # Look for both .md and .txt files
        file_patterns = ["*.md", "*.txt"]
        all_files = []
        for pattern in file_patterns:
            all_files.extend(self.documents_dir.glob(pattern))
        
        if not all_files:
            print(f"ERROR: No .md or .txt files found in: {self.documents_dir.absolute()}")
            return documents

        for file_path in all_files:
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
        
        if not documents:
            print("\nWARNING: No documents loaded. Cannot create vector store.")
            print("Please add .md or .txt files to the documents directory.")
            return

        # Chunk documents
        chunks = self._chunk_documents(documents)
        
        if not chunks:
            print("\nWARNING: No chunks created. Cannot create vector store.")
            return

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
    
