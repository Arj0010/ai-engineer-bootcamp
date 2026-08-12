# Narration — how to talk about your thought process

Your interviewer said they want to see **how you would use these and what your thought
process was**. That is a specific ask, and it is not the same as "explain LangGraph".

They are testing three things:

1. **Do you have a default?** (decisive)
2. **Do you know why?** (principled, not cargo-culting)
3. **Do you know when you'd be wrong?** (experienced)

Most candidates do (1) and half of (2). Doing all three puts you in a different band.

---

## The narration template

Use this shape for any design answer. It takes ~60–90 seconds, which is the right length.

```
1. CONSTRAINT      "Before I pick — what's the latency budget / is this user-facing?"
2. DEFAULT         "I'd start with X."
3. REASON          "Because [ONE concrete property], which matters here since [requirement]."
4. REJECTED        "I considered Y and ruled it out — [cost it would impose]."
5. SWITCH          "I'd move to Y if [specific, observable condition]."
6. DISCONFIRM      "I'd watch [metric]. If it [moved], my choice was wrong."
```

Step 1 is the highest-leverage. **Asking a clarifying question before answering a design
question is almost always the right move** — it demonstrates that you know the answer
depends on constraints, and it buys you thinking time.

Step 6 is the differentiator. Very few candidates say how they'd find out they were wrong.

---

## Worked answers

### "Walk me through how you'd build an agent that answers questions over our docs."

> "First — is this a single question-answer, or a conversation where follow-ups matter?
> And roughly what latency can we spend?
>
> *[assume: conversational, a few seconds is fine]*
>
> I'd start with the simplest thing that could work: retrieve, stuff the context, generate.
> A straight chain, no agent. That handles most doc-QA, and it's one LLM call so it's fast
> and cheap and I can evaluate it properly.
>
> The place that breaks is when retrieval misses — the user phrases it differently from the
> docs, and you get a confident answer built on irrelevant chunks. So the first thing I'd
> add is a **grading step**: after retrieval, ask a cheap model whether the chunks actually
> answer the question. If they don't, rewrite the query and retry.
>
> That's the moment I'd move from a chain to LangGraph, and the reason is specific: 'retry
> with a modified query' is a **cycle**, and a chain is a DAG. It genuinely cannot express
> that without me hand-rolling a loop and a state object — which is just LangGraph with
> more bugs.
>
> I'd cap the retries at two, because past that you're usually not going to find it and
> you're spending the user's latency budget on a guess.
>
> What I'd *not* do here is a multi-agent system. There's one skill involved — find and
> summarise. Splitting it across agents would multiply latency and give me nothing.
>
> To know whether it works, I'd measure retrieval recall and answer groundedness
> separately, because when quality drops I need to know which half regressed."

**Why this answer scores:** clarifying question, simplest-first, a *specific* technical
reason for the framework choice, an explicit non-choice with justification, and a
measurement plan.

---

### "Why LangGraph and not just LangChain?"

Short version if they want it quick:

> "Chains are DAGs — the control flow only goes forward. The moment I need a cycle, like
> retry-with-a-different-query or an agent looping over tools until it's done, a chain
> can't express it. That's the line for me."

Then, if they push:

> "The two things I actually reach for it for are **cycles** and **durable state**. The
> checkpointer means the graph can pause mid-run, persist, and resume — which is how you
> do human-in-the-loop approval. If a workflow needs a human to approve a step before it
> executes, that's not something you retrofit onto a chain.
>
> I'd push back on using it for a linear pipeline, though. You have to design a state
> schema and think about reducers, and for `format → call → parse` that's pure overhead."

---

### "You have 30 tools. How do you handle that?"

> "30 tools in one agent is where tool selection starts failing — the system prompt is
> enormous and the model picks wrong. So I'd look at three options in order.
>
> First: **do I need the model to choose at all?** Often the tool is determined by the
> request type, and I can route with a classifier or even rules. That's faster, cheaper,
> and testable. I'd try to move as much as possible out of the model's decision.
>
> Second: **tool retrieval.** Embed the tool descriptions, retrieve the top 5 relevant to
> the query, and only expose those. The agent stays single but its decision gets small.
>
> Third: **split into specialist agents** with a supervisor — a research agent, a data
> agent, an action agent. I'd do this last, because it multiplies latency and every handoff
> is another place to lose context.
>
> The deciding factor between two and three is whether the tools cluster into groups that
> also want *different system prompts*. If they just want different tools, retrieval is
> enough. If a 'careful and cite everything' agent and a 'take action' agent need
> genuinely conflicting instructions, that's a real reason to split."

---

### "How would you make this reliable in production?"

> "I'd separate the failure modes, because they need different fixes.
>
> **The model returns something malformed** — structured output mode, validate on the way
> out, retry once with the validation error in context. Fails twice, the schema is too
> complex.
>
> **A tool errors** — feed the error back to the model rather than crashing. It can often
> recover or explain. But cap the loop, or a confused model will retry forever.
>
> **The model loops** — hard iteration cap, and I'd log every run that hits it, because
> that's a prompt or tool-description problem showing up as a symptom.
>
> **It's confidently wrong** — this is the dangerous one, because nothing throws. That's
> what groundedness checking is for: is every claim traceable to retrieved context.
>
> **It's slow** — stream tokens so time-to-first-token is low even if total time isn't, run
> independent tool calls concurrently, and cache aggressively at the retrieval layer.
>
> Underneath all of it I'd want tracing from day one — every LLM call, its inputs, its
> tokens, its latency. Without that you cannot debug a non-deterministic system; you're
> just guessing."

---

### "What was your thought process on [your own project]?"

The trap: describing **what** you built. They asked **why**.

Structure every project answer as **decisions**, not features:

```
"The core constraint was [X].

The main decision was [A vs B]. I went with A because [reason tied to the constraint].

The thing I got wrong initially was [honest mistake] — I found out because [signal],
and I changed it to [fix].

If I rebuilt it I'd [specific change], because [what I learned]."
```

**The mistake paragraph is not a weakness — it is the strongest part of the answer.** It
proves the project was real. Projects with no wrong turns are projects that were followed
from a tutorial.

Pick your mistake deliberately: something with a clear signal that made you change
something, not a character flaw. "I let the retriever return 10 chunks because more context
seemed better, and quality got *worse* — the relevant chunk was getting buried. I cut it to
3 and added reranking" is a perfect answer.

---

## Phrases that signal seniority

| Say | Because it shows |
|---|---|
| "Before I answer — what's the latency budget?" | design depends on constraints |
| "I'd start with the simplest thing that could work" | you don't over-engineer |
| "The deciding factor is..." | you have a criterion, not a preference |
| "I'd rule out X because it costs Y" | you evaluate trade-offs |
| "I'd know I was wrong if [metric] moved" | you think in feedback loops |
| "That's a cycle, so a chain can't express it" | precise technical reasoning |
| "I'd push that logic into code, not the prompt" | you know what to trust a model with |
| "That's one LLM call vs five — worth it here?" | you think about cost and latency |

## Phrases to avoid

| Don't say | Why |
|---|---|
| "I'd use LangChain for that" (as a whole answer) | names a tool, not a decision |
| "Agents are better for complex tasks" | vague; every task is 'complex' |
| "It just works" | no |
| "I'd fine-tune" (as a first answer) | almost never the first move; suggests you skip cheaper options |
| "You'd add a vector database" | *which*, and why, and what happens when retrieval misses? |

---

## The 10-minute pre-interview drill

Do this the morning of. Out loud. Timed.

1. **Draw the agent loop from memory.** (Card 3.) 2 minutes.
2. **Say the chain-vs-graph line.** "Chains are DAGs; cycles need a graph." 30 seconds.
3. **Say the cost table.** prompt=1 call, tool=2, agent=2–10, multi-agent=5–30. 30 seconds.
4. **Rehearse the multi-agent default.** "I'd start with one agent and split only when
   I have evidence." 1 minute.
5. **Rehearse your project mistake paragraph.** 2 minutes.
6. **Rehearse one clarifying question** you'll ask before any design answer. 30 seconds.

If you can do those six things cold, you can hold a conversation about anything in this
space — because almost every question in this area reduces to one of them.
