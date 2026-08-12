# LLM Systems — Revision Layer

**Purpose:** you already have the reference material (Modules 1–6). This is the layer on
top of it, built for the specific thing your interviewer said they want to see — **how you
would use these, the deciding factors, and your thought process.**

Reference material answers *"what is LangGraph?"*
This answers *"why did you choose it, and when would you not?"*

---

## The three files

| File | What it's for | When to use it |
|---|---|---|
| **[01-pseudocode.md](01-pseudocode.md)** | Six cards you can reproduce on a whiteboard from memory | Study first. If you can't draw the agent loop cold, start here. |
| **[02-decisions.md](02-decisions.md)** | Eight decision frameworks with deciding factors and switching conditions | The core of what they're probing |
| **[03-narration.md](03-narration.md)** | Worked answers, the narration template, phrases that signal seniority | The day before / morning of |

---

## The five things to know cold

If you retain nothing else:

**1. Chains are DAGs. Cycles need a graph.**
That single sentence answers "why LangGraph?" precisely and technically. Everything else is
elaboration.

**2. The agent loop.**
```
loop (bounded):
    response = llm(messages, tools)
    append(response)
    if no tool_calls: return
    for each call: run it, append result with matching tool_call_id
```
Multi-agent systems are this, nested. If you know this loop you can derive most of the rest.

**3. The cost ladder.**
```
prompt only    1 call
+ tool call    2 calls
agent loop     2-10 calls
multi-agent    5-30 calls
```
This turns every "should we use agents?" question into a budget question, which is the
right frame.

**4. Default to NOT multi-agent.**
"I'd start with one agent and a curated tool set, and split only when I have evidence."
Saying this unprompted signals judgement more than any amount of architecture knowledge.

**5. Rules go in code; judgement goes in the model.**
Every business rule in a prompt is a rule you cannot test or guarantee. In LangGraph terms:
routers are code, nodes call models.

---

## How to study this

**If you have a few hours:**
1. Read `01-pseudocode.md`. Then close it and write out Card 3 (the agent loop) by hand.
   Check it. Repeat until it's right without looking.
2. Read `02-decisions.md` once for the shape, then re-read only the *"what would change my
   mind"* sections — that's the part that's hard to improvise.
3. Read the worked answers in `03-narration.md` **out loud**. Reading silently does not
   prepare you to speak.

**If you have 30 minutes:**
Read the five things above, then the worked answers in `03-narration.md`.

**If you have 10 minutes:**
The drill at the bottom of `03-narration.md`.

---

## The single highest-value habit

**Ask a clarifying question before answering any design question.**

"What's the latency budget?" / "Is this user-facing or batch?" / "How often does the
corpus change?"

It demonstrates that you know the design depends on constraints, it buys you thinking time,
and it turns an interrogation into a conversation. It costs ten seconds and changes the
register of the whole interview.

---

## Where this sits relative to your other material

| Module | Role |
|---|---|
| Module 1 — RAG & Vector DBs | reference: chunking, embeddings, vector stores |
| Module 2 — LangChain | reference: chains, LCEL, memory, tools |
| Module 3 — Agentic AI | reference: ReAct, CoT, LangGraph, multi-agent |
| Module 4 — Interview Storytelling | narrative structure for project stories |
| Module 6 — Know Your Projects | your specific project details |
| **This module** | **decisions and narration across all of the above** |

Use Modules 1–3 to look things up. Use this one to prepare what you'll *say*.

---

## Honest scope note

These files are **pseudocode and decision reasoning**, not runnable code — LangChain isn't
installed in this repo, so nothing here has been executed. That's deliberate: the ask was
to be able to *pseudocode* these and explain the reasoning, and pseudocode is what survives
API churn and what you can actually reproduce on a whiteboard.

If you want runnable, tested implementations of any of these patterns, that's a separate
piece of work — say the word.
