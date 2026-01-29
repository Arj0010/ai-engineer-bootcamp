# Interview Cheat Sheet

**Print this. Review right before the interview.**

---

## Your 2-Minute Pitch

> "I'm a BCA ML graduate focused on building production AI systems.
>
> Most recently, I built **LAIPath**, an AI-powered learning platform using LLM APIs with workflow orchestration and reflection mechanisms. It's currently in testing.
>
> I also built **PlanLift**, an AI product that converts 2D blueprints to 3D renders - full-stack, backend to model integration.
>
> I'm excited about Wednesday's focus on production AI. I've been building exactly that, and I'm expanding into **RAG and vector databases** - I actually built a RAG system this week in preparation."

---

## Key Terms to Use

| Say This | Not This |
|----------|----------|
| Workflow orchestration | I call APIs in order |
| Structured prompting | I write prompts |
| Reflection mechanism | System checks itself |
| State management | I save data |
| Production-ready | It works |
| Agentic patterns | The AI makes decisions |

---

## RAG Quick Reference

```
Documents → Chunk → Embed → Vector DB
                              ↓
Query → Embed → Retrieve Similar → LLM → Answer
```

**Components:**
- **Chunking**: RecursiveCharacterTextSplitter (500-1000 chars)
- **Embedding**: OpenAI embeddings, Sentence Transformers
- **Vector DB**: ChromaDB (prototype), FAISS (performance), Pinecone (scale)
- **Retrieval**: Top-k similarity search (k=3-5)

**When to use RAG:**
- Private/proprietary data
- Changing information
- Need source citations
- Reduce hallucinations

---

## LangChain Quick Reference

```python
# Chain = sequence of steps
chain = prompt | llm | parser

# Agent = LLM decides what to do
agent = create_react_agent(llm, tools, prompt)

# Memory = remember conversation
memory = ConversationBufferMemory()

# Tools = external functions
@tool
def search(query): ...
```

**LangGraph** = Graphs with loops and conditionals (more powerful than chains)

---

## Agentic AI Quick Reference

**What makes AI "agentic":**
1. Plan - break down tasks
2. Act - use tools
3. Observe - process results
4. Reason - decide next steps
5. Reflect - self-correct

**ReAct Pattern:**
```
Thought: [reasoning]
Action: [tool to use]
Observation: [result]
... repeat ...
Final Answer: [answer]
```

---

## Your Projects - Key Points

### LAIPath
- **What**: AI-powered adaptive learning platform
- **Stack**: Node.js, Express, LLM APIs
- **Key features**: Workflow orchestration, structured prompting, reflection
- **Status**: In testing with real users

### PlanLift
- **What**: 2D blueprint to 3D visualization
- **Stack**: Full-stack, external AI models
- **Key feature**: End-to-end ML pipeline

### RAG Project (Tonight)
- **What**: Portfolio Q&A system
- **Stack**: ChromaDB, OpenAI, LangChain
- **Demo-able**: Yes!

---

## Common Questions - Quick Answers

**"Built production AI systems?"**
> "Yes - LAIPath is in testing, handles real user workflows with LLM orchestration."

**"Experience with LLMs?"**
> "LAIPath uses structured prompting, workflow orchestration, reflection for quality control."

**"Vector databases?"**
> "Built a RAG system with ChromaDB this week. Understand FAISS, Pinecone trade-offs."

**"LangChain?"**
> "I built similar patterns manually in LAIPath. Understand chains, agents, memory concepts."

**"How do you learn new tech?"**
> "By building. I learned RAG this week by building a working system overnight."

---

## Numbers to Remember

| Metric | Value |
|--------|-------|
| Chunk size (typical) | 500-1000 characters |
| Chunk overlap | 50-100 characters |
| Top-k retrieval | 3-5 documents |
| Temperature (factual) | 0-0.3 |
| Temperature (creative) | 0.7-1.0 |
| Context window (GPT-4) | 128K tokens |
| Context window (GPT-3.5) | 16K tokens |

---

## If You Don't Know Something

**Don't say:** "I don't know."

**Say:**
> "I haven't used [X] in production, but it's similar to [Y] which I built. I could pick it up quickly - that's how I learned RAG this week."

---

## Questions to Ask Them

1. "What AI frameworks does your team use?"
2. "How do you handle LLM unpredictability in production?"
3. "What's your typical AI system lifecycle?"
4. "What vector databases are you using, and at what scale?"
5. "How does the team stay current with AI advancements?"

---

## Confidence Reminders

- You've built **production systems**, not just tutorials
- LAIPath is **real orchestration** with real users
- You learned RAG **overnight by building** - that's your superpower
- Most candidates have courses, you have **deployed code**

---

## Final Checklist

Before leaving:
- [ ] LAIPath architecture in your head
- [ ] RAG pipeline memorized
- [ ] 2-minute pitch practiced
- [ ] Questions for them ready
- [ ] GitHub repos accessible
- [ ] Confidence high

---

## The Meta-Story

If they ask about learning:
> "When I saw this role required RAG experience, I spent the night before building a RAG system from scratch. I learn by doing."

---

**You've got this. Go show them what you can build.**
