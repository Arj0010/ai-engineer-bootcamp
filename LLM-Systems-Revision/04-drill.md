# Mock Interview Drill — 20 questions, answered

**How to use this file:** cover the answer. Say yours **out loud**, timed to ~60–90
seconds. Then read the model answer and diff them — not word-for-word, but structurally:
did you state a default, a reason, a switching condition?

Reading this silently is worth maybe a fifth of saying it aloud. The failure mode in a real
interview is never "I didn't know" — it's "I knew it and it came out as mush." That's a
speech problem, and it only responds to speech practice.

Each question is tagged with **what they're actually probing**, because the literal question
is often not the question.

---

## Section A — Framework choice (Q1–5)

### Q1. "When would you use LangChain?"

*Probing: do you reach for tools reflexively, or do you scope first?*

**Weak answer:** "LangChain is good for building LLM applications with chains, memory, and
tool integrations." — That's a description. It answers "what is", not "when".

**Model answer:**

> "My honest first answer is: often I wouldn't. A lot of production LLM code is a prompt
> template and one SDK call, and wrapping that in a framework costs debuggability for
> nothing.
>
> Where it earns its place is when I want the uniform interface — every component supports
> invoke, batch, stream, and async, so I get streaming and concurrency without writing them,
> and I can swap the model or the parser without touching neighbours. And the integration
> surface is real: if I need six vector stores and four providers behind one interface,
> writing that myself is a waste of a month.
>
> The deciding factor is whether I'm composing more than about three steps or need
> streaming through the whole pipeline. Under that, plain functions."

---

### Q2. "LangChain or LangGraph for this? *(any workflow they describe)*"

*Probing: do you have a crisp technical criterion, or a vibe?*

**Model answer:**

> "One test: does the control flow ever need to go backwards? A chain is a DAG — it only
> flows forward. If anywhere in this workflow I need 'check the result, and if it's bad try
> again with different inputs', that's a cycle, and a DAG cannot express it.
>
> From what you've described, [the retry / the tool loop / the approval step] is the cycle,
> so: graph.
>
> If it turns out that's really just an SDK-level retry on a timeout, I'd take that back —
> that's a `tenacity` decorator, not a state machine, and I don't want a state schema and
> reducers for a linear pipeline."

**If they say there is no loop:** say chain, and say *why you'd resist the graph* — that's
the part that scores.

---

### Q3. "Why not just write the loop yourself with the raw SDK?"

*Probing: can you defend a framework, or do you only know how to criticise one? This is the
mirror of Q1 and they sometimes ask both to see if you'll contradict yourself.*

**Model answer:**

> "For a single tool loop, I often would — it's about forty lines and I know exactly what
> every one of them does.
>
> What I'd stop hand-rolling at is **durable state**. Once I need the run to survive a
> process restart, or pause for human approval and resume in a different process days later,
> I'm building a checkpointer, a state schema, and merge semantics. That's LangGraph, with
> my bugs instead of theirs.
>
> The second thing is streaming and observability across a multi-step flow. Getting
> token streaming to work cleanly through five composed steps is fiddly, and it's solved."

---

### Q4. "What are the downsides of these frameworks?"

*Probing: are you a fan or an engineer? Never answer "none, really".*

**Model answer:**

> "Three real ones.
>
> **Indirection when debugging.** When something behaves strangely in production, you want
> to see the exact string that went to the model. Layers of abstraction sit between you and
> that. I mitigate it by logging the fully-rendered prompt on every call — not the template,
> the rendered result.
>
> **API churn.** These libraries move fast; code written a year ago often needs rewriting.
> So I keep framework types out of my core logic and wrap them at the edges — my business
> code shouldn't import framework classes.
>
> **They make over-engineering easy.** It costs one line to add an agent, so people add an
> agent to a problem that was a function call."

---

### Q5. "We're not sure if we need agents at all. What do you think?"

*Probing: this is the trap question. The wrong answer is enthusiasm.*

**Model answer:**

> "I'd want to push the work as far *down* this ladder as it will go, because every step up
> costs a round trip and a failure mode:
>
> - Can the model answer from the prompt alone? One call. Do that.
> - Does it need external data, but I know *which* data from the request? Then I fetch it
>   myself and put it in the prompt — I don't need the model to choose. Still one call.
> - Does *which* tool depend on the input? Now it's tool calling. Two calls.
> - Does each step depend on the last result, in a way I can't predict? Now it's an agent
>   loop. Two to ten calls, and non-deterministic.
>
> Most things people call agents are actually the third box — one round of tool selection,
> not an open-ended loop. So: what does the workflow look like? If I can determine the next
> step from the input with a classifier or even rules, I'd do that, because it's faster,
> cheaper, and I can unit-test it."

---

## Section B — Tool calling and agents (Q6–11)

### Q6. "Draw the agent loop."

*Probing: have you actually implemented one? This is where the tutorial-readers get found
out — they draw the boxes but miss the message bookkeeping.*

Write this. Say each line as you write it.

```
messages = [system, user_input]

for i in 1..MAX:
    response = llm(messages, tools=schemas)
    messages.append(assistant_message(response))   # even if content is empty

    if not response.tool_calls:
        return response.content

    for call in response.tool_calls:
        try:    result = tools[call.name](call.args)
        except: result = "ERROR: " + str(e)        # errors go back to the MODEL
        messages.append(tool_message(result, tool_call_id=call.id))

return "hit iteration limit"
```

Then say the three things that prove you've shipped it:

> "The `tool_call_id` has to round-trip exactly — the provider rejects the next request if a
> tool result doesn't match an issued call, and that's the number one integration bug.
>
> You append the assistant message even when its content is empty, because the tool results
> have to attach to the call that requested them; skip it and you corrupt the history.
>
> And the loop is bounded. Without a cap, a confused model retries until your bill or your
> timeout ends it."

---

### Q7. "A tool throws an exception mid-run. What happens?"

*Probing: do you know that errors are context, not exceptions?*

**Model answer:**

> "It goes back to the model as a tool result, not up as an exception. The model can often
> recover — it called the weather API with a malformed date, sees the error, and retries with
> the right format. Crashing wastes the whole turn and the user's context.
>
> Two caveats. I sanitise the error first — a raw stack trace can leak internals and it's
> mostly noise to the model; I want the actionable part. And errors count against the
> iteration cap, because a model that can't fix its own call will try forever.
>
> The exception to feeding it back is anything where the retry itself is dangerous — a
> payment call that might have partially succeeded. That one I surface, and I make those
> tools idempotent with a client-side key so a retry is safe."

---

### Q8. "How do you decide what makes a good tool?"

*Probing: rarely asked, always impressive. Most people never think about tool design.*

**Model answer:**

> "I design tools the way I'd design an API for a junior engineer who can only read the
> docstring and never asks questions.
>
> **Few, and well-separated.** Two tools whose descriptions overlap will get confused; that
> shows up as the model picking the wrong one on ambiguous input. If I see that, I merge
> them or sharpen the boundary.
>
> **The description is the prompt.** It's not documentation, it's the thing the decision is
> made from. I put *when to use this* and *when not to* in it, and examples of the argument
> format.
>
> **Small, typed argument sets.** Flat beats nested. Enums beat free strings — every
> constraint I can express in the schema is a class of error the model can't make.
>
> **Return something the model can read.** Terse structured text, not a 40KB JSON dump — that
> blows the context and buries the answer.
>
> And the tool validates its own inputs regardless. The model is an untrusted caller."

---

### Q9. "How do you stop an agent looping forever?"

*Probing: the cap is the obvious half. The diagnosis is the other half.*

**Model answer:**

> "Hard iteration cap, always — that's the floor, not the solution.
>
> But hitting the cap is a *symptom*, so I log every run that hits it and read them. It's
> almost always one of three things: two tools whose descriptions overlap so the model
> oscillates between them; a tool returning something that doesn't actually answer what it
> was asked, so the model retries the same call; or a genuinely impossible task where the
> model can't conclude it's stuck.
>
> The first two are fixable in tool design. For the third I add an explicit 'report that you
> cannot do this' path — models are reluctant to give up unless you give them a way to.
>
> I also track a budget in tokens or cost, not just iteration count, because ten cheap calls
> and ten calls with a huge context are very different problems."

---

### Q10. "How would you test a non-deterministic system?"

*Probing: engineering maturity. Very common follow-up and lots of people flounder.*

**Model answer:**

> "I split it into the parts that are deterministic and the parts that aren't, because they
> need completely different treatment.
>
> **Deterministic, so normal unit tests:** every tool, the routers, the parsers, the retrieval
> layer against a fixed index, and the whole graph with a mocked model. That covers most of
> the code, and it's fast enough to run on every commit.
>
> **Non-deterministic, so evaluation not assertion:** a fixed set of inputs with expected
> outcomes, scored — exact match where it applies, an LLM judge where it doesn't. I track the
> score over time and gate on regression, not on an absolute bar.
>
> The thing I'd say explicitly: if the LLM judge is part of the pipeline, it needs its own
> validation — agreement with human labels on a sample — otherwise I'm trusting a second
> unvalidated model to tell me the first one is fine.
>
> And I'd run the eval at temperature 0 to cut variance, while knowing production isn't
> necessarily at 0."

---

### Q11. "The agent works in testing and fails in production. Where do you look?"

*Probing: debugging instinct.*

**Model answer:**

> "First question: do I have traces? If not, that's the actual problem and I'd fix it before
> guessing. I want every LLM call with its rendered prompt, its response, its tokens, and its
> latency.
>
> With traces, I'd look in this order, because it's roughly the order of likelihood:
>
> **Input distribution.** Test inputs are clean; real ones have typos, other languages,
> pasted HTML, and prompt injection. Usually it's this.
>
> **Context length.** Longer real conversations push the instructions further from the end,
> and instruction-following degrades. If failures correlate with conversation length, that's
> it.
>
> **Tool behaviour under real data.** The tool that returned three tidy rows in testing
> returns four hundred, and now the context is full of noise.
>
> **Concurrency.** Shared state between requests — a checkpointer keyed wrong, a session
> object reused.
>
> I'd bucket the failures before fixing anything. Fixing the first failure I see is how you
> spend a day on the tail."

---

## Section C — Multi-agent (Q12–15)

### Q12. "When do you go multi-agent?"

*Probing: judgement. The right answer starts with reluctance.*

**Model answer:**

> "My default is **don't**. One agent with five to eight well-described tools handles more
> than people expect, and multi-agent multiplies latency and compounds error rates — 90%
> reliable per agent across five agents is 59% end to end.
>
> The three things that would actually change my mind:
>
> **Context pressure.** One agent with thirty tools has a bloated system prompt and picks
> wrong. Splitting into three agents with ten each shrinks every decision.
>
> **Genuinely conflicting system prompts.** A 'careful, cite everything' researcher and a
> 'terse, idiomatic' code writer want opposite instructions. One prompt can't be both.
>
> **Parallelism over independent subtasks** — that's a real latency win, not just structure.
>
> What I wouldn't do is split because the diagram looks better. Every handoff is a full round
> trip and a place to lose context."

---

### Q13. "Supervisor or network?"

*Probing: do you know the operational difference, not just the picture?*

**Model answer:**

> "Supervisor, by default, and the reason is debuggability. Routing lives in one place, so
> there's one decision log to read when it goes wrong. With a network, routing is scattered
> across every agent and 'why did it go there?' means reconstructing a path from several
> logs.
>
> The cost is honest: the supervisor is an extra LLM call on every hop, and it's a single
> point of failure. On a four-hop task that's four extra calls.
>
> I'd go network only when routing genuinely can't centralise — when the agent doing the work
> is the only thing with enough context to know who's next, and summarising that back up to a
> supervisor loses information.
>
> Past six or eight agents a single supervisor's routing prompt gets unreliable, and that's
> when hierarchical starts making sense — team supervisors under a top-level one."

---

### Q14. "What do the agents share?"

*Probing: this is the question that separates people who've built one from people who've
drawn one. If they don't ask it, raise it yourself.*

**Model answer:**

> "This is the design decision that actually matters, more than the topology.
>
> **Shared message history:** every agent sees everything. Easy to get working, no
> information loss at the handoff. But context grows without bound, agents get distracted by
> history irrelevant to them, and cost scales with agents times turns.
>
> **Isolated state with an explicit handoff payload:** each agent's context stays small and
> focused, cost is bounded, and the transitions are loggable — I can see exactly what agent A
> told agent B. The cost is that the handoff schema has to carry everything the next agent
> needs, and when it doesn't, the bug is subtle and shows up late.
>
> I default to isolated with an explicit schema, because 'the handoff dropped a field' is a
> bug I can find and fix, whereas 'the agent got distracted by 40 turns of someone else's
> context' is diffuse and shows up as quality drift."

---

### Q15. "Two agents keep handing work back and forth. What do you do?"

*Probing: can you diagnose, or do you only know happy paths?*

**Model answer:**

> "Cap it first so it stops burning money — a global hop limit across the whole run, not
> per-agent.
>
> Then diagnose, and it's usually one of two things. Either their responsibilities overlap,
> so each genuinely thinks the other owns it — that's a boundary problem and the fix is in
> the descriptions, or merging them. Or neither has the authority to finish: nobody's prompt
> says 'if you have enough to answer, answer'. Every agent needs a terminal path.
>
> If it's a supervisor topology I'd also check whether the supervisor is seeing worker
> *outputs* or just the fact that a worker ran. If it can't see what came back, it can't tell
> the task is done and it'll keep dispatching.
>
> The structural fix, if it keeps happening, is a hop budget in the state that every handoff
> decrements — when it hits zero, forced summarise-and-answer."

---

## Section D — Design and production (Q16–20)

### Q16. "Design a customer support agent for us."

*Probing: the full end-to-end. Ask a question first — always.*

**Model answer:**

> "Two things before I design: can it take actions on the customer's account — refunds,
> cancellations — or is it read-only? And is there a human agent it can escalate to?
>
> *[assume: some actions, and escalation exists]*
>
> Then I'd shape it as a graph, roughly:
>
> **Classify first** — and in code where I can. Billing, technical, account action, chitchat.
> A cheap classifier or rules, because the route is a rule, not a judgement call.
>
> **Read-only questions** go to RAG over the help centre — retrieve, generate, and check
> grounding, because a confident wrong answer about a refund policy is worse than no answer.
>
> **Actions** go through tools, and the money-touching ones sit behind an approval interrupt.
> That's a specific reason I'd want LangGraph: the graph pauses, persists, waits, and resumes
> — possibly in a different process. You can't retrofit that onto a chain.
>
> **Escalation is a first-class path**, not a failure. Low grounding score, two failed
> attempts, detected frustration, or an explicit ask — hand off with a summary of what was
> already tried, so the customer doesn't repeat themselves.
>
> One agent, not several. There's one skill here: understand the request, look things up, act
> within limits.
>
> What I'd measure: containment rate, escalation quality, and groundedness — but the one I'd
> watch hardest is *wrong actions taken*, because that's the only failure with a refund
> attached to it."

---

### Q17. "How do you handle memory in a long conversation?"

*Probing: do you know the failure modes, not just the techniques?*

**Model answer:**

> "It depends on how long and whether it has to survive the session.
>
> Under about ten turns, full history. It works, it's simple, and complexity here is
> premature.
>
> Longer: sliding window of the last N turns plus a running summary of what fell off. I
> default to that because the failure mode is *graceful* — you lose old detail, not the
> current turn. Whereas summarising everything can lose the one specific thing the user
> mentioned once, and that's the failure users actually notice.
>
> Across sessions: now it's extraction and retrieval — pull out durable facts, store them,
> retrieve by relevance. That's RAG over the conversation, with its own accuracy budget, so
> I'd only build it when users genuinely expect the system to remember them.
>
> One thing I'd pin regardless of strategy: the system prompt and any hard constraints never
> get summarised away. Those stay verbatim, always."

---

### Q18. "Where do you put business logic — the prompt or the code?"

*Probing: what do you trust a model with? Answer this crisply; it separates people fast.*

**Model answer:**

> "Rules go in code. Judgement goes in the model.
>
> `if amount > 10000: require_approval` is not a prompt. Put it in a prompt and you have a
> rule you can't unit-test, can't guarantee, and that silently changes behaviour the day you
> swap models.
>
> The model handles what's genuinely ambiguous — is this customer frustrated, does this
> document answer this question, what is this person actually asking for.
>
> In LangGraph terms this maps directly onto the structure: **routers are code, nodes call
> models.** That's actually one of the better arguments for the graph — it forces the
> separation to be explicit instead of letting it blur into a prompt.
>
> The grey area is rules that are deterministic but tedious to enumerate. There I'll use the
> model but constrain it hard — structured output, enums, validation on the way out — so the
> rule is enforced in code even though the decision came from the model."

---

### Q19. "How do you know it's working?"

*Probing: do you measure, or do you eyeball demos?*

**Model answer:**

> "Four layers, and I keep them separate on purpose.
>
> **Retrieval:** recall@k and MRR. If the document isn't retrieved, nothing downstream can
> fix it — no amount of prompt work recovers from a miss.
>
> **Generation:** groundedness — is every claim traceable to retrieved context. This is the
> one I watch hardest, because a fluent, confident, unsupported answer is the worst failure
> mode: nothing throws, nothing alerts, and the user believes it.
>
> **Task:** end-to-end success rate. The only one the business cares about.
>
> **Ops:** p95 latency, cost per request, tool error rate. What actually pages you.
>
> The reason to keep retrieval and generation separate is diagnostic: when end-to-end quality
> drops I need to know which half regressed, and a single number won't tell me.
>
> If I'm using an LLM as judge for any of these, it needs validating against human labels on
> a sample first — otherwise I'm trusting an unvalidated model to grade another one."

---

### Q20. "Tell me about something you got wrong."

*Probing: was the project real? A project with no wrong turns is a tutorial.*

**Structure:** constraint → decision → what you got wrong → the signal that told you →
the fix → what you'd do differently. The mistake paragraph is the **strongest** part of the
answer, not a weakness — it's the proof.

**Model answer (shape — fill in with your own project):**

> "On [project], the constraint was [X].
>
> I initially [decision], because [reasoning that was plausible at the time — this matters,
> it shows the mistake wasn't careless].
>
> What I got wrong: [specific thing]. I found out because [concrete signal — a metric, a
> user complaint, an eval score, a bill].
>
> I changed it to [fix], and [what improved].
>
> If I rebuilt it I'd [specific change], because [the generalisable lesson]."

**Worked example of the shape, if you need one to borrow the rhythm from:**

> "I let the retriever return ten chunks, on the theory that more context is better. Quality
> got *worse*, not better — I only caught it because I was comparing eval scores across
> configurations rather than eyeballing outputs. The relevant chunk was getting buried in the
> middle of the context, which is exactly where models attend least. I cut it to three and
> added a reranker. The lesson I took: context is not free, and 'more information' and 'more
> tokens' are different things."

**Pick your mistake deliberately.** It should be technical, have a clear signal that made you
change something, and end in a fix. Not a character flaw, not something that suggests
carelessness, and not something so trivial it reads as a dodge.

---

## Scoring yourself

After each answer, check:

- [ ] Did I state a **default** in the first fifteen seconds? (Not "it depends" as the whole answer.)
- [ ] Did I give **one concrete reason**, not three vague ones?
- [ ] Did I name a **switching condition** — what would make me choose otherwise?
- [ ] Did I name something I'd **rule out**, and why?
- [ ] Did I say how I'd **know I was wrong**?
- [ ] Did I stay under ~90 seconds?

Three or more ticks is a good answer. The fifth one is the rarest and the most senior-sounding.

## If you only rehearse five

Q2 (chain vs graph), Q6 (draw the loop), Q12 (multi-agent default), Q18 (prompt vs code),
Q20 (your mistake).

Those five cover the criterion, the mechanics, the judgement, the taste, and the evidence
that you've actually built something. Almost every other question in this space is one of
those five wearing a different hat.
