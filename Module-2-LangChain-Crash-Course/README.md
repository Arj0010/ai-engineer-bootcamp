# Module 2: LangChain Crash Course

**Duration:** 30-60 minutes
**Focus:** Understanding LangChain's core concepts and how they map to production AI systems

## Learning Objectives
- Understand LangChain's core abstractions
- Map LangChain concepts to your LAIPath workflows
- Recognize when to use LangChain vs custom implepip install -r requirements.txtmentations
- Be interview-ready on LangChain questions

---

## 1. What is LangChain? (10 mins)

### Definition

**LangChain** is a framework for building applications with large language models. It provides abstractions and tools for common LLM patterns.

### Core Philosophy

```
Instead of writing custom code for:
- Prompt templates
- Chaining multiple LLM calls
- Memory management
- Tool/function calling
- Document loading and processing

LangChain provides pre-built components.
```

### When to Use LangChain

| Use LangChain | Build Custom (like LAIPath) |
|---------------|----------------------------|
| Rapid prototyping | Specific production requirements |
| Standard RAG patterns | Custom orchestration logic |
| Quick demos/MVPs | Performance-critical systems |
| Learning LLM patterns | Full control needed |
| Common use cases | Novel architectures |

**Your LAIPath Context:**
You built custom orchestration, which shows strong engineering skills. LangChain would have accelerated development but your custom approach gave you deeper understanding and control.

---

## 2. LangChain Core Concepts (40 mins)

### 2.1 Models and Prompts

#### **Models (LLMs)**

```python
from langchain_openai import ChatOpenAI
from langchain_anthropic import ChatAnthropic

# Initialize LLM
llm = ChatOpenAI(model="gpt-4", temperature=0.7)

# Simple invocation
response = llm.invoke("What is Python?")
print(response.content)
```

**Supported Model Types:**
- `ChatOpenAI` - OpenAI's chat models (GPT-4, GPT-3.5)
- `ChatAnthropic` - Anthropic's Claude
- `HuggingFaceHub` - Open source models
- `Ollama` - Local LLMs

---

#### **Prompt Templates**

```python
from langchain.prompts import PromptTemplate, ChatPromptTemplate

# Simple template
template = PromptTemplate(
    input_variables=["product"],
    template="Write a tagline for a company that makes {product}."
)

prompt = template.format(product="AI-powered code editors")
# Output: "Write a tagline for a company that makes AI-powered code editors."

# Chat template (for chat models)
chat_template = ChatPromptTemplate.from_messages([
    ("system", "You are a helpful coding assistant."),
    ("human", "Explain {concept} in Python."),
])

messages = chat_template.format_messages(concept="decorators")
```

**Why Templates?**
- Reusability: Define once, use many times
- Maintainability: Change prompts without changing code
- Testing: Easier to test with different inputs

**LAIPath Connection:**
If LAIPath uses LLM APIs, you likely have strings or functions that format prompts. LangChain templates would formalize this pattern.

---

### 2.2 Chains

**Chains** combine multiple components in a sequence.

#### **Simple Chain**

```python
from langchain.chains import LLMChain

# Create chain
chain = LLMChain(llm=llm, prompt=template)

# Run chain
result = chain.run(product="AI interview prep tools")
print(result)
```

#### **Sequential Chain**

```python
from langchain.chains import SimpleSequentialChain

# Chain 1: Generate code
code_template = PromptTemplate(
    input_variables=["task"],
    template="Write Python code for: {task}"
)
code_chain = LLMChain(llm=llm, prompt=code_template)

# Chain 2: Explain code
explain_template = PromptTemplate(
    input_variables=["code"],
    template="Explain this code:\n{code}"
)
explain_chain = LLMChain(llm=llm, prompt=explain_template)

# Combine chains
full_chain = SimpleSequentialChain(
    chains=[code_chain, explain_chain],
    verbose=True
)

result = full_chain.run("sort a list")
# Output of code_chain → Input to explain_chain
```

#### **LCEL (LangChain Expression Language)**

Modern LangChain uses pipes (`|`) for chaining:

```python
# New style (LCEL)
chain = prompt | llm | output_parser

result = chain.invoke({"product": "AI tools"})
```

**LAIPath Connection:**
Your workflow orchestration is essentially a custom chain implementation. LangChain chains formalize this pattern with error handling, retries, and logging built-in.

---

### 2.3 Memory

**Memory** allows LLMs to remember conversation context.

#### **Conversation Buffer Memory**

```python
from langchain.memory import ConversationBufferMemory
from langchain.chains import ConversationChain

# Initialize memory
memory = ConversationBufferMemory()

# Create conversation chain
conversation = ConversationChain(
    llm=llm,
    memory=memory,
    verbose=True
)

# Multi-turn conversation
print(conversation.predict(input="Hi, I'm working on a RAG system."))
# LLM remembers this context

print(conversation.predict(input="What vector database should I use?"))
# LLM knows you're asking about RAG (from previous message)

# View memory
print(memory.buffer)
```

#### **Memory Types**

| Memory Type | Description | Use Case |
|------------|-------------|----------|
| `ConversationBufferMemory` | Stores all messages | Short conversations |
| `ConversationBufferWindowMemory` | Stores last K messages | Limit context size |
| `ConversationSummaryMemory` | Summarizes old messages | Long conversations |
| `ConversationKGMemory` | Extracts knowledge graph | Structured memory |

#### **Example: Window Memory**

```python
from langchain.memory import ConversationBufferWindowMemory

memory = ConversationBufferWindowMemory(k=2)  # Keep last 2 exchanges

# After 3 exchanges, first one is forgotten
```

**Production Consideration:**
- Memory grows linearly with conversation length
- For production, use summary or window memory
- Or store in external database (Redis, PostgreSQL)

---

### 2.4 Agents and Tools

**Agents** can use tools to accomplish tasks. They decide which tools to use and in what order.

#### **Defining Tools**

```python
from langchain.agents import tool

@tool
def get_current_temperature(location: str) -> str:
    """Get current temperature for a location."""
    # In production, call weather API
    return f"The temperature in {location} is 72°F"

@tool
def search_wikipedia(query: str) -> str:
    """Search Wikipedia for information."""
    # In production, call Wikipedia API
    return f"Wikipedia results for: {query}"
```

#### **Creating an Agent**

```python
from langchain.agents import create_openai_functions_agent, AgentExecutor
from langchain import hub

# Get a prompt from LangChain hub
prompt = hub.pull("hwchase17/openai-functions-agent")

# Define tools
tools = [get_current_temperature, search_wikipedia]

# Create agent
agent = create_openai_functions_agent(llm, tools, prompt)

# Create executor
agent_executor = AgentExecutor(
    agent=agent,
    tools=tools,
    verbose=True
)

# Run agent
result = agent_executor.invoke({
    "input": "What's the temperature in San Francisco?"
})
```

**How it works:**
1. Agent receives question
2. Decides which tool to use (using LLM reasoning)
3. Calls tool with appropriate arguments
4. Uses tool output to answer question

#### **ReAct Pattern**

Agents often use the **ReAct** pattern (Reason + Act):

```
Thought: I need to find the temperature in San Francisco
Action: get_current_temperature
Action Input: "San Francisco"
Observation: The temperature in San Francisco is 72°F
Thought: I have the answer
Final Answer: The temperature in San Francisco is 72°F
```

**LAIPath Connection:**
If LAIPath has adaptive logic that chooses different actions based on context, that's similar to an agent. LangChain agents formalize this with built-in reasoning.

---

### 2.5 RAG with LangChain

LangChain makes RAG simple:

```python
from langchain.document_loaders import TextLoader
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain.embeddings import OpenAIEmbeddings
from langchain.vectorstores import Chroma
from langchain.chains import RetrievalQA

# 1. Load documents
loader = TextLoader("data.txt")
documents = loader.load()

# 2. Split into chunks
text_splitter = RecursiveCharacterTextSplitter(
    chunk_size=1000,
    chunk_overlap=200
)
chunks = text_splitter.split_documents(documents)

# 3. Create embeddings and vector store
embeddings = OpenAIEmbeddings()
vectorstore = Chroma.from_documents(chunks, embeddings)

# 4. Create retrieval QA chain
qa_chain = RetrievalQA.from_chain_type(
    llm=llm,
    chain_type="stuff",  # How to combine retrieved docs
    retriever=vectorstore.as_retriever(search_kwargs={"k": 4})
)

# 5. Ask questions
answer = qa_chain.run("What is the main topic of the document?")
```

**Chain Types:**
- `stuff`: Put all docs in one prompt (simple, works for small context)
- `map_reduce`: Summarize each doc separately, then combine
- `refine`: Iteratively refine answer with each doc
- `map_rerank`: Score each doc, use highest score

---

## 3. LAIPath vs LangChain Mapping (10 mins)

### Your LAIPath System → LangChain Equivalents

| LAIPath Feature | LangChain Equivalent | Notes |
|----------------|---------------------|-------|
| Workflow orchestration | Chains / LCEL | Sequential execution |
| LLM API calls | Models (`ChatOpenAI`) | Abstraction over APIs |
| Adaptive logic | Agents | Tool use and reasoning |
| Context management | Memory | Conversation history |
| Custom prompts | PromptTemplate | Reusable templates |

### Interview Story

**If asked: "Have you used LangChain?"**

Option 1 (Honest and Strong):
> "I haven't used LangChain in production, but I understand its abstractions deeply because I've built similar patterns from scratch in LAIPath. For example, LAIPath's workflow orchestration is essentially what LangChain calls 'Chains,' and my adaptive logic maps to LangChain's 'Agents.' Building from scratch gave me deeper understanding of the underlying patterns, but I recognize LangChain's value for rapid development and standardization."

Option 2 (If you run the examples):
> "Yes, I'm familiar with LangChain. I've used it for prototyping RAG systems and understand its core abstractions - Chains for orchestration, Agents for tool use, and Memory for conversation context. I've also built similar patterns from scratch in LAIPath, which helps me understand when to use LangChain versus when custom implementations are better."

### When You Would Choose Each

**Choose LangChain when:**
- Building standard RAG, chatbots, Q&A systems
- Rapid prototyping and MVPs
- Team needs common abstractions
- Leveraging community integrations

**Choose Custom (like LAIPath) when:**
- Novel architectures not covered by LangChain
- Performance-critical (LangChain adds overhead)
- Need full control over execution
- Specific production requirements (custom retry logic, monitoring)

---

## 4. Interview Questions on LangChain

### Q1: What is LangChain and what problems does it solve?
<details>
<summary>Answer</summary>

LangChain is a framework for building LLM applications. It solves:

1. **Abstraction**: Provides common patterns (chains, agents, memory) so you don't reinvent the wheel
2. **Integration**: Supports 50+ LLM providers, vector databases, tools
3. **Rapid Development**: Pre-built components for common use cases (RAG, chatbots, agents)
4. **Maintainability**: Standardized patterns make code easier to understand and maintain

It's particularly useful for prototyping and building standard LLM applications quickly. For custom production systems, you might build from scratch for more control (like I did with LAIPath).
</details>

---

### Q2: Explain LangChain Chains with an example.
<details>
<summary>Answer</summary>

Chains combine multiple components sequentially. Example:

```python
# Chain 1: Translate to Spanish
translate_prompt = PromptTemplate(
    input_variables=["text"],
    template="Translate to Spanish: {text}"
)
translate_chain = LLMChain(llm=llm, prompt=translate_prompt)

# Chain 2: Make it formal
formal_prompt = PromptTemplate(
    input_variables=["text"],
    template="Make this more formal: {text}"
)
formal_chain = LLMChain(llm=llm, prompt=formal_prompt)

# Combine
full_chain = SimpleSequentialChain(
    chains=[translate_chain, formal_chain]
)

result = full_chain.run("Hello, how are you?")
# 1. Translates to Spanish: "Hola, ¿cómo estás?"
# 2. Makes formal: "Hola, ¿cómo se encuentra usted?"
```

The output of one chain becomes input to the next. This is useful for multi-step workflows.
</details>

---

### Q3: What are LangChain Agents and how do they differ from Chains?
<details>
<summary>Answer</summary>

**Chains**: Pre-defined sequence (A → B → C)
**Agents**: Dynamic tool selection based on task

Example:
```python
User: "What's 25 * 17 + weather in NYC?"

# Agent reasoning:
Thought: Need to do math and get weather
Action: Use calculator for 25 * 17
Observation: 425
Action: Use weather API for NYC
Observation: 68°F, sunny
Final Answer: 425, and weather in NYC is 68°F, sunny
```

Agents use the **ReAct pattern** (Reasoning + Acting) to:
1. Analyze the question
2. Choose appropriate tool
3. Execute tool
4. Reason about result
5. Repeat or provide final answer

Chains are deterministic, agents are dynamic.
</details>

---

### Q4: How does LangChain handle memory in conversations?
<details>
<summary>Answer</summary>

LangChain provides multiple memory types:

1. **ConversationBufferMemory**: Stores all messages
   - Good for: Short conversations
   - Issue: Grows unbounded

2. **ConversationBufferWindowMemory**: Stores last K messages
   - Good for: Limiting context size
   - Issue: Loses older context

3. **ConversationSummaryMemory**: Summarizes old messages
   - Good for: Long conversations
   - Issue: Loses detail

4. **ConversationKGMemory**: Extracts entities and relationships
   - Good for: Structured information retention

Example:
```python
memory = ConversationBufferMemory()
conversation = ConversationChain(llm=llm, memory=memory)

conversation.predict(input="I'm building a RAG system")
# Later...
conversation.predict(input="What database should I use?")
# LLM knows you're asking about RAG from previous context
```

For production, I'd use summary memory or store in external database (Redis) for persistence and scalability.
</details>

---

### Q5: What is LCEL and why was it introduced?
<details>
<summary>Answer</summary>

**LCEL (LangChain Expression Language)** is the modern way to build chains using pipes:

```python
# Old way
chain = LLMChain(llm=llm, prompt=prompt)
result = chain.run(input_data)

# New way (LCEL)
chain = prompt | llm | output_parser
result = chain.invoke(input_data)
```

**Why LCEL?**
1. **Simpler syntax**: More intuitive pipe operator
2. **Better composition**: Easy to combine components
3. **Streaming support**: Built-in streaming
4. **Async support**: Native async/await
5. **Standardized interface**: All components use invoke/stream/batch

Example with streaming:
```python
chain = prompt | llm

for chunk in chain.stream({"topic": "RAG"}):
    print(chunk.content, end="")
```

LCEL is now the recommended way to build LangChain applications.
</details>

---

### Q6: How would you build a RAG system with LangChain?
<details>
<summary>Answer</summary>

```python
from langchain.document_loaders import PyPDFLoader
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain.embeddings import OpenAIEmbeddings
from langchain.vectorstores import Chroma
from langchain.chains import RetrievalQA
from langchain_openai import ChatOpenAI

# 1. Load documents
loader = PyPDFLoader("documentation.pdf")
documents = loader.load()

# 2. Chunk documents
splitter = RecursiveCharacterTextSplitter(
    chunk_size=1000,
    chunk_overlap=200
)
chunks = splitter.split_documents(documents)

# 3. Create vector store
embeddings = OpenAIEmbeddings()
vectorstore = Chroma.from_documents(
    documents=chunks,
    embedding=embeddings,
    persist_directory="./chroma_db"
)

# 4. Create retrieval chain
llm = ChatOpenAI(model="gpt-4")
qa_chain = RetrievalQA.from_chain_type(
    llm=llm,
    retriever=vectorstore.as_retriever(search_kwargs={"k": 4}),
    chain_type="stuff",  # Combine all docs in one prompt
    return_source_documents=True
)

# 5. Query
result = qa_chain.invoke({"query": "What is the main topic?"})
print(result["result"])
print(result["source_documents"])  # See sources
```

For production, I'd add:
- Error handling and retries
- Caching for repeated queries
- Monitoring for retrieval quality
- Custom prompts for better answers
</details>

---

### Q7: Compare LangChain to building custom solutions.
<details>
<summary>Answer</summary>

**LangChain Pros:**
- Fast prototyping (pre-built components)
- Community integrations (50+ LLMs, databases)
- Standardized patterns (easier for teams)
- Built-in features (retries, streaming, async)

**LangChain Cons:**
- Abstraction overhead (slower than direct API calls)
- Learning curve for the framework
- Less control over execution
- Can be overkill for simple use cases

**Custom (like LAIPath) Pros:**
- Full control over implementation
- Optimized performance
- Exactly what you need (no bloat)
- Deeper understanding of patterns

**Custom Cons:**
- Slower initial development
- Need to implement common features (retries, logging)
- Maintenance burden

**My Approach:**
- Prototype with LangChain to validate idea
- If going to production with complex requirements, consider custom
- For standard use cases (RAG, chatbots), LangChain is great
- For novel architectures (like LAIPath's adaptive logic), custom might be better

The fact that you built LAIPath custom shows strong engineering. Now understanding LangChain gives you both perspectives.
</details>

---

## 5. Practical Examples

See `langchain-examples.py` for working code demonstrating:
1. Simple chain (prompt → LLM)
2. Conversation with memory
3. Agent with tools
4. RAG with LangChain

Run these examples to solidify your understanding.

---

## 6. Key Takeaways

1. **LangChain is an abstraction framework** for common LLM patterns
2. **Core concepts**: Models, Prompts, Chains, Memory, Agents, Tools
3. **LCEL** is the modern way to build chains (pipe operator)
4. **Your LAIPath** shows you understand these patterns deeply (you built them!)
5. **Use LangChain for**: Rapid prototyping, standard patterns, team standardization
6. **Use custom for**: Novel architectures, performance, full control

---

## 7. Interview Prep Strategy

**For tomorrow:**
1. Run `langchain-examples.py` (15 mins)
2. Review these Q&A (15 mins)
3. Practice explaining how LAIPath maps to LangChain concepts (10 mins)

**Key message for interview:**
> "I've built production AI systems from scratch (LAIPath), which means I understand the patterns that frameworks like LangChain abstract. I can work with LangChain for rapid development or build custom solutions when needed. Having built workflows, agents, and orchestration from scratch gives me a deeper understanding than just using the framework."

This positions you as someone who:
- Understands fundamentals (built from scratch)
- Can use modern tools (knows LangChain)
- Makes informed decisions (when to use each)

---

## Next Steps

1. Complete `langchain-examples.py` (30 mins)
2. Review LangChain interview questions (15 mins)
3. Move to Module 3: Agentic AI (more advanced agent patterns)

Time spent on Module 2: ~1 hour
Remaining tonight: Move to Module 3 or rest for tomorrow

Good luck!
