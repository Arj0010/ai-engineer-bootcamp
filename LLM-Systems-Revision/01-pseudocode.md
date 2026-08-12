# Pseudocode Cards — reproduce these from memory

The goal: you can draw any of these on a whiteboard **without notes**, and explain
what each line is doing. Not memorised API signatures — the *shape* of the thing.

Interviewers rarely care whether you remember `add_conditional_edges`'s exact argument
order. They care whether you know **there has to be a router function, and what it
returns**.

> These are pseudocode, deliberately. Real API surfaces churn every few months; the
> mechanics below have been stable for years. Where a real API name matters for
> recognition, it is noted in the margin.

---

## Card 1 — The LLM call (the atom everything else is built from)

```
function llm_call(messages, tools?):
    request  = { model, messages, temperature, tools?, response_format? }
    response = provider.send(request)

    return {
        content:    response.text            # may be empty when tool_calls present
        tool_calls: response.tool_calls      # [] when the model just answered
        usage:      response.tokens          # ALWAYS log this — cost + latency
        stop_reason: response.finish_reason  # "stop" | "tool_use" | "length"
    }
```

**Say this in interview:** "Everything above this is orchestration. The model itself is
stateless — every call resends the full context. That single fact drives the memory
design, the cost model, and the latency budget."

**The three things people forget:**
- `stop_reason == "length"` means you were **truncated**, not finished. Must be handled.
- Usage must be logged per-call, not per-request, or you cannot attribute cost.
- An empty `content` with populated `tool_calls` is normal, not an error.

---

## Card 2 — Chain (LangChain / LCEL)

```
chain = prompt | model | parser          # left-to-right dataflow

# what the pipe actually means:
function chain.invoke(input):
    x = prompt.format(input)     # dict  -> messages
    x = model.call(x)            # messages -> response
    x = parser.parse(x)          # response -> your type
    return x
```

Fan-out / fan-in:

```
parallel = {
    "summary":  summarise_chain,
    "keywords": keyword_chain,
}                                        # runs BOTH concurrently
combined = parallel | merge_prompt | model
```

Passing the original input forward:

```
chain = { "context": retriever, "question": passthrough } | prompt | model
#          ^ retrieved docs      ^ the original question, unchanged
```

**The whole value proposition:** a uniform interface. Every component supports
`invoke` / `batch` / `stream` / `async`, so you get streaming and concurrency for free
and can swap any component without touching its neighbours.

**The limitation that forces LangGraph:** a chain is a **DAG**. It flows one way. There
is no loop, so there is no "try again", no "keep calling tools until done", no
"reflect and revise".

---

## Card 3 — Tool calling (the loop that makes an agent)

This is the single most important card. Multi-agent systems are this loop, nested.

```
function agent_loop(user_input, tools, max_iterations = 10):

    messages = [system_prompt, user_message(user_input)]

    for iteration in 1..max_iterations:

        response = llm_call(messages, tools = tool_schemas(tools))
        messages.append(assistant_message(response))     # ALWAYS append, even if empty

        if response.tool_calls is empty:
            return response.content                      # model is done

        for call in response.tool_calls:
            try:
                result = tools[call.name].run(call.args)
            catch error:
                result = "ERROR: " + error.message       # feed errors BACK to the model

            messages.append(tool_message(
                content      = stringify(result),
                tool_call_id = call.id                   # ** MUST match **
            ))

    return "stopped: hit iteration limit"                # NEVER loop unbounded
```

### The five details that separate "read a tutorial" from "shipped this"

| Detail | Why it matters |
|---|---|
| `tool_call_id` must round-trip exactly | The provider rejects the next request if a tool result doesn't match an issued call. This is the #1 integration bug. |
| Append the assistant message **even when content is empty** | The tool results must attach to the call that requested them. Skipping it corrupts the history. |
| Errors go **back to the model**, not to the user | The model can often recover — retry with different args, or explain the failure. Crashing wastes the turn. |
| **Bounded** iterations | Without a cap, a confused model loops until your bill or your timeout ends it. |
| Parallel tool calls | Modern models emit several calls in one turn. Execute them concurrently, but append results in a deterministic order. |

**Say this in interview:** "The agent loop is: call model, if it asked for tools run them,
append results, repeat. The bounded loop is not optional — it's the difference between a
demo and something you can put in front of users."

---

## Card 4 — LangGraph (state machine)

The mental model: **a graph where nodes mutate a shared state object, and edges decide
what runs next.** That's it.

```
# 1. STATE — the contract every node reads and writes
State = {
    messages: list        with reducer = append      # reducers define MERGE semantics
    retries:  int         with reducer = overwrite
    context:  list
}

# 2. NODES — plain functions: state in, PARTIAL state out
function retrieve(state):
    docs = vectorstore.search(state.messages.last)
    return { context: docs }                # only the keys you changed

function generate(state):
    answer = llm_call(build_prompt(state))
    return { messages: [answer] }           # reducer APPENDS this

function grade(state):
    ok = llm_judge(state.messages.last, state.context)
    return { quality_ok: ok }

# 3. ROUTER — a function returning the NAME of the next node
function route_after_grade(state):
    if state.quality_ok:            return "END"
    if state.retries >= 3:          return "fallback"
    return "retrieve"                        # LOOP BACK — chains cannot do this

# 4. WIRING
graph.add_node("retrieve", retrieve)
graph.add_node("generate", generate)
graph.add_node("grade",    grade)

graph.add_edge(START,      "retrieve")
graph.add_edge("retrieve", "generate")
graph.add_edge("generate", "grade")
graph.add_conditional_edges("grade", route_after_grade)   # <- the branch point

app = graph.compile(checkpointer = store)     # checkpointer = durable state
```

### Why each piece exists

- **State + reducers** — nodes don't pass arguments to each other; they read and write a
  shared object. The reducer says *how* to merge (append to message history, overwrite a
  counter). This is what makes the graph resumable.
- **Partial returns** — a node returns only what it changed. Less coupling, easier testing.
- **Router functions** — this is where your *business logic* lives, and it's plain code, not
  a prompt. Cheap, deterministic, testable, and debuggable.
- **Checkpointer** — persists state after every node. Buys you three things at once:
  resume-after-crash, conversation memory, and human-in-the-loop pause/approve/resume.

**The killer feature to name:** `interrupt_before=["execute_payment"]`. The graph *pauses*,
persists, waits for approval, and resumes exactly where it stopped — possibly days later,
possibly in a different process. You cannot build that on a chain.

---

## Card 5 — Multi-agent: the three topologies

### (a) Supervisor — one router, N specialists

```
function supervisor(state):
    decision = llm_call(
        "Given the conversation, which worker should act next, or FINISH?",
        workers = ["researcher", "coder", "writer"]
    )
    return { next: decision }

route: supervisor -> (researcher | coder | writer | END)
each worker -> back to supervisor
```

Controlled, debuggable, easy to log. **The default choice.** Cost: the supervisor is an
extra LLM call on every hop, and it is a single point of failure.

### (b) Network — any agent hands off to any agent

```
function agent_a(state):
    ...work...
    if needs_specialist:
        return goto("agent_b", update = { messages: [handoff_note] })
    return goto(END)
```

Flexible, and very hard to reason about. Use only when the routing genuinely cannot be
centralised.

### (c) Hierarchical — supervisors of supervisors

```
top_supervisor -> { research_team_supervisor, eng_team_supervisor }
research_team_supervisor -> { web_agent, pdf_agent }
```

Scales past ~6–8 agents where a single supervisor's routing prompt gets unreliable.

### The design question that actually matters: **what do agents share?**

```
SHARED message history        every agent sees everything
    + no information loss, easy handoffs
    - context grows without bound; agents get distracted by irrelevant history
    - cost scales with (agents x turns)

ISOLATED state + explicit handoff payload
    + each agent's context stays small and focused
    + cost is bounded
    - the handoff schema must carry everything the next agent needs
    - information loss bugs are subtle and appear late
```

**Say this in interview:** "I default to a supervisor with isolated agent state and an
explicit handoff schema, because it keeps each agent's context small and makes the
transitions loggable. Shared history is easier to get working and harder to keep working."

---

## Card 6 — RAG, as a graph (ties the modules together)

```
                 ┌─────────────────────────────────┐
                 v                                 │ (retry with rewritten query)
  query -> [rewrite] -> [retrieve] -> [grade docs] ─┤
                                          │        │
                                    relevant       │
                                          v        │
                                    [generate]     │
                                          v        │
                                  [check grounding]┘
                                          │
                                    grounded -> END
```

```
function grade_docs(state):
    keep = [d for d in state.docs if llm_judge(d, state.query) == "relevant"]
    return { docs: keep, need_rewrite: len(keep) == 0 }

function check_grounding(state):
    # is every claim in the answer supported by a retrieved doc?
    return { grounded: llm_judge(state.answer, state.docs) }
```

This is "self-RAG" / "corrective RAG". **The point for interviews:** naive RAG is
`retrieve -> stuff -> generate`, a straight chain. The moment you add "was that any
good? if not, try differently" you have a **cycle**, and a cycle is exactly the thing
chains cannot express. That's the honest technical reason to reach for a graph.

---

## The one-page recall drill

Cover the right column. Write the left from memory.

| Prompt | You should be able to produce |
|---|---|
| Agent loop | Card 3, including `tool_call_id` and the iteration cap |
| Why not a chain? | "No cycles. Chains are DAGs." |
| LangGraph state | TypedDict + reducers; nodes return partials |
| Router function | takes state, returns next node NAME |
| Checkpointer buys you | resume, memory, human-in-the-loop |
| Three topologies | supervisor / network / hierarchical |
| Multi-agent's real question | shared vs isolated context |
| Corrective RAG | grade docs -> rewrite query -> retry (a cycle) |
