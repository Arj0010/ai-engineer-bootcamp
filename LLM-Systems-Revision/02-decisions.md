# Decision Frameworks — the deciding factors

Feature-comparison tables tell an interviewer you read the docs. **Decision criteria plus
the thing that would change your mind** tells them you have shipped something.

The pattern for every answer in this file:

> **"I'd default to X. The deciding factor is [ONE concrete thing]. I'd switch to Y if
> [specific condition]. What I'd rule out immediately is Z, because [reason]."**

That structure does four jobs at once: it shows a default (you're decisive), a criterion
(you're principled), a switching condition (you're not dogmatic), and an exclusion (you
have judgement about what *not* to do).

---

## Decision 1 — Do you need a framework at all?

**This is the question most candidates skip, and the most senior-sounding one to raise.**

```
Is it ONE prompt in, one answer out?
    -> Just call the SDK. No framework.

Is it a fixed sequence of 2-4 steps, no branching?
    -> Plain functions, or a thin chain. A framework is overhead here.

Do you need retries, tool loops, branching, or persistence?
    -> Now a framework earns its keep.
```

**Say it like this:** "My first question is whether it needs a framework at all. A lot of
production LLM code is a prompt template and an SDK call, and wrapping that in an
abstraction costs you debuggability for nothing. I reach for orchestration when I need
control flow the SDK doesn't give me — loops, branching, or durable state."

**The honest trade-off to name:** frameworks buy you composability and a huge integration
surface; they cost you an indirection layer that gets in the way when you're debugging a
weird production failure at 2am. On a small team with one well-understood workflow, direct
SDK calls are often the better engineering decision.

---

## Decision 2 — LangChain chain vs LangGraph

### The one-line test

> **Does the control flow ever need to go backwards?**
>
> No -> chain. Yes -> graph.

That is the whole thing. A chain is a DAG. If you need "check the output, and if it's bad
try again," you need a cycle, and a DAG cannot express a cycle.

### The full decision table

| Signal in the requirements | Points to |
|---|---|
| Fixed pipeline: format → call → parse | **Chain** |
| Retry with a *modified* input on failure | **Graph** |
| Loop until a quality bar is met | **Graph** |
| Agent calling tools until done | **Graph** |
| Branch on the *content* of an intermediate result | **Graph** |
| Must survive a process restart mid-run | **Graph** (checkpointer) |
| Human approval in the middle | **Graph** (interrupt) |
| Need to stream tokens to a UI | Either — both support it |
| Team has never seen either | **Chain** — much lower learning cost |

### What would change my mind

- **Toward chain:** if the "loop" is really just an SDK-level retry on a 429 or a timeout,
  that's a `tenacity` decorator, not a state machine. Don't reach for a graph to get a
  retry.
- **Toward graph:** the moment a second person asks "what if it comes back empty?" — that's
  a branch, and branches multiply. Two branches in a chain is a pile of `if` statements
  with no state model; five is unmaintainable.

### The trap

LangGraph is genuinely more complex: you have to design a state schema, choose reducers,
and think about merge semantics. Using it for a linear pipeline is over-engineering, and a
good interviewer will push on exactly that. **Have the "when I would NOT use it" answer
ready** — it's the one that signals seniority.

---

## Decision 3 — Prompt-only vs Tool calling vs Agent

```
Can the model answer from its own weights + the prompt?
    -> PROMPT ONLY. Cheapest, fastest, most reliable. Always try first.

Does it need external data or an action, but you KNOW which one?
    -> Just call the tool yourself, then put the result in the prompt.
       ** You do not need the model to choose. **

Does it need external data, and WHICH one depends on the input?
    -> TOOL CALLING (single turn: model picks, you execute, model answers).

Does it need MULTIPLE steps where each depends on the last result?
    -> AGENT LOOP.
```

**The insight worth stating out loud:** most "agent" use cases are actually the third box —
one round of tool selection, not an open-ended loop. Letting the model iterate freely when
you already know the workflow adds latency, cost, and non-determinism for no benefit.

**Say it like this:** "I try to push work *down* this list. Every step up costs latency,
tokens, and a failure mode. If I can determine the tool from the request with a classifier
or even a regex, I do that and skip the model's decision entirely — it's faster, cheaper,
and I can unit-test it."

### Cost/latency intuition to have ready

| Approach | LLM calls | Relative latency | Determinism |
|---|---|---|---|
| Prompt only | 1 | 1× | high |
| Pre-fetched context | 1 | 1× + fetch | high |
| Single tool call | 2 | ~2× | medium |
| Agent loop | 2–10+ | 2–10× | low |
| Multi-agent | 5–30+ | high, and variable | lowest |

That table is the single most useful thing to memorise for a systems-design LLM interview.
It reframes every "should we use agents?" question as a **budget** question.

---

## Decision 4 — Single agent vs Multi-agent

### Default position: **do not use multi-agent.**

Say that plainly. It is the correct default and it demonstrates judgement.

```
Would ONE agent with 5-8 well-described tools work?
    -> Do that. Almost always yes.

Is the problem genuinely composed of distinct SKILLS with different
system prompts, tools, and success criteria?
    -> Multi-agent may be justified.

Do you need PARALLEL work on independent subtasks?
    -> Multi-agent is genuinely good at this.

Are you doing it because it sounds impressive?
    -> Don't.
```

### The real reasons to split

1. **Context pressure.** One agent with 30 tools has a bloated system prompt and picks the
   wrong tool. Splitting into three agents with 10 tools each shrinks every decision.
2. **Genuinely different system prompts.** A "careful, cite-everything" researcher and a
   "terse, idiomatic" code writer want conflicting instructions. One prompt can't be both.
3. **Parallelism.** Three independent subtasks can run concurrently — a real latency win.
4. **Independent evaluation.** You can test and improve the retrieval agent without
   touching the writing agent.

### The costs people underestimate

- **Latency multiplies.** Each handoff is a full LLM round trip. A 4-hop task at 2s/hop is
  8 seconds before any real work.
- **Errors compound.** 90% reliable per agent over 5 agents is 59% end-to-end.
- **Debugging gets much harder.** "Which agent decided that, and what did it see?" needs
  real tracing infrastructure from day one.
- **Cost multiplies**, and context often gets re-sent to every agent.

**Say it like this:** "I'd start with one agent and a well-curated tool set, and split only
when I have evidence — usually the model picking wrong tools because there are too many, or
a genuine need for parallelism. Multi-agent multiplies latency and compounds error rates,
so it needs to buy something specific."

### If you *do* split: supervisor vs network

| | Supervisor | Network |
|---|---|---|
| Routing logic | one place | scattered across agents |
| Debuggable | yes — one decision log | hard |
| Extra LLM calls | one per hop | fewer |
| Failure mode | supervisor is a bottleneck/SPOF | unpredictable loops |
| **Default** | **this one** | only when routing can't centralise |

---

## Decision 5 — Memory strategy

```
Single-turn?                       -> no memory. Don't build it.
Short conversation, < ~10 turns?   -> full history in context. Simplest, works.
Long conversation?                 -> sliding window (last N turns)
                                      + running summary of what fell off
Facts must persist across sessions?-> extract to a store, retrieve by relevance
                                      (this is RAG over the conversation)
```

**The deciding factor is where the failure shows up:** full history fails by hitting the
context limit *and* by burying the current question in noise. Summarisation fails by losing
a specific detail the user mentioned once. Retrieval fails by not retrieving the right
memory.

**Say it like this:** "Sliding window plus summary is my default because the failure mode is
graceful — you lose old detail, not the current turn. I'd move to extracted long-term
memory only when users expect the system to remember across sessions, because that's a
retrieval problem with its own accuracy budget, not a buffer."

---

## Decision 6 — Where does the logic live: prompt or code?

This one comes up constantly and separates people fast.

```
Is the rule DETERMINISTIC and expressible?
    -> CODE. Every time.
    "if amount > 10000: require_approval"  is not a prompt.

Does it need judgement over unstructured input?
    -> Model.

Is it deterministic but tedious to enumerate?
    -> Model, but constrain it: structured output, enum, validation on the way out.
```

**The principle:** *the model should handle ambiguity; code should handle rules.* Every
business rule you push into a prompt is a rule you cannot unit-test, cannot guarantee, and
that silently changes behaviour when you swap models.

In a LangGraph design this maps directly: **router functions are code, nodes call models.**
That's a genuinely good reason to prefer the graph — it forces the separation to be explicit.

---

## Decision 7 — Structured output

```
Need a specific shape back?
    -> Native structured output / function calling. Not prompt-and-pray.

Provider doesn't support it?
    -> JSON mode + schema validation + ONE retry with the validation error appended.

Still failing?
    -> Simplify the schema. Deeply nested objects fail far more than flat ones.
```

**Say it like this:** "I use the provider's structured output mode and validate on the way
out anyway. On a validation failure I retry once with the error message in the context —
the model usually self-corrects. If it fails twice the schema is too complex, and I'd flatten
it rather than add retries."

---

## Decision 8 — What to measure (they will ask)

Have these four ready:

| Layer | Metric | Why |
|---|---|---|
| **Retrieval** | recall@k, MRR | if the doc isn't retrieved, nothing downstream can fix it |
| **Generation** | groundedness / faithfulness | is every claim supported by context? catches hallucination |
| **Task** | end-to-end success rate | the only one the business cares about |
| **Ops** | p95 latency, cost/request, tool error rate | what actually pages you |

**Say it like this:** "I evaluate the retrieval and the generation separately, because when
end-to-end quality drops I need to know which half regressed. Groundedness is the one I
watch most — a fluent, confident, unsupported answer is the worst failure mode because
nothing surfaces it."

**Bonus point:** mention that an LLM-as-judge needs its own validation — agreement with
human labels on a sample — or you're just trusting a second unvalidated model.

---

## The universal fallback

When you get a decision question you haven't prepared, use this shape:

1. **Clarify the constraint.** "What's the latency budget?" / "Is this user-facing or batch?"
   — the answer usually determines the design, and asking shows you know that.
2. **State a default and why.** "I'd start with X because it's the simplest thing that
   handles the stated requirement."
3. **Name the switching condition.** "I'd move to Y when Z."
4. **Name what you'd rule out.** "I wouldn't do W here — it buys flexibility we don't need
   and costs debuggability."
5. **Name how you'd know you were wrong.** "I'd watch [metric]; if it regressed I'd revisit."

Point 5 is the one almost nobody does, and it is the most senior-sounding thing you can
say in an entire interview.
