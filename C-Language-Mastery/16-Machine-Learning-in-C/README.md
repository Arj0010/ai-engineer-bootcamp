# Module 16 — Machine Learning in C

Every algorithm here is built from scratch, with no libraries beyond `<math.h>`. By the end
you will have written a neural network and a **reverse-mode automatic differentiation
engine** — which is the thing PyTorch and TensorFlow actually are, underneath.

---

## Why do this in C?

Not because you should train models in C. You should not — use PyTorch.

Do it because **C removes every layer of abstraction between you and the arithmetic.**
`loss.backward()` is one line in Python and a mystery. In C you write the chain rule
yourself, and it stops being a mystery permanently.

There is also a practical reason: **inference** in C is a real job.
`llama.cpp`, `ggml`, TensorFlow Lite, and every model that runs on a microcontroller are C
or C++ for the same reasons — no runtime, no GC pauses, predictable memory, and direct
control over SIMD and cache behaviour.

---

## The mathematical spine

Every model in this module is the same three steps:

1. **A model** with parameters θ, producing a prediction `ŷ = f(x; θ)`.
2. **A loss** measuring how wrong it is: `L(ŷ, y)`.
3. **Gradient descent**: nudge θ against the gradient, `θ ← θ − α·∂L/∂θ`.

The only thing that changes between linear regression and a transformer is how complicated
`f` is — and how you compute `∂L/∂θ`. That second problem is what backpropagation solves,
and what module 16's autodiff engine automates.

### The derivatives you actually need

| Function | Derivative |
|---|---|
| MSE: `L = (ŷ−y)²` | `∂L/∂ŷ = 2(ŷ−y)` |
| Sigmoid: `σ(x) = 1/(1+e⁻ˣ)` | `σ'(x) = σ(x)(1−σ(x))` |
| ReLU: `max(0,x)` | `1 if x>0 else 0` |
| tanh | `1 − tanh²(x)` |
| Binary cross-entropy with sigmoid | `∂L/∂z = ŷ − y` (the sigmoid derivative cancels) |
| Softmax with cross-entropy | `∂L/∂z = ŷ − y` (same beautiful cancellation) |

That last pair is why those loss/activation combinations are always paired: the messy
derivatives cancel and you get the simplest possible expression.

---

## The progression

| Sub-module | What it teaches |
|---|---|
| `01-linear-regression/` | Gradient descent itself, in its simplest possible setting. Closed form vs iterative. Feature scaling. |
| `02-logistic-regression/` | Classification, the sigmoid, cross-entropy, decision boundaries, and why BCE beats MSE here. |
| `03-knn-and-kmeans/` | Instance-based learning and unsupervised clustering. No gradients at all. |
| `04-decision-tree/` | Entropy, information gain, recursive partitioning, overfitting made visible. |
| `05-neural-network/` | **Backpropagation, derived and implemented.** XOR, then a real multi-class problem. |
| `06-autodiff/` | **A micrograd-style reverse-mode autodiff engine.** Build a computation graph, call `backward()`, and watch the chain rule propagate automatically. |
| `07-cnn-and-inference/` | Convolution, pooling, and a forward-only inference loop — the shape of production ML in C. |
| `common/` | A shared matrix library and dataset generators. |

Each sub-directory has its own `Makefile` and `README.md`.

---

## The one idea that matters: backpropagation

For a network `x → [W₁,b₁] → h → [W₂,b₂] → ŷ → L`, you want `∂L/∂W₁`.

The chain rule says:

```
∂L/∂W₁ = ∂L/∂ŷ · ∂ŷ/∂h · ∂h/∂W₁
```

Computing that **forwards** (one variable at a time) costs one full pass per parameter —
hopeless for a million parameters. Computing it **backwards** — starting from `∂L/∂L = 1`
and propagating toward the inputs — gets *every* gradient in **one** pass.

That asymmetry is the entire reason deep learning is computationally possible. It is called
reverse-mode automatic differentiation, and `06-autodiff/` implements it in about 200 lines.

---

## Building and running

```bash
cd 16-Machine-Learning-in-C

# each sub-module builds and runs independently
cd 01-linear-regression && make run

# or build everything
for d in */; do [ -f "$d/Makefile" ] && make -C "$d"; done
```

All of it links only `-lm`. There are no dependencies.

---

## What this module is not

It is not a substitute for understanding the mathematics, and it is not production ML.
The implementations here are **correct and readable**, not fast — no SIMD, no
multithreading, no GPU. Module 13 covers the optimisations you would apply, and
`07-cnn-and-inference/` shows where they matter most.

If you want to go further after this: read the `ggml` source. It is C, it is the engine
behind `llama.cpp`, and everything in this module is a prerequisite for reading it.
