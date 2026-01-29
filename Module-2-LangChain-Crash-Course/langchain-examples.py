"""
LangChain Practical Examples
=============================
Working examples of LangChain's core concepts.
Updated for LangChain v1.x

Requirements:
    pip install langchain langchain-core langchain-openai langchain-community chromadb

Author: AI Engineer Bootcamp
Duration: 30 minutes
"""

import os

# Check for API key first
if not os.getenv("OPENAI_API_KEY"):
    print("="*60)
    print("ERROR: OPENAI_API_KEY not set!")
    print("="*60)
    print("\nSet your API key first:")
    print("  Windows CMD: set OPENAI_API_KEY=your-key-here")
    print("  PowerShell:  $env:OPENAI_API_KEY='your-key-here'")
    print("\nThen run this script again.")
    exit(1)

# Updated imports for LangChain v1.x / 0.3+
from langchain_openai import ChatOpenAI, OpenAIEmbeddings
from langchain_core.prompts import PromptTemplate, ChatPromptTemplate, MessagesPlaceholder
from langchain_core.output_parsers import StrOutputParser
from langchain_core.runnables import RunnablePassthrough
from langchain_core.messages import HumanMessage, AIMessage
from langchain_community.vectorstores import Chroma
try:
    from langchain_text_splitters import RecursiveCharacterTextSplitter
    TEXT_SPLITTER_AVAILABLE = True
except (ImportError, AttributeError, ValueError, Exception) as e:
    # Handle scipy/numpy/sklearn compatibility issues
    TEXT_SPLITTER_AVAILABLE = False
    RecursiveCharacterTextSplitter = None
    print(f"Warning: Text splitter not available due to: {type(e).__name__}")
from langchain_core.documents import Document
from langchain_core.tools import tool
try:
    # Try new import location first (LangChain 1.2+)
    from langgraph.prebuilt import create_react_agent
    AGENT_AVAILABLE = True
except ImportError:
    AGENT_AVAILABLE = False
try:
    from langchain import hub
    HUB_AVAILABLE = True
except ImportError:
    HUB_AVAILABLE = False

# ============================================================================
# SETUP
# ============================================================================

# Initialize LLM
llm = ChatOpenAI(
    model="gpt-3.5-turbo",  # Using 3.5 for cost efficiency
    temperature=0.7
)

print("=" * 60)
print("LANGCHAIN EXAMPLES (Updated for v1.x)")
print("=" * 60)


# ============================================================================
# EXAMPLE 1: Simple Prompt Template and Chain (LCEL Style)
# ============================================================================

def example_1_simple_chain():
    """
    Demonstrates basic prompt template and LCEL chain.
    LCEL = LangChain Expression Language (modern approach)
    """
    print("\n" + "=" * 60)
    print("EXAMPLE 1: Simple Prompt Template and Chain (LCEL)")
    print("=" * 60)

    # Create prompt template
    template = PromptTemplate(
        input_variables=["technology"],
        template="""You are a technical interviewer.

Ask 3 insightful interview questions about {technology} for a senior engineer position.

Format your response as:
1. [Question 1]
2. [Question 2]
3. [Question 3]
"""
    )

    # Modern LCEL chain: prompt | llm | parser
    chain = template | llm | StrOutputParser()

    # Run chain
    print("\nRunning chain for 'RAG systems'...")
    result = chain.invoke({"technology": "RAG systems"})
    print("\nResult:")
    print(result)

    # Another example
    print("\n" + "-" * 60)
    print("Running chain for 'Vector Databases'...")
    print("-" * 60)

    result = chain.invoke({"technology": "Vector Databases"})
    print("\nResult:")
    print(result)


# ============================================================================
# EXAMPLE 2: Sequential Chain (LCEL Style)
# ============================================================================

def example_2_sequential_chain():
    """
    Demonstrates chaining multiple LLM calls sequentially using LCEL.
    """
    print("\n" + "=" * 60)
    print("EXAMPLE 2: Sequential Chain (LCEL)")
    print("=" * 60)

    # Chain 1: Generate Python code
    code_prompt = PromptTemplate(
        input_variables=["task"],
        template="Write clean Python code for this task: {task}\n\nCode:"
    )

    # Chain 2: Explain the code
    explain_prompt = PromptTemplate(
        input_variables=["code"],
        template="""Explain this code in 2-3 sentences for a technical interviewer:

{code}

Explanation:"""
    )

    # Create individual chains
    code_chain = code_prompt | llm | StrOutputParser()
    explain_chain = explain_prompt | llm | StrOutputParser()

    # Sequential execution
    print("\nCreating sequential chain: Code Generation → Explanation")

    task = "implement a binary search function"
    print(f"\nTask: {task}")
    print("\nStep 1: Generating code...")

    # Run first chain
    code_result = code_chain.invoke({"task": task})
    print("\nGenerated Code:")
    print(code_result)

    # Run second chain with output from first
    print("\nStep 2: Explaining code...")
    explanation = explain_chain.invoke({"code": code_result})

    print("\n" + "=" * 60)
    print("EXPLANATION:")
    print("=" * 60)
    print(explanation)


# ============================================================================
# EXAMPLE 3: Conversation with Memory
# ============================================================================

def example_3_memory():
    """
    Demonstrates conversation memory for context retention.
    Using modern LCEL approach (no deprecated ConversationChain).
    """
    print("\n" + "=" * 60)
    print("EXAMPLE 3: Conversation with Memory")
    print("=" * 60)

    # Manual message history (modern approach)
    chat_history = []

    # Create a simple chat prompt with history
    prompt = ChatPromptTemplate.from_messages([
        ("system", "You are a helpful AI assistant."),
        MessagesPlaceholder(variable_name="chat_history"),
        ("human", "{input}")
    ])

    # Create chain
    chain = prompt | llm | StrOutputParser()

    print("\n" + "-" * 60)
    print("Multi-turn conversation:")
    print("-" * 60)

    # Turn 1
    user_input_1 = "I'm preparing for an AI Engineer interview."
    print(f"\n[USER]: {user_input_1}")
    
    response1 = chain.invoke({
        "chat_history": chat_history,
        "input": user_input_1
    })
    print(f"\n[ASSISTANT]: {response1}")
    
    # Add to history
    chat_history.append(HumanMessage(content=user_input_1))
    chat_history.append(AIMessage(content=response1))

    # Turn 2 - LLM remembers context
    user_input_2 = "What should I focus on for RAG systems?"
    print(f"\n[USER]: {user_input_2}")
    
    response2 = chain.invoke({
        "chat_history": chat_history,
        "input": user_input_2
    })
    print(f"\n[ASSISTANT]: {response2}")
    
    # Add to history
    chat_history.append(HumanMessage(content=user_input_2))
    chat_history.append(AIMessage(content=response2))

    # Turn 3 - LLM remembers both previous turns
    user_input_3 = "I've built a project called LAIPath with workflow orchestration. How can I talk about it?"
    print(f"\n[USER]: I've built a project called LAIPath. How can I talk about it?")
    
    response3 = chain.invoke({
        "chat_history": chat_history,
        "input": user_input_3
    })
    print(f"\n[ASSISTANT]: {response3}")

    # Show memory
    print("\n" + "=" * 60)
    print("MEMORY BUFFER (what the LLM remembers):")
    print("=" * 60)
    for i, msg in enumerate(chat_history):
        role = "Human" if isinstance(msg, HumanMessage) else "AI"
        print(f"{i+1}. [{role}]: {msg.content[:100]}...")


# ============================================================================
# EXAMPLE 4: Agents with Tools
# ============================================================================

def example_4_agents():
    """
    Demonstrates agents that can use tools dynamically.
    """
    print("\n" + "=" * 60)
    print("EXAMPLE 4: Agents with Tools")
    print("=" * 60)

    if not AGENT_AVAILABLE:
        print("\n⚠️  Agent functionality requires 'langgraph' package.")
        print("Install with: pip install langgraph")
        print("Skipping agent example...\n")
        return

    # Define tools using @tool decorator
    @tool
    def calculator(expression: str) -> str:
        """Evaluate a mathematical expression. Input should be a valid Python math expression like '2+2' or '25*17'."""
        try:
            # Only allow safe math operations
            allowed_chars = set('0123456789+-*/(). ')
            if not all(c in allowed_chars for c in expression):
                return "Error: Invalid characters in expression"
            result = eval(expression)
            return f"Result: {result}"
        except Exception as e:
            return f"Error: {str(e)}"

    @tool
    def word_counter(text: str) -> str:
        """Count the number of words in the given text."""
        words = text.split()
        return f"Word count: {len(words)}"

    @tool
    def string_reverser(text: str) -> str:
        """Reverse the given string."""
        return f"Reversed: {text[::-1]}"

    # Create list of tools
    tools = [calculator, word_counter, string_reverser]

    # Create agent using modern langgraph API
    print("\nCreating React agent with tools...")
    agent_executor = create_react_agent(llm, tools)

    # Test queries
    test_queries = [
        "What is 25 multiplied by 17?",
        "How many words are in: 'LangChain makes building LLM apps easier'",
        "Reverse the string 'LAIPath'"
    ]

    for query in test_queries:
        print("\n" + "=" * 60)
        print(f"QUERY: {query}")
        print("=" * 60)

        try:
            # Invoke the agent
            result = agent_executor.invoke({"messages": [("user", query)]})
            
            print("\n" + "-" * 40)
            print("AGENT'S ANSWER:")
            print("-" * 40)
            # Extract the final message
            if "messages" in result and len(result["messages"]) > 0:
                final_message = result["messages"][-1]
                print(final_message.content)
            else:
                print(result)
        except Exception as e:
            print(f"Error: {str(e)}")


# ============================================================================
# EXAMPLE 5: Simple RAG with LangChain
# ============================================================================

def example_5_rag_simple():
    """
    Demonstrates a simple RAG setup with LangChain.
    """
    print("\n" + "=" * 60)
    print("EXAMPLE 5: Simple RAG with LangChain")
    print("=" * 60)

    if not TEXT_SPLITTER_AVAILABLE:
        print("\n⚠️  RAG example requires text splitter (scipy/numpy compatibility issue).")
        print("To fix, run: pip install --upgrade scipy numpy")
        print("Skipping RAG example...\n")
        return

    # Sample documents about your projects
    documents_text = [
        """LAIPath is a production AI system with workflow orchestration.
        It integrates multiple LLM APIs and implements adaptive logic for dynamic decision-making.
        The system demonstrates production-ready error handling and monitoring.""",

        """PlanLift is a full-stack AI MVP that showcases end-to-end AI model integration.
        It combines frontend, backend, and AI components into a cohesive application.
        PlanLift converts 2D blueprints to 3D visualizations using AI.""",

        """RAG (Retrieval-Augmented Generation) combines retrieval from knowledge bases with
        LLM generation. It's useful for grounding LLM responses in factual data.
        RAG systems use vector databases to store and retrieve relevant documents."""
    ]

    # Create Document objects
    docs = [Document(page_content=text) for text in documents_text]

    # Split into chunks
    print("\nSplitting documents into chunks...")
    text_splitter = RecursiveCharacterTextSplitter(
        chunk_size=200,
        chunk_overlap=50
    )
    chunks = text_splitter.split_documents(docs)
    print(f"Created {len(chunks)} chunks")

    # Create embeddings and vector store
    print("\nCreating vector store with OpenAI embeddings...")
    embeddings = OpenAIEmbeddings()
    vectorstore = Chroma.from_documents(
        documents=chunks,
        embedding=embeddings,
        collection_name="demo_rag"
    )

    # Create retriever
    retriever = vectorstore.as_retriever(search_kwargs={"k": 2})

    # Create RAG chain using LCEL
    template = """Answer the question based only on the following context:

Context:
{context}

Question: {question}

Answer:"""

    prompt = ChatPromptTemplate.from_template(template)

    def format_docs(docs):
        return "\n\n".join(doc.page_content for doc in docs)

    # RAG chain
    rag_chain = (
        {"context": retriever | format_docs, "question": RunnablePassthrough()}
        | prompt
        | llm
        | StrOutputParser()
    )

    # Test queries
    queries = [
        "What is LAIPath?",
        "Tell me about RAG"
    ]

    for query in queries:
        print("\n" + "=" * 60)
        print(f"QUERY: {query}")
        print("=" * 60)

        answer = rag_chain.invoke(query)

        print("\nANSWER:")
        print(answer)

        # Show retrieved docs
        print("\nRETRIEVED DOCUMENTS:")
        retrieved_docs = retriever.invoke(query)
        for i, doc in enumerate(retrieved_docs, 1):
            print(f"\n[{i}] {doc.page_content[:100]}...")


# ============================================================================
# MAIN - Run All Examples
# ============================================================================

def main():
    """Run all examples with menu."""

    print("\n")
    print("=" * 60)
    print("   LANGCHAIN PRACTICAL EXAMPLES")
    print("   AI Engineer Bootcamp - Module 2")
    print("=" * 60)
    print("\n")

    examples = [
        ("1", "Simple Chain (LCEL)", example_1_simple_chain),
        ("2", "Sequential Chain", example_2_sequential_chain),
        ("3", "Conversation Memory", example_3_memory),
        ("4", "Agents with Tools", example_4_agents),
        ("5", "Simple RAG", example_5_rag_simple),
    ]

    print("Available examples:")
    for num, name, _ in examples:
        print(f"  {num}. {name}")
    print("  a. Run ALL examples")
    print("  q. Quit")

    while True:
        choice = input("\nEnter choice (1-5, 'a' for all, 'q' to quit): ").strip().lower()

        if choice == 'q':
            print("Goodbye!")
            break
        elif choice == 'a':
            for num, name, func in examples:
                try:
                    func()
                    if num != "5":  # Don't pause after last
                        input("\nPress Enter for next example...")
                except Exception as e:
                    print(f"Error in {name}: {e}")
            print("\n" + "=" * 60)
            print("ALL EXAMPLES COMPLETED!")
            print("=" * 60)
            break
        elif choice in [e[0] for e in examples]:
            for num, name, func in examples:
                if num == choice:
                    try:
                        func()
                    except Exception as e:
                        print(f"Error: {e}")
                    break
        else:
            print("Invalid choice. Enter 1-5, 'a', or 'q'.")

    print("\n" + "=" * 60)
    print("KEY TAKEAWAYS:")
    print("=" * 60)
    print("1. LCEL (|) is the modern way to chain components")
    print("2. Prompt | LLM | Parser = basic chain pattern")
    print("3. Memory enables multi-turn conversations")
    print("4. Agents dynamically choose which tools to use")
    print("5. RAG = Retrieve relevant docs → Feed to LLM")
    print("\nYour LAIPath maps to these patterns:")
    print("  - Your orchestration = LangChain chains")
    print("  - Your progress tracking = LangChain memory")
    print("  - Your adaptive logic = LangChain agents")
    print("=" * 60)


if __name__ == "__main__":
    main()
