"""
Module 3: Agentic AI Examples
Run these examples to understand agentic patterns.

Prerequisites:
    pip install langchain langchain-openai langgraph
    export OPENAI_API_KEY=your-key-here
"""

import os
from typing import TypedDict, Annotated, Sequence
from langchain_openai import ChatOpenAI
from langchain_core.messages import BaseMessage, HumanMessage, AIMessage
from langchain_core.prompts import ChatPromptTemplate

# Check for API key
if not os.getenv("OPENAI_API_KEY"):
    print("WARNING: Set OPENAI_API_KEY environment variable")
    print("Windows: set OPENAI_API_KEY=your-key")
    print("Linux/Mac: export OPENAI_API_KEY=your-key")


# =============================================================================
# Example 1: Simple Chain-of-Thought
# =============================================================================

def chain_of_thought_example():
    """Demonstrate Chain-of-Thought prompting."""
    print("\n" + "="*60)
    print("Example 1: Chain-of-Thought Prompting")
    print("="*60)

    llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0)

    # Without CoT
    prompt_without_cot = "What is 23 * 47?"

    # With CoT
    prompt_with_cot = "What is 23 * 47? Let's think step by step."

    print("\nWithout Chain-of-Thought:")
    print(f"Prompt: {prompt_without_cot}")
    response = llm.invoke(prompt_without_cot)
    print(f"Response: {response.content}")

    print("\nWith Chain-of-Thought:")
    print(f"Prompt: {prompt_with_cot}")
    response = llm.invoke(prompt_with_cot)
    print(f"Response: {response.content}")


# =============================================================================
# Example 2: Simple ReAct Pattern (Manual Implementation)
# =============================================================================

def simple_react_example():
    """Demonstrate a simplified ReAct pattern."""
    print("\n" + "="*60)
    print("Example 2: Simple ReAct Pattern")
    print("="*60)

    llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0)

    # Define simple tools
    def calculator(expression: str) -> str:
        """Calculate a mathematical expression."""
        try:
            return str(eval(expression))
        except:
            return "Error: Could not calculate"

    def knowledge_lookup(topic: str) -> str:
        """Lookup factual knowledge (simulated)."""
        knowledge_base = {
            "leap year days": "A leap year has 366 days.",
            "python creator": "Python was created by Guido van Rossum.",
            "earth moon distance": "The Moon is about 384,400 km from Earth.",
        }
        for key, value in knowledge_base.items():
            if key in topic.lower():
                return value
        return f"No information found about: {topic}"

    tools = {
        "calculator": calculator,
        "knowledge_lookup": knowledge_lookup
    }

    # ReAct prompt
    react_prompt = """You are a helpful assistant that answers questions step by step.

Available tools:
- calculator: For math calculations. Input: mathematical expression
- knowledge_lookup: For factual information. Input: topic to look up

When answering, follow this format:
Thought: [your reasoning about what to do next]
Action: [tool name]
Action Input: [input for the tool]

After receiving an observation, continue thinking until you have the final answer.
When you have the final answer, respond with:
Thought: I now know the final answer
Final Answer: [your answer]

Question: {question}

Begin!"""

    question = "What is 25 * 17 plus the number of days in a leap year?"

    print(f"\nQuestion: {question}")
    print("\nReAct Process:")
    print("-" * 40)

    # Simulate ReAct loop (simplified)
    messages = [HumanMessage(content=react_prompt.format(question=question))]

    for i in range(5):  # Max 5 iterations
        response = llm.invoke(messages)
        print(f"\n{response.content}")

        # Check if we have final answer
        if "Final Answer:" in response.content:
            break

        # Parse and execute action (simplified parsing)
        content = response.content
        if "Action:" in content and "Action Input:" in content:
            # Extract action and input
            action_line = [l for l in content.split('\n') if l.startswith('Action:')][0]
            input_line = [l for l in content.split('\n') if l.startswith('Action Input:')][0]

            action = action_line.replace('Action:', '').strip()
            action_input = input_line.replace('Action Input:', '').strip()

            # Execute tool
            if action in tools:
                observation = tools[action](action_input)
                print(f"\nObservation: {observation}")
                messages.append(AIMessage(content=response.content))
                messages.append(HumanMessage(content=f"Observation: {observation}"))
            else:
                print(f"\nUnknown action: {action}")
                break


# =============================================================================
# Example 3: Reflection Pattern
# =============================================================================

def reflection_example():
    """Demonstrate the reflection/self-correction pattern."""
    print("\n" + "="*60)
    print("Example 3: Reflection Pattern")
    print("="*60)

    llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0.7)

    task = "Write a Python function to check if a number is prime."

    print(f"\nTask: {task}")

    # Step 1: Initial generation
    print("\n--- Step 1: Initial Generation ---")
    initial_prompt = f"Write code for: {task}"
    initial_response = llm.invoke(initial_prompt)
    print(f"Initial code:\n{initial_response.content}")

    # Step 2: Self-critique
    print("\n--- Step 2: Self-Critique ---")
    critique_prompt = f"""Review this code for bugs, edge cases, and improvements:

{initial_response.content}

Provide specific critique points:"""

    critique_response = llm.invoke(critique_prompt)
    print(f"Critique:\n{critique_response.content}")

    # Step 3: Improvement based on critique
    print("\n--- Step 3: Improved Version ---")
    improve_prompt = f"""Original code:
{initial_response.content}

Critique:
{critique_response.content}

Provide an improved version addressing the critique:"""

    improved_response = llm.invoke(improve_prompt)
    print(f"Improved code:\n{improved_response.content}")


# =============================================================================
# Example 4: Planning Pattern
# =============================================================================

def planning_example():
    """Demonstrate the planning pattern."""
    print("\n" + "="*60)
    print("Example 4: Planning Pattern")
    print("="*60)

    llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0)

    goal = "Create a simple REST API for a todo list application"

    print(f"\nGoal: {goal}")

    # Step 1: Create plan
    print("\n--- Step 1: Create Plan ---")
    plan_prompt = f"""Break down this goal into concrete implementation steps:

Goal: {goal}

Provide a numbered list of steps (5-7 steps):"""

    plan_response = llm.invoke(plan_prompt)
    print(f"Plan:\n{plan_response.content}")

    # Step 2: Execute first step (simulated)
    print("\n--- Step 2: Execute First Step ---")
    execute_prompt = f"""Based on this plan:
{plan_response.content}

Implement Step 1 in detail. Provide the actual code or actions needed:"""

    execute_response = llm.invoke(execute_prompt)
    print(f"Step 1 Implementation:\n{execute_response.content}")


# =============================================================================
# Example 5: LangGraph Simple Workflow (if langgraph is installed)
# =============================================================================

def langgraph_example():
    """Demonstrate a simple LangGraph workflow."""
    print("\n" + "="*60)
    print("Example 5: LangGraph Workflow")
    print("="*60)

    try:
        from langgraph.graph import StateGraph, END

        # Define state
        class State(TypedDict):
            input: str
            draft: str
            critique: str
            final: str
            iteration: int

        llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0.7)

        # Define nodes
        def generate_draft(state: State) -> State:
            """Generate initial draft."""
            response = llm.invoke(f"Write a short poem about: {state['input']}")
            state['draft'] = response.content
            state['iteration'] = 1
            print(f"\nDraft:\n{state['draft']}")
            return state

        def critique_draft(state: State) -> State:
            """Critique the draft."""
            response = llm.invoke(f"Briefly critique this poem:\n{state['draft']}")
            state['critique'] = response.content
            print(f"\nCritique:\n{state['critique']}")
            return state

        def improve_draft(state: State) -> State:
            """Improve based on critique."""
            response = llm.invoke(
                f"Improve this poem based on the critique:\n\n"
                f"Poem: {state['draft']}\n\n"
                f"Critique: {state['critique']}"
            )
            state['final'] = response.content
            state['iteration'] += 1
            print(f"\nImproved:\n{state['final']}")
            return state

        def should_continue(state: State) -> str:
            """Check if we should continue improving."""
            if state['iteration'] >= 2:
                return "end"
            return "continue"

        # Build graph using StateGraph (modern API)
        workflow = StateGraph(State)

        workflow.add_node("generate", generate_draft)
        workflow.add_node("critique", critique_draft)
        workflow.add_node("improve", improve_draft)

        workflow.set_entry_point("generate")
        workflow.add_edge("generate", "critique")
        workflow.add_edge("critique", "improve")
        workflow.add_conditional_edges(
            "improve",
            should_continue,
            {
                "continue": "critique",
                "end": END
            }
        )

        app = workflow.compile()

        # Run
        print("\nRunning LangGraph workflow...")
        result = app.invoke({
            "input": "coding at night",
            "draft": "",
            "critique": "",
            "final": "",
            "iteration": 0
        })

        print(f"\n--- Final Result ---")
        print(f"Final poem:\n{result['final']}")

    except ImportError as e:
        print(f"LangGraph not installed or import error: {e}")
        print("Install with: pip install langgraph")
        print("Skipping this example.")
    except Exception as e:
        print(f"Error running LangGraph example: {e}")
        print("This may be due to an API change in LangGraph.")


# =============================================================================
# Example 6: Simple Multi-Agent Simulation
# =============================================================================

def multi_agent_example():
    """Simulate a simple multi-agent interaction."""
    print("\n" + "="*60)
    print("Example 6: Multi-Agent Simulation")
    print("="*60)

    llm = ChatOpenAI(model="gpt-3.5-turbo", temperature=0.7)

    task = "Write a function to validate email addresses"

    print(f"\nTask: {task}")

    # Agent 1: Developer
    print("\n--- Developer Agent ---")
    developer_prompt = f"""You are a Python developer. Write code for: {task}

Provide only the code, no explanations."""

    developer_response = llm.invoke(developer_prompt)
    print(f"Developer's code:\n{developer_response.content}")

    # Agent 2: Code Reviewer
    print("\n--- Reviewer Agent ---")
    reviewer_prompt = f"""You are a code reviewer. Review this code for bugs, security issues, and improvements:

{developer_response.content}

Be specific and constructive."""

    reviewer_response = llm.invoke(reviewer_prompt)
    print(f"Reviewer's feedback:\n{reviewer_response.content}")

    # Agent 3: Developer responds to review
    print("\n--- Developer Responds ---")
    fix_prompt = f"""You are a Python developer. Fix your code based on this review:

Original code:
{developer_response.content}

Review feedback:
{reviewer_response.content}

Provide the improved code:"""

    final_response = llm.invoke(fix_prompt)
    print(f"Final code:\n{final_response.content}")


# =============================================================================
# Main
# =============================================================================

def main():
    print("="*60)
    print("Module 3: Agentic AI Examples")
    print("="*60)

    examples = [
        ("1", "Chain-of-Thought", chain_of_thought_example),
        ("2", "Simple ReAct", simple_react_example),
        ("3", "Reflection Pattern", reflection_example),
        ("4", "Planning Pattern", planning_example),
        ("5", "LangGraph Workflow", langgraph_example),
        ("6", "Multi-Agent Simulation", multi_agent_example),
    ]

    print("\nAvailable examples:")
    for num, name, _ in examples:
        print(f"  {num}. {name}")
    print("  a. Run all examples")
    print("  q. Quit")

    while True:
        choice = input("\nEnter example number (or 'a' for all, 'q' to quit): ").strip().lower()

        if choice == 'q':
            print("Goodbye!")
            break
        elif choice == 'a':
            for _, name, func in examples:
                try:
                    func()
                except Exception as e:
                    print(f"Error in {name}: {e}")
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
            print("Invalid choice. Try again.")


if __name__ == "__main__":
    main()
