# Technical Questions Bank

## How to Use This Document

Practice answering these questions out loud. Each question has:
- A concise answer
- Key points to hit
- Connection to your experience

---

## RAG Questions

### Q1: What is RAG and when would you use it?

**Answer:**
RAG = Retrieval Augmented Generation. It combines retrieval from a knowledge base with LLM generation.

**When to use:**
1. LLM needs access to private/proprietary data
2. Information changes frequently (LLM's training is stale)
3. Need source attribution (where did this answer come from?)
4. Reduce hallucinations by grounding in real documents

**When NOT to use:**
1. General knowledge questions (LLM knows already)
2. Creative tasks (no retrieval needed)
3. Real-time requirements (retrieval adds latency)

**Your connection:**
> "I just built a RAG system called AskMyProjects. It answers questions about my portfolio by retrieving relevant chunks from my project documentation."

---

### Q2: Explain the RAG pipeline.

**Answer:**
```
1. INGEST: Documents → Chunk → Embed → Store in vector DB
2. QUERY: Question → Embed → Retrieve similar chunks
3. GENERATE: Question + Retrieved chunks → LLM → Answer
```

**Key points:**
- Chunking: Split docs into meaningful pieces (500-1000 chars)
- Embedding: Convert text to vectors (semantic meaning)
- Retrieval: Find top-k similar vectors
- Generation: LLM synthesizes answer from context

**Your connection:**
> "In my RAG project, I used RecursiveCharacterTextSplitter for chunking, OpenAI embeddings, ChromaDB for storage, and GPT for generation."

---

### Q3: How do you choose chunk size?

**Answer:**
Trade-off between context and precision:

| Chunk Size | Pros | Cons |
|------------|------|------|
| Small (200-500) | Precise retrieval | May miss context |
| Large (1000-2000) | More context | May retrieve irrelevant parts |

**Factors:**
1. Document type (code vs prose vs Q&A)
2. Query type (specific vs broad)
3. LLM context window
4. Retrieval precision needs

**Best practice:** Start with 500-1000, test with real queries, adjust.

---

### Q4: Compare vector databases: Chroma vs FAISS vs Pinecone

**Answer:**

| Feature | ChromaDB | FAISS | Pinecone |
|---------|----------|-------|----------|
| Type | Open source | Open source | Managed service |
| Best for | Prototyping, small scale | High performance, local | Production, scale |
| Setup | Easy | Moderate | Easy (managed) |
| Cost | Free | Free | Paid |
| Scale | 100K-1M vectors | Millions-billions | Billions |
| Features | Metadata filtering | Pure similarity | Full-featured |

**When to use each:**
- **Chroma**: Prototypes, small apps, learning
- **FAISS**: High performance, cost-sensitive, local deployment
- **Pinecone**: Production at scale, need managed service

**Your connection:**
> "I chose ChromaDB for my RAG project because it's easy to set up and perfect for demonstrating concepts. For production scale, I'd evaluate Pinecone for managed service or FAISS for performance."

---

### Q5: How do you improve RAG quality?

**Answer:**

**1. Better Retrieval:**
- Hybrid search (semantic + keyword)
- Query expansion (add related terms)
- Reranking (LLM reranks retrieved docs)
- Metadata filtering (narrow search space)

**2. Better Chunking:**
- Semantic chunking (preserve meaning)
- Overlapping chunks (don't lose context)
- Document hierarchy (sections, paragraphs)

**3. Better Generation:**
- Better prompts (clearer instructions)
- Citation requirements (cite sources)
- Multi-step reasoning (retrieve → think → retrieve more)

---

## LLM Questions

### Q6: How do you handle LLM unpredictability in production?

**Answer:**

**1. Structured Output:**
```python
# Force JSON output with schema
response = llm.invoke(
    "Return JSON with fields: answer, confidence, sources"
)
```

**2. Output Validation:**
```python
try:
    result = json.loads(response)
    validate_schema(result)
except:
    retry_with_clarified_prompt()
```

**3. Retry Logic:**
```python
for attempt in range(3):
    result = llm.invoke(prompt)
    if is_valid(result):
        return result
return fallback_response()
```

**4. Guardrails:**
- Input validation
- Output filtering
- Rate limiting
- Content moderation

**Your connection:**
> "In LAIPath, I implemented structured prompting with JSON schemas, validation, and retry logic. The reflection mechanism also catches low-quality outputs and triggers regeneration."

---

### Q7: What is prompt engineering? Give examples.

**Answer:**
Designing prompts to get desired LLM behavior.

**Techniques:**

1. **Role Setting:**
```
You are an expert Python programmer...
```

2. **Few-Shot Examples:**
```
Q: What is 2+2? A: 4
Q: What is 3+3? A: 6
Q: What is 5+5? A:
```

3. **Chain of Thought:**
```
Let's think step by step...
```

4. **Output Format:**
```
Return your answer as JSON with fields: answer, reasoning
```

5. **Constraints:**
```
Keep response under 100 words. Only use information from the provided context.
```

---

### Q8: Fine-tuning vs RAG vs Prompt Engineering - when to use each?

**Answer:**

| Approach | Use When | Pros | Cons |
|----------|----------|------|------|
| **Prompt Engineering** | Quick iteration, behavior changes | Fast, cheap, no training | Limited by context window |
| **RAG** | Need external knowledge, citations | No training, updatable | Retrieval latency, complexity |
| **Fine-tuning** | Change model behavior/style | Deep customization | Expensive, needs data, can degrade |

**Decision Flow:**
1. Can prompt engineering solve it? → Yes → Do that
2. Need external/changing knowledge? → Yes → Use RAG
3. Need fundamental behavior change? → Yes → Fine-tune

---

## LangChain Questions

### Q9: Explain LangChain's core concepts.

**Answer:**

1. **Chains**: Sequence of operations (prompt → LLM → parse)
2. **Agents**: LLM decides which tools to use
3. **Memory**: Store conversation history
4. **Tools**: Functions the LLM can call

**How they relate:**
```
Chain: A → B → C (fixed sequence)
Agent: LLM decides: A or B or C? (dynamic)
Memory: Remember past interactions
Tools: External capabilities (search, calculate, etc.)
```

**Your connection:**
> "I built these patterns manually in LAIPath. My orchestration engine is like Chains, my adaptive logic is like Agents, my progress tracking is like Memory. Understanding LangChain was quick because I knew the concepts."

---

### Q10: What is LangGraph?

**Answer:**
Library for building **stateful, cyclic workflows** with LLMs.

**vs LangChain Chains:**
- Chains: Linear (A → B → C)
- LangGraph: Graph with loops and conditionals

**Use when:**
1. Need loops (retry, refine, iterate)
2. Need conditionals (different paths based on state)
3. Complex agent workflows
4. Human-in-the-loop

**Core concepts:**
- **State**: Data flowing through graph
- **Nodes**: Functions that process state
- **Edges**: Connections (can be conditional)

---

## Agentic AI Questions

### Q11: What makes an AI system "agentic"?

**Answer:**
Ability to:
1. **Plan**: Break tasks into steps
2. **Act**: Use tools/APIs
3. **Observe**: Process results
4. **Reason**: Decide next steps
5. **Reflect**: Evaluate and self-correct

**Non-agentic:** "What's the weather?" → "I don't have that data"
**Agentic:** "What's the weather?" → [calls weather API] → "It's 72°F"

**Your connection:**
> "LAIPath demonstrates agentic patterns: it plans learning paths, uses LLM APIs as tools, tracks progress (observes), adapts based on context (reasons), and self-corrects through reflection."

---

### Q12: Explain the ReAct pattern.

**Answer:**
**ReAct = Reasoning + Acting**

```
Thought: I need to find X
Action: search("X")
Observation: [search results]
Thought: Now I know X, need Y
Action: calculate(Y)
Observation: [result]
Thought: I have the answer
Final Answer: [answer]
```

**Why it works:**
- Makes reasoning explicit
- Grounds LLM in real data
- Enables error recovery
- Interpretable (see the thinking)

---

### Q13: Multi-agent systems - when and why?

**Answer:**

**What:** Multiple specialized agents working together.

**Example - Research Team:**
- Researcher Agent: Finds information
- Writer Agent: Creates content
- Editor Agent: Reviews and refines

**When to use:**
1. Complex tasks needing diverse skills
2. Need verification (agents check each other)
3. Parallel processing possible
4. Domain specialization helps

**When NOT to use:**
1. Simple tasks (overkill)
2. Tight latency (multiple agents = slow)
3. Cost constraints (more API calls)

---

## Production AI Questions

### Q14: How would you deploy a RAG system in production?

**Answer:**

**Architecture:**
```
Load Balancer → API Servers → Cache → Vector DB
                    ↓
                 LLM API
```

**Key considerations:**

1. **Performance:**
- Cache frequent queries
- Async retrieval
- Batch embeddings

2. **Reliability:**
- Retry logic
- Fallback responses
- Rate limiting

3. **Monitoring:**
- Retrieval quality metrics
- Latency tracking
- Cost monitoring

4. **Scaling:**
- Horizontal API scaling
- Vector DB sharding
- LLM API rate management

---

### Q15: How do you evaluate RAG quality?

**Answer:**

**Retrieval Metrics:**
- **Recall@k**: Did we retrieve the right documents?
- **Precision@k**: Are retrieved docs relevant?
- **MRR**: Is the best doc ranked high?

**Generation Metrics:**
- **Faithfulness**: Does answer match retrieved context?
- **Answer relevance**: Does it answer the question?
- **Groundedness**: Can we trace to sources?

**Practical approach:**
1. Create test set of Q&A pairs
2. Run RAG on questions
3. Compare to expected answers
4. Human evaluation for quality

---

## Coding Questions

### Q16: Implement a simple RAG function

```python
def simple_rag(question: str, documents: List[str]) -> str:
    # 1. Embed documents
    doc_embeddings = [embed(doc) for doc in documents]

    # 2. Embed question
    question_embedding = embed(question)

    # 3. Find most similar documents
    similarities = [cosine_similarity(question_embedding, doc_emb)
                    for doc_emb in doc_embeddings]
    top_indices = sorted(range(len(similarities)),
                         key=lambda i: similarities[i],
                         reverse=True)[:3]

    # 4. Build context
    context = "\n".join([documents[i] for i in top_indices])

    # 5. Generate answer
    prompt = f"Context: {context}\n\nQuestion: {question}\n\nAnswer:"
    answer = llm.invoke(prompt)

    return answer
```

---

### Q17: Implement a simple agent with tool use

```python
def simple_agent(question: str, tools: dict) -> str:
    """Simple ReAct-style agent."""

    history = []
    max_iterations = 5

    for i in range(max_iterations):
        # Ask LLM what to do
        prompt = f"""Question: {question}

History: {history}

Available tools: {list(tools.keys())}

What should I do next? Respond with:
- Thought: [your reasoning]
- Action: [tool_name] or Final Answer
- Input: [input to tool] or [final answer]"""

        response = llm.invoke(prompt)

        # Parse response
        if "Final Answer" in response:
            return response.split("Final Answer:")[-1].strip()

        # Execute tool
        action = parse_action(response)
        tool_name = action["tool"]
        tool_input = action["input"]

        result = tools[tool_name](tool_input)
        history.append({"action": tool_name, "result": result})

    return "Could not find answer"
```

---

## System Design Questions

### Q18: Design a customer support chatbot with RAG

**Answer:**

```
┌─────────────────────────────────────────────────────────┐
│                  Customer Support Bot                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  User Query                                             │
│      ↓                                                  │
│  ┌──────────────┐                                      │
│  │ Intent       │ → "Order status" / "Product Q" /     │
│  │ Classifier   │   "Complaint" / "Other"              │
│  └──────────────┘                                      │
│      ↓                                                  │
│  ┌──────────────┐    ┌─────────────────┐              │
│  │ RAG Search   │───→│ Knowledge Base   │              │
│  │              │    │ (FAQ, Docs)      │              │
│  └──────────────┘    └─────────────────┘              │
│      ↓                                                  │
│  ┌──────────────┐    ┌─────────────────┐              │
│  │ Tool Router  │───→│ Order API       │              │
│  │              │    │ Ticket System   │              │
│  └──────────────┘    └─────────────────┘              │
│      ↓                                                  │
│  ┌──────────────┐                                      │
│  │ Response     │                                      │
│  │ Generator    │                                      │
│  └──────────────┘                                      │
│      ↓                                                  │
│  ┌──────────────┐                                      │
│  │ Guardrails   │ → No PII, polite, on-topic          │
│  └──────────────┘                                      │
│      ↓                                                  │
│  User Response                                          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**Key components:**
1. **Intent Classification**: Route query type
2. **RAG**: Search knowledge base
3. **Tools**: Check orders, create tickets
4. **Guardrails**: Safety, compliance

---

### Q19: How would you add RAG to LAIPath?

**Answer:**
> "I'd add a knowledge retrieval layer:
>
> 1. **Index course materials**: Chunk and embed learning resources
> 2. **User session history**: Store past interactions for context
> 3. **Query augmentation**: Before generating, retrieve relevant materials
> 4. **Personalized retrieval**: Filter by user's current learning path
>
> Architecture change:
> ```
> User Query → Retrieve from KB → LAIPath Orchestration → LLM → Response
> ```
>
> This would let LAIPath reference specific documentation when teaching, and remember what the user struggled with before."

---

## Quick-Fire Questions (One-Line Answers)

| Question | Answer |
|----------|--------|
| Embedding vs tokenization? | Tokenization = words to IDs. Embedding = words to vectors with meaning. |
| Cosine vs Euclidean distance? | Cosine = angle (direction). Euclidean = straight line. Cosine preferred for text. |
| Temperature in LLMs? | Controls randomness. 0 = deterministic. 1 = creative. |
| Zero-shot vs few-shot? | Zero = no examples. Few = give examples in prompt. |
| Hallucination? | LLM generating false but confident information. |
| Context window? | Max tokens LLM can process at once. |
| Streaming? | Return tokens as generated, not all at once. |
| Vector index types? | HNSW (fast, approx), IVF (clustered), Flat (exact, slow) |
