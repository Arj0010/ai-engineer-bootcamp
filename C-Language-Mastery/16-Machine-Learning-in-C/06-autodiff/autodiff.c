/* autodiff.c — REVERSE-MODE AUTOMATIC DIFFERENTIATION, from scratch.
 *
 *   make run
 *   gcc -std=c17 -Wall -Wextra -O2 autodiff.c -o autodiff -lm && ./autodiff
 *
 * This is what PyTorch and TensorFlow ARE, underneath. Karpathy's micrograd
 * is the Python version; this is the same idea in C, in about 250 lines.
 *
 * THE IDEA:
 *   Every arithmetic operation builds a NODE in a graph, recording its
 *   inputs and how to propagate a gradient back through itself. When you
 *   call backward() on the final value, the chain rule is applied
 *   automatically, in reverse topological order, all the way to the leaves.
 *
 * In module 05 you derived dL/dW1 by hand. Here you never derive anything:
 * you write the forward computation and the gradients appear.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* ================================================================= *
 * THE VALUE NODE
 *
 * Each node holds:
 *   - its VALUE (computed on the way forward)
 *   - its GRADIENT dL/d(this) (computed on the way back)
 *   - the nodes it was computed FROM
 *   - which OPERATION produced it, so backward() knows the local derivative
 * ================================================================= */
typedef enum {
    OP_LEAF,        /* an input or a parameter — nothing to propagate through */
    OP_ADD, OP_MUL, OP_SUB, OP_DIV, OP_NEG,
    OP_POW, OP_EXP, OP_LOG,
    OP_TANH, OP_RELU, OP_SIGMOID,
} Op;

typedef struct Value Value;
struct Value {
    double  data;          /* the forward value       */
    double  grad;          /* dL/d(this) — accumulated */
    Op      op;
    Value  *lhs, *rhs;     /* the operands (rhs is NULL for unary ops) */
    double  aux;           /* an extra constant, e.g. the exponent for POW */
    const char *label;
    bool    visited;       /* for the topological sort */
};

/* ================================================================= *
 * AN ARENA FOR THE NODES.
 *
 * A computation graph is many small, uniformly sized objects created
 * together and destroyed together — the exact case module 05 said an ARENA
 * is for. One allocation, no per-node free, and freeing the graph is a
 * single instruction.
 * ================================================================= */
#define MAX_NODES 100000
static Value  node_pool[MAX_NODES];
static size_t node_count = 0;

static Value *value_new(double data, Op op, Value *lhs, Value *rhs, const char *label)
{
    if (node_count >= MAX_NODES) { fprintf(stderr, "node pool exhausted\n"); exit(1); }
    Value *v = &node_pool[node_count++];
    v->data = data; v->grad = 0.0;
    v->op = op; v->lhs = lhs; v->rhs = rhs;
    v->aux = 0.0; v->label = label; v->visited = false;
    return v;
}

static Value *value_leaf(double data, const char *label)
{
    return value_new(data, OP_LEAF, NULL, NULL, label);
}

/* Reset the graph, keeping the first `keep` nodes (the parameters).
 * This is exactly what a training loop needs: rebuild the graph every
 * iteration, but keep the weights. */
static void graph_reset(size_t keep)
{
    node_count = keep;
    for (size_t i = 0; i < keep; i++) { node_pool[i].grad = 0.0; node_pool[i].visited = false; }
}

/* ================================================================= *
 * THE FORWARD OPERATIONS
 *
 * Each one computes a value AND records how it was made. That recording
 * is the only difference between this and ordinary arithmetic — and it is
 * what makes the backward pass possible.
 * ================================================================= */
static Value *v_add(Value *a, Value *b) { return value_new(a->data + b->data, OP_ADD, a, b, NULL); }
static Value *v_sub(Value *a, Value *b) { return value_new(a->data - b->data, OP_SUB, a, b, NULL); }
static Value *v_mul(Value *a, Value *b) { return value_new(a->data * b->data, OP_MUL, a, b, NULL); }
static Value *v_div(Value *a, Value *b) { return value_new(a->data / b->data, OP_DIV, a, b, NULL); }
static Value *v_neg(Value *a)           { return value_new(-a->data,          OP_NEG, a, NULL, NULL); }

static Value *v_pow(Value *a, double p)
{
    Value *v = value_new(pow(a->data, p), OP_POW, a, NULL, NULL);
    v->aux = p;
    return v;
}
static Value *v_exp(Value *a)  { return value_new(exp(a->data),  OP_EXP,  a, NULL, NULL); }
static Value *v_log(Value *a)  { return value_new(log(a->data),  OP_LOG,  a, NULL, NULL); }
static Value *v_tanh(Value *a) { return value_new(tanh(a->data), OP_TANH, a, NULL, NULL); }
static Value *v_relu(Value *a) { return value_new(a->data > 0 ? a->data : 0.0, OP_RELU, a, NULL, NULL); }

static Value *v_sigmoid(Value *a)
{
    double s = (a->data >= 0.0) ? 1.0 / (1.0 + exp(-a->data))
                                : exp(a->data) / (1.0 + exp(a->data));
    return value_new(s, OP_SIGMOID, a, NULL, NULL);
}

/* Convenience: operate with a plain constant. */
static Value *v_addc(Value *a, double c) { return v_add(a, value_leaf(c, "const")); }
static Value *v_mulc(Value *a, double c) { return v_mul(a, value_leaf(c, "const")); }

/* ================================================================= *
 * THE BACKWARD PASS
 *
 * For each node, given dL/d(this node), push gradient into its inputs
 * using the LOCAL derivative of the operation. That is the chain rule:
 *
 *     dL/d(input) += dL/d(output) * d(output)/d(input)
 *
 * Note the `+=`. A node used in several places accumulates gradient from
 * every path — that is the multivariable chain rule, and getting it wrong
 * (using `=`) is the classic autodiff bug.
 * ================================================================= */
static void backward_one(Value *v)
{
    switch (v->op) {
    case OP_LEAF:
        break;                                  /* nothing upstream */

    case OP_ADD:                                /* d(a+b)/da = 1, d/db = 1 */
        v->lhs->grad += v->grad;
        v->rhs->grad += v->grad;
        break;

    case OP_SUB:                                /* d(a-b)/da = 1, d/db = -1 */
        v->lhs->grad += v->grad;
        v->rhs->grad -= v->grad;
        break;

    case OP_MUL:                                /* d(a*b)/da = b, d/db = a */
        v->lhs->grad += v->rhs->data * v->grad;
        v->rhs->grad += v->lhs->data * v->grad;
        break;

    case OP_DIV:                                /* d(a/b)/da = 1/b, d/db = -a/b^2 */
        v->lhs->grad += v->grad / v->rhs->data;
        v->rhs->grad -= v->grad * v->lhs->data / (v->rhs->data * v->rhs->data);
        break;

    case OP_NEG:
        v->lhs->grad -= v->grad;
        break;

    case OP_POW:                                /* d(a^p)/da = p * a^(p-1) */
        v->lhs->grad += v->aux * pow(v->lhs->data, v->aux - 1.0) * v->grad;
        break;

    case OP_EXP:                                /* d(e^a)/da = e^a = the OUTPUT */
        v->lhs->grad += v->data * v->grad;
        break;

    case OP_LOG:                                /* d(ln a)/da = 1/a */
        v->lhs->grad += v->grad / v->lhs->data;
        break;

    case OP_TANH:                               /* d(tanh a)/da = 1 - tanh^2(a) */
        v->lhs->grad += (1.0 - v->data * v->data) * v->grad;
        break;

    case OP_RELU:                               /* 1 if positive, else 0 */
        v->lhs->grad += (v->data > 0.0 ? 1.0 : 0.0) * v->grad;
        break;

    case OP_SIGMOID:                            /* s * (1 - s) */
        v->lhs->grad += v->data * (1.0 - v->data) * v->grad;
        break;
    }
}

/* TOPOLOGICAL SORT.
 *
 * A node's gradient must be COMPLETE before it is propagated further back —
 * otherwise a node feeding two paths would push a partial gradient. A
 * depth-first post-order gives exactly the right order when reversed. */
static void topo_sort(Value *v, Value **order, size_t *n)
{
    if (v == NULL || v->visited) return;
    v->visited = true;
    topo_sort(v->lhs, order, n);
    topo_sort(v->rhs, order, n);
    order[(*n)++] = v;                          /* children first, then self */
}

static Value **topo_buffer = NULL;

static void backward(Value *root)
{
    if (topo_buffer == NULL) topo_buffer = malloc(MAX_NODES * sizeof *topo_buffer);

    for (size_t i = 0; i < node_count; i++) node_pool[i].visited = false;

    size_t n = 0;
    topo_sort(root, topo_buffer, &n);

    /* THE SEED: dL/dL = 1. Everything else follows from this one line. */
    root->grad = 1.0;

    /* Walk the topological order BACKWARDS. */
    for (size_t i = n; i-- > 0; ) backward_one(topo_buffer[i]);
}

static void zero_grad_all(void)
{
    for (size_t i = 0; i < node_count; i++) node_pool[i].grad = 0.0;
}

/* ================================================================= *
 * A NEURON, A LAYER, AND A NETWORK — built from Values.
 *
 * Note there is NO backward pass written anywhere below. The graph
 * records itself, and backward() figures it out.
 * ================================================================= */
typedef struct { Value **w; Value *b; size_t n_in; } Neuron;
typedef struct { Neuron *neurons; size_t n_neurons, n_in; } Layer;

static unsigned rng = 7;
static double randu(void) { rng = rng * 1103515245u + 12345u; return (double)((rng >> 16) & 0x7FFF) / 32767.0; }

static void layer_init(Layer *l, size_t n_in, size_t n_neurons)
{
    l->n_in = n_in; l->n_neurons = n_neurons;
    l->neurons = malloc(n_neurons * sizeof *l->neurons);

    double limit = sqrt(6.0 / (double)(n_in + n_neurons));
    for (size_t j = 0; j < n_neurons; j++) {
        l->neurons[j].n_in = n_in;
        l->neurons[j].w = malloc(n_in * sizeof *l->neurons[j].w);
        for (size_t i = 0; i < n_in; i++)
            l->neurons[j].w[i] = value_leaf((randu() * 2.0 - 1.0) * limit, "w");
        l->neurons[j].b = value_leaf(0.0, "b");
    }
}
static void layer_free(Layer *l)
{
    for (size_t j = 0; j < l->n_neurons; j++) free(l->neurons[j].w);
    free(l->neurons);
}

/* Forward: build the graph. tanh on hidden layers, raw output on the last. */
static void layer_forward(const Layer *l, Value **x, Value **out, bool activate)
{
    for (size_t j = 0; j < l->n_neurons; j++) {
        Value *sum = l->neurons[j].b;
        for (size_t i = 0; i < l->n_in; i++)
            sum = v_add(sum, v_mul(l->neurons[j].w[i], x[i]));
        out[j] = activate ? v_tanh(sum) : sum;
    }
}

int main(void)
{
    puts("=== WHAT AUTOMATIC DIFFERENTIATION IS ===");
    puts("  In module 05 you DERIVED dL/dW1 by hand and wrote it out. That is");
    puts("  fine for a two-layer network and impossible for a transformer.");
    puts("");
    puts("  Autodiff records the computation as a GRAPH while it runs. Each");
    puts("  node knows its inputs and the LOCAL derivative of its own");
    puts("  operation. Seed the output with dL/dL = 1, walk the graph");
    puts("  backwards applying the chain rule, and every gradient appears.");
    puts("");
    puts("  This is not symbolic differentiation (which explodes in size) and");
    puts("  not numerical differentiation (which is slow and imprecise). It is");
    puts("  exact, and it costs about one extra forward pass.\n");

    /* ---------------- a hand-checkable example ---------------- */
    puts("=== EXAMPLE 1: a tiny expression, checked by hand ===");
    {
        graph_reset(0);
        Value *a = value_leaf(2.0, "a");
        Value *b = value_leaf(-3.0, "b");
        Value *c = value_leaf(10.0, "c");

        Value *e = v_mul(a, b);          /* e = a*b   = -6  */
        Value *d = v_add(e, c);          /* d = e+c   =  4  */
        Value *f = value_leaf(-2.0, "f");
        Value *L = v_mul(d, f);          /* L = d*f   = -8  */

        printf("  a=%.1f b=%.1f c=%.1f f=%.1f\n", a->data, b->data, c->data, f->data);
        printf("  e = a*b = %.1f\n", e->data);
        printf("  d = e+c = %.1f\n", d->data);
        printf("  L = d*f = %.1f\n\n", L->data);

        backward(L);

        printf("  after backward():\n");
        printf("    dL/dL = %.1f    (the seed)\n", L->grad);
        printf("    dL/dd = %.1f    = f              (L = d*f, so dL/dd = f)\n", d->grad);
        printf("    dL/df = %.1f    = d\n", f->grad);
        printf("    dL/de = %.1f    = dL/dd * 1      (d = e+c: addition PASSES\n", e->grad);
        printf("    dL/dc = %.1f                      gradient through unchanged)\n", c->grad);
        printf("    dL/da = %.1f    = dL/de * b      = -2 * -3\n", a->grad);
        printf("    dL/db = %.1f    = dL/de * a      = -2 *  2\n", b->grad);
        puts("");
        puts("  Verify by hand: L = (a*b + c) * f, so dL/da = b*f = (-3)(-2) = 6.");
        puts("  The engine got 6. It applied the chain rule for us, node by node.");
        printf("  the graph used %zu nodes\n", node_count);
    }

    /* ---------------- gradient accumulation ---------------- */
    puts("\n=== EXAMPLE 2: a value used TWICE — why gradients ACCUMULATE ===");
    {
        graph_reset(0);
        Value *x = value_leaf(3.0, "x");
        Value *y = v_add(x, x);              /* y = x + x = 2x */

        backward(y);
        printf("  y = x + x, with x = %.1f -> y = %.1f\n", x->data, y->data);
        printf("  dy/dx = %.1f   (correct: y = 2x, so dy/dx = 2)\n", x->grad);
        puts("");
        puts("  x appears on BOTH sides, so gradient arrives twice: 1 + 1 = 2.");
        puts("  This is why backward_one uses `+=` and never `=`.");
        puts("");
        puts("  Using `=` would give dy/dx = 1 — a bug that produces a network");
        puts("  which trains, slowly and wrongly, with no error message. It is");
        puts("  THE classic autodiff bug, and it is why every framework makes");
        puts("  you call zero_grad() explicitly: accumulation is the default.");

        graph_reset(0);
        Value *a = value_leaf(4.0, "a");
        Value *b = v_mul(a, a);              /* b = a^2 */
        backward(b);
        printf("\n  b = a * a, with a = %.1f -> b = %.1f\n", a->data, b->data);
        printf("  db/da = %.1f   (correct: d(a^2)/da = 2a = 8)\n", a->grad);
        puts("  Both operands of the multiply are the SAME node, so each path");
        puts("  contributes a->data = 4, giving 8. Automatically.");
    }

    /* ---------------- verify against calculus ---------------- */
    puts("\n=== EXAMPLE 3: verified against hand calculus ===");
    {
        struct { const char *expr; double x; double expected; } tests[] = {
            {"d/dx (x^3)          = 3x^2",       2.0,  12.0},
            {"d/dx (exp(x))       = exp(x)",     1.0,  2.718281828},
            {"d/dx (log(x))       = 1/x",        4.0,  0.25},
            {"d/dx (tanh(x))      = 1-tanh^2",   0.5,  0.786447733},
            {"d/dx (sigmoid(x))   = s(1-s)",     0.0,  0.25},
            {"d/dx (x*x + 3x + 1) = 2x + 3",     5.0,  13.0},
            {"d/dx (relu(x))      = 1 if x>0",   2.0,  1.0},
            {"d/dx (relu(x))      = 0 if x<0",  -2.0,  0.0},
            {"d/dx (2x + 5)       = 2",          1.0,  2.0},
            {"d/dx (-x)           = -1",         3.0, -1.0},
            {"d/dx (1/x)          = -1/x^2",     2.0, -0.25},
        };

        for (size_t t = 0; t < sizeof tests / sizeof tests[0]; t++) {
            graph_reset(0);
            Value *x = value_leaf(tests[t].x, "x");
            Value *out = NULL;
            switch (t) {
            case 0: out = v_pow(x, 3.0); break;
            case 1: out = v_exp(x); break;
            case 2: out = v_log(x); break;
            case 3: out = v_tanh(x); break;
            case 4: out = v_sigmoid(x); break;
            case 5: out = v_add(v_add(v_mul(x, x), v_mulc(x, 3.0)), value_leaf(1.0, "1")); break;
            case 6: case 7: out = v_relu(x); break;
            case 8: out = v_addc(v_mulc(x, 2.0), 5.0); break;
            case 9: out = v_neg(x); break;
            case 10: out = v_div(value_leaf(1.0, "1"), x); break;
            }
            backward(out);
            double err = fabs(x->grad - tests[t].expected);
            printf("  %-32s at x=%.1f -> %.9f  (expected %.9f) %s\n",
                   tests[t].expr, tests[t].x, x->grad, tests[t].expected,
                   err < 1e-6 ? "OK" : "*** WRONG ***");
        }
        puts("\n  Note relu at x = -2: the gradient is exactly ZERO. A unit sitting");
        puts("  on the negative side receives no gradient at all and can never");
        puts("  recover — that is the DYING RELU problem, visible right here in");
        puts("  the derivative table.");
        puts("");
        puts("  Every one matches hand-computed calculus. We never wrote a");
        puts("  derivative for any of these EXPRESSIONS — only for the eleven");
        puts("  primitive OPERATIONS. Composition is handled by the graph.");
    }

    /* ---------------- train a network with zero hand-derived gradients ---- */
    puts("\n=== EXAMPLE 4: TRAINING XOR WITH NO HAND-DERIVED GRADIENTS ===");
    {
        graph_reset(0);

        Layer hidden, output;
        layer_init(&hidden, 2, 6);
        layer_init(&output, 6, 1);
        size_t n_params = node_count;          /* everything so far is a parameter */

        printf("  architecture: 2 -> 6 (tanh) -> 1\n");
        printf("  %zu parameters, all of them leaf nodes in the graph\n\n", n_params);

        double xs[4][2] = {{0,0},{0,1},{1,0},{1,1}};
        double ys[4]    = {-1.0, 1.0, 1.0, -1.0};   /* tanh-friendly targets */

        const int EPOCHS = 300;
        for (int epoch = 0; epoch <= EPOCHS; epoch++) {
            /* Rebuild the graph each epoch, keeping the parameters. This is
             * a DEFINE-BY-RUN framework — the graph is whatever the code did
             * this time, exactly like PyTorch. */
            graph_reset(n_params);
            zero_grad_all();

            Value *loss = value_leaf(0.0, "loss");
            for (int s = 0; s < 4; s++) {
                Value *x[2] = { value_leaf(xs[s][0], "x0"), value_leaf(xs[s][1], "x1") };
                Value *h[6], *o[1];
                layer_forward(&hidden, x, h, true);
                layer_forward(&output, h, o, false);

                Value *diff = v_sub(o[0], value_leaf(ys[s], "y"));
                loss = v_add(loss, v_pow(diff, 2.0));       /* squared error */
            }

            backward(loss);                    /* <-- EVERY gradient, one call */

            /* SGD update. The parameters are leaf nodes; their .grad is now
             * dLoss/dParam, computed automatically. */
            const double lr = 0.05;
            for (size_t j = 0; j < hidden.n_neurons; j++) {
                for (size_t i = 0; i < hidden.n_in; i++)
                    hidden.neurons[j].w[i]->data -= lr * hidden.neurons[j].w[i]->grad;
                hidden.neurons[j].b->data -= lr * hidden.neurons[j].b->grad;
            }
            for (size_t j = 0; j < output.n_neurons; j++) {
                for (size_t i = 0; i < output.n_in; i++)
                    output.neurons[j].w[i]->data -= lr * output.neurons[j].w[i]->grad;
                output.neurons[j].b->data -= lr * output.neurons[j].b->grad;
            }

            if (epoch % 50 == 0)
                printf("    epoch %4d  loss %.8f  (graph: %zu nodes)\n",
                       epoch, loss->data, node_count);
        }

        puts("\n  final predictions:");
        int correct = 0;
        for (int s = 0; s < 4; s++) {
            graph_reset(n_params);
            Value *x[2] = { value_leaf(xs[s][0], "x0"), value_leaf(xs[s][1], "x1") };
            Value *h[6], *o[1];
            layer_forward(&hidden, x, h, true);
            layer_forward(&output, h, o, false);
            int pred = o[0]->data > 0.0 ? 1 : -1;
            if ((double)pred == ys[s]) correct++;
            printf("    (%.0f, %.0f) -> %+.6f  sign %+d, expected %+.0f  %s\n",
                   xs[s][0], xs[s][1], o[0]->data, pred, ys[s],
                   (double)pred == ys[s] ? "OK" : "WRONG");
        }
        printf("  accuracy: %d/4\n", correct);

        puts("");
        puts("  READ THE TRAINING LOOP AGAIN. There is NO backward pass in it.");
        puts("  No delta, no W-transpose, no chain rule. We wrote the FORWARD");
        puts("  computation — the same arithmetic you would write to make a");
        puts("  prediction — and called backward() once.");
        puts("");
        puts("  That is the entire value proposition of a deep learning");
        puts("  framework. Change the architecture, add a layer, swap the loss:");
        puts("  the gradients follow automatically, because they were never");
        puts("  hard-coded in the first place.");

        layer_free(&hidden); layer_free(&output);
    }

    puts("\n=== WHY *REVERSE* MODE ===");
    puts("  There are two ways to apply the chain rule mechanically:");
    puts("");
    puts("  FORWARD MODE : start at an INPUT, push derivatives forward.");
    puts("                 One pass gives you dEVERYTHING/dx_i — the derivative");
    puts("                 of every output with respect to ONE input.");
    puts("                 Cost: O(n_inputs) passes to get all gradients.");
    puts("");
    puts("  REVERSE MODE : start at the OUTPUT, push derivatives backward.");
    puts("                 One pass gives you dL/dEVERYTHING — the derivative");
    puts("                 of ONE output with respect to every input.");
    puts("                 Cost: O(n_outputs) passes.");
    puts("");
    puts("  Machine learning has MILLIONS of parameters and ONE scalar loss.");
    puts("  Forward mode would need a million passes. Reverse mode needs one.");
    puts("");
    puts("  That asymmetry is the entire reason deep learning is computable.");
    puts("  Backpropagation is not a special algorithm invented for neural");
    puts("  networks — it is reverse-mode autodiff, which predates them.");

    puts("\n=== WHAT REAL FRAMEWORKS ADD ===");
    puts("  TENSORS      operate on whole arrays, not scalars. One node holds a");
    puts("               1024x1024 matrix, so the graph has thousands of nodes");
    puts("               instead of billions. This is the single biggest change.");
    puts("  KERNELS      each operation dispatches to a tuned BLAS or CUDA");
    puts("               routine instead of a scalar loop.");
    puts("  GRAPH OPTS   fuse operations, reuse buffers, drop dead nodes.");
    puts("  CHECKPOINTING recompute activations instead of storing them, to");
    puts("               trade compute for memory on very deep models.");
    puts("  DEVICES      move nodes to a GPU, shard across machines.");
    puts("");
    puts("  But the CORE — build a graph, seed dL/dL = 1, walk it backwards");
    puts("  applying local derivatives — is exactly what you just read.");
    puts("  It genuinely is about 250 lines.");

    printf("\n  peak graph size this run: %zu nodes of %d available\n",
           node_count, MAX_NODES);
    free(topo_buffer);
    return 0;
}
