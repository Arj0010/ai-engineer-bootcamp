/* nn.c — a neural network from scratch, with BACKPROPAGATION derived by hand.
 *
 *   make run
 *   gcc -std=c17 -Wall -Wextra -O2 nn.c -o nn -lm && ./nn
 *
 * This is the centre of the module. Everything before it was a warm-up for
 * the chain rule; everything after it automates what is written out here.
 *
 * THE NETWORK:
 *
 *   x --[W1,b1]--> z1 --sigma--> a1 --[W2,b2]--> z2 --sigma--> y_hat --> L
 *
 * FORWARD:   compute the prediction and the loss
 * BACKWARD:  compute dL/dW1, dL/db1, dL/dW2, dL/db2
 * UPDATE:    W -= alpha * dL/dW
 *
 * The backward pass is the only hard part, and it is written out in full
 * below with the derivation attached to each line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* ================================================================= *
 * ACTIVATIONS AND THEIR DERIVATIVES
 *
 * An activation must be NON-LINEAR. Without one, stacking layers is
 * pointless: W2*(W1*x) is just (W2*W1)*x, a single linear layer. The
 * non-linearity is what lets depth buy you anything.
 * ================================================================= */

static double sigmoid(double x)
{
    /* Numerically stable: exp(x) overflows for large positive x, so use the
     * algebraically identical form that only ever exponentiates a negative. */
    if (x >= 0.0) return 1.0 / (1.0 + exp(-x));
    double e = exp(x);
    return e / (1.0 + e);
}
/* d/dx sigma(x) = sigma(x) * (1 - sigma(x)).
 * Note it is expressed in terms of the OUTPUT, so the forward pass's result
 * is reused rather than recomputed — which is why every framework caches
 * activations during the forward pass. */
static double sigmoid_prime_from_output(double a) { return a * (1.0 - a); }

static double relu(double x) { return x > 0.0 ? x : 0.0; }
static double relu_prime_from_output(double a) { return a > 0.0 ? 1.0 : 0.0; }

static double tanh_prime_from_output(double a) { return 1.0 - a * a; }

typedef enum { ACT_SIGMOID, ACT_RELU, ACT_TANH } Activation;

static double activate(Activation f, double x)
{
    switch (f) {
    case ACT_SIGMOID: return sigmoid(x);
    case ACT_RELU:    return relu(x);
    case ACT_TANH:    return tanh(x);
    }
    return x;
}
static double activate_prime(Activation f, double output)
{
    switch (f) {
    case ACT_SIGMOID: return sigmoid_prime_from_output(output);
    case ACT_RELU:    return relu_prime_from_output(output);
    case ACT_TANH:    return tanh_prime_from_output(output);
    }
    return 1.0;
}

/* ================================================================= *
 * THE NETWORK — a fully connected two-layer perceptron.
 *
 * Weights are stored ROW-MAJOR and FLAT: W1[j * n_in + i] is the weight
 * from input i to hidden neuron j. One allocation, contiguous, cache
 * friendly (module 05's lesson applied).
 * ================================================================= */
typedef struct {
    size_t n_in, n_hidden, n_out;
    Activation hidden_act, output_act;

    double *W1, *b1;        /* input  -> hidden */
    double *W2, *b2;        /* hidden -> output */

    /* Cached during the forward pass, needed by the backward pass. */
    double *z1, *a1;        /* hidden pre-activation and activation */
    double *z2, *a2;        /* output pre-activation and activation */

    /* Gradient accumulators. */
    double *dW1, *db1, *dW2, *db2;
    double *delta1, *delta2;
} Network;

static unsigned rng_state = 1234;
static double uniform_rand(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return (double)((rng_state >> 16) & 0x7FFF) / 32767.0;
}

static bool nn_init(Network *n, size_t n_in, size_t n_hidden, size_t n_out,
                    Activation hidden_act, Activation output_act)
{
    memset(n, 0, sizeof *n);
    n->n_in = n_in; n->n_hidden = n_hidden; n->n_out = n_out;
    n->hidden_act = hidden_act; n->output_act = output_act;

    n->W1 = malloc(n_hidden * n_in * sizeof *n->W1);
    n->b1 = calloc(n_hidden, sizeof *n->b1);
    n->W2 = malloc(n_out * n_hidden * sizeof *n->W2);
    n->b2 = calloc(n_out, sizeof *n->b2);

    n->z1 = calloc(n_hidden, sizeof *n->z1);
    n->a1 = calloc(n_hidden, sizeof *n->a1);
    n->z2 = calloc(n_out, sizeof *n->z2);
    n->a2 = calloc(n_out, sizeof *n->a2);

    n->dW1 = calloc(n_hidden * n_in, sizeof *n->dW1);
    n->db1 = calloc(n_hidden, sizeof *n->db1);
    n->dW2 = calloc(n_out * n_hidden, sizeof *n->dW2);
    n->db2 = calloc(n_out, sizeof *n->db2);
    n->delta1 = calloc(n_hidden, sizeof *n->delta1);
    n->delta2 = calloc(n_out, sizeof *n->delta2);

    if (!n->W1 || !n->b1 || !n->W2 || !n->b2 || !n->z1 || !n->a1 ||
        !n->z2 || !n->a2 || !n->dW1 || !n->db1 || !n->dW2 || !n->db2 ||
        !n->delta1 || !n->delta2) return false;

    /* WEIGHT INITIALISATION MATTERS ENORMOUSLY.
     *
     * All zeros is fatal: every hidden neuron would compute the same thing,
     * receive the same gradient, and stay identical forever. The network
     * would have the capacity of ONE neuron no matter how wide it is. This
     * is the SYMMETRY BREAKING problem.
     *
     * The scale matters too. Xavier/Glorot initialisation uses a range of
     * sqrt(6 / (fan_in + fan_out)), which keeps the variance of the
     * activations roughly constant from layer to layer. Too small and the
     * signal vanishes with depth; too large and it saturates the sigmoid
     * (where the gradient is ~0) and learning stops. */
    double limit1 = sqrt(6.0 / (double)(n_in + n_hidden));
    for (size_t i = 0; i < n_hidden * n_in; i++)
        n->W1[i] = (uniform_rand() * 2.0 - 1.0) * limit1;

    double limit2 = sqrt(6.0 / (double)(n_hidden + n_out));
    for (size_t i = 0; i < n_out * n_hidden; i++)
        n->W2[i] = (uniform_rand() * 2.0 - 1.0) * limit2;

    return true;
}

static void nn_free(Network *n)
{
    free(n->W1); free(n->b1); free(n->W2); free(n->b2);
    free(n->z1); free(n->a1); free(n->z2); free(n->a2);
    free(n->dW1); free(n->db1); free(n->dW2); free(n->db2);
    free(n->delta1); free(n->delta2);
    memset(n, 0, sizeof *n);
}

/* ================================================================= *
 * FORWARD PASS
 *
 *   z1 = W1 . x  + b1        a1 = f(z1)
 *   z2 = W2 . a1 + b2        a2 = g(z2)      <- the prediction
 *
 * Every intermediate value is CACHED, because the backward pass needs them.
 * That memory cost is why training uses far more RAM than inference.
 * ================================================================= */
static const double *nn_forward(Network *n, const double *x)
{
    for (size_t j = 0; j < n->n_hidden; j++) {
        double sum = n->b1[j];
        for (size_t i = 0; i < n->n_in; i++) sum += n->W1[j * n->n_in + i] * x[i];
        n->z1[j] = sum;
        n->a1[j] = activate(n->hidden_act, sum);
    }
    for (size_t k = 0; k < n->n_out; k++) {
        double sum = n->b2[k];
        for (size_t j = 0; j < n->n_hidden; j++) sum += n->W2[k * n->n_hidden + j] * n->a1[j];
        n->z2[k] = sum;
        n->a2[k] = activate(n->output_act, sum);
    }
    return n->a2;
}

/* ================================================================= *
 * BACKWARD PASS — THE CHAIN RULE, WRITTEN OUT
 *
 * We want dL/dW for every weight. Work BACKWARDS from the loss.
 *
 * STEP 1 — the output layer.
 *   L = (1/2) * sum (a2_k - y_k)^2      (the 1/2 cancels the 2 from the power)
 *
 *   dL/da2_k = a2_k - y_k                          how wrong the output is
 *   da2_k/dz2_k = g'(z2_k)                         through the activation
 *
 *   Define  delta2_k = dL/dz2_k = (a2_k - y_k) * g'(z2_k)
 *
 *   "delta" is the error signal AT THE PRE-ACTIVATION. Every gradient in
 *   the layer is expressible in terms of it:
 *
 *   dL/dW2[k][j] = delta2_k * a1_j        because z2_k = sum_j W2[k][j]*a1_j
 *   dL/db2_k     = delta2_k               because dz2_k/db2_k = 1
 *
 * STEP 2 — the hidden layer. This is the step that IS backpropagation.
 *
 *   a1_j influences EVERY output, so its error is the SUM over all outputs
 *   of how much each one blames it:
 *
 *   dL/da1_j = sum_k delta2_k * W2[k][j]           <- the error flows BACK
 *                                                     through the same weights
 *   delta1_j = dL/dz1_j = (sum_k delta2_k * W2[k][j]) * f'(z1_j)
 *
 *   dL/dW1[j][i] = delta1_j * x_i
 *   dL/db1_j     = delta1_j
 *
 * THAT IS THE WHOLE ALGORITHM. For a deeper network you repeat step 2 once
 * per layer. Notice the pattern:
 *
 *     delta_layer = (W_next^T . delta_next) * f'(this layer)
 *     dW_layer    = delta_layer . input_to_this_layer^T
 *
 * The forward pass multiplies by W; the backward pass multiplies by W
 * TRANSPOSED. That symmetry is the whole of backpropagation.
 * ================================================================= */
static double nn_backward(Network *n, const double *x, const double *y)
{
    double loss = 0.0;

    /* ---- STEP 1: the output layer ---- */
    for (size_t k = 0; k < n->n_out; k++) {
        double error = n->a2[k] - y[k];                 /* dL/da2 */
        loss += 0.5 * error * error;

        n->delta2[k] = error * activate_prime(n->output_act, n->a2[k]);   /* dL/dz2 */

        for (size_t j = 0; j < n->n_hidden; j++)
            n->dW2[k * n->n_hidden + j] += n->delta2[k] * n->a1[j];
        n->db2[k] += n->delta2[k];
    }

    /* ---- STEP 2: propagate the error BACK to the hidden layer ---- */
    for (size_t j = 0; j < n->n_hidden; j++) {
        double back = 0.0;
        for (size_t k = 0; k < n->n_out; k++)
            back += n->delta2[k] * n->W2[k * n->n_hidden + j];   /* W2 TRANSPOSED */

        n->delta1[j] = back * activate_prime(n->hidden_act, n->a1[j]);

        for (size_t i = 0; i < n->n_in; i++)
            n->dW1[j * n->n_in + i] += n->delta1[j] * x[i];
        n->db1[j] += n->delta1[j];
    }
    return loss;
}

static void nn_zero_gradients(Network *n)
{
    memset(n->dW1, 0, n->n_hidden * n->n_in * sizeof *n->dW1);
    memset(n->db1, 0, n->n_hidden * sizeof *n->db1);
    memset(n->dW2, 0, n->n_out * n->n_hidden * sizeof *n->dW2);
    memset(n->db2, 0, n->n_out * sizeof *n->db2);
}

static void nn_apply_gradients(Network *n, double alpha, size_t batch_size)
{
    double scale = alpha / (double)batch_size;
    for (size_t i = 0; i < n->n_hidden * n->n_in; i++) n->W1[i] -= scale * n->dW1[i];
    for (size_t i = 0; i < n->n_hidden;            i++) n->b1[i] -= scale * n->db1[i];
    for (size_t i = 0; i < n->n_out * n->n_hidden; i++) n->W2[i] -= scale * n->dW2[i];
    for (size_t i = 0; i < n->n_out;               i++) n->b2[i] -= scale * n->db2[i];
}

/* One full-batch epoch: forward + backward over every sample, then ONE update. */
static double nn_train_epoch(Network *n, const double *X, const double *Y, size_t n_samples,
                             double alpha)
{
    nn_zero_gradients(n);
    double total = 0.0;

    for (size_t s = 0; s < n_samples; s++) {
        const double *x = X + s * n->n_in;
        const double *y = Y + s * n->n_out;
        nn_forward(n, x);
        total += nn_backward(n, x, y);
    }
    nn_apply_gradients(n, alpha, n_samples);
    return total / (double)n_samples;
}

/* ================================================================= *
 * GRADIENT CHECKING — verify the hand-derived backprop numerically.
 *
 * This is the single most important debugging tool in this file. A wrong
 * gradient does not crash; the network just learns badly, and you will
 * blame everything else first.
 * ================================================================= */
static double nn_loss_only(Network *n, const double *X, const double *Y, size_t n_samples)
{
    double total = 0.0;
    for (size_t s = 0; s < n_samples; s++) {
        const double *a = nn_forward(n, X + s * n->n_in);
        for (size_t k = 0; k < n->n_out; k++) {
            double e = a[k] - Y[s * n->n_out + k];
            total += 0.5 * e * e;
        }
    }
    return total;
}

static double nn_gradient_check(Network *n, const double *X, const double *Y, size_t n_samples)
{
    const double h = 1e-5;
    double worst = 0.0;

    /* Analytic gradients, accumulated over the whole set. */
    nn_zero_gradients(n);
    for (size_t s = 0; s < n_samples; s++) {
        nn_forward(n, X + s * n->n_in);
        nn_backward(n, X + s * n->n_in, Y + s * n->n_out);
    }

    /* Numerically perturb each W1 weight and compare. */
    for (size_t i = 0; i < n->n_hidden * n->n_in; i++) {
        double original = n->W1[i];

        n->W1[i] = original + h;
        double loss_plus = nn_loss_only(n, X, Y, n_samples);
        n->W1[i] = original - h;
        double loss_minus = nn_loss_only(n, X, Y, n_samples);
        n->W1[i] = original;

        double numeric = (loss_plus - loss_minus) / (2.0 * h);
        double denom = fabs(numeric) + fabs(n->dW1[i]) + 1e-12;
        double rel = fabs(numeric - n->dW1[i]) / denom;
        if (rel > worst) worst = rel;
    }
    /* The forward passes above overwrote the caches; restore a clean state. */
    nn_zero_gradients(n);
    return worst;
}

/* ================================================================= *
 * A LINEAR MODEL, to prove XOR genuinely needs the hidden layer.
 * ================================================================= */
static double linear_xor_best_accuracy(const double *X, const double *Y, size_t n)
{
    /* Brute-force search over a grid of (w0, w1, b): if NO linear boundary
     * can separate XOR, the best achievable accuracy is 75%. */
    double best = 0.0;
    for (double w0 = -3; w0 <= 3; w0 += 0.25)
    for (double w1 = -3; w1 <= 3; w1 += 0.25)
    for (double b  = -3; b  <= 3; b  += 0.25) {
        size_t correct = 0;
        for (size_t s = 0; s < n; s++) {
            double z = w0 * X[s*2] + w1 * X[s*2+1] + b;
            double pred = (z > 0.0) ? 1.0 : 0.0;
            if (pred == Y[s]) correct++;
        }
        double acc = (double)correct / (double)n;
        if (acc > best) best = acc;
    }
    return best;
}

int main(void)
{
    puts("=== WHY A HIDDEN LAYER IS NECESSARY: XOR ===");
    puts("     x0  x1 | XOR");
    puts("      0   0 |  0");
    puts("      0   1 |  1");
    puts("      1   0 |  1");
    puts("      1   1 |  0");
    puts("");
    puts("  No straight line separates the 1s from the 0s. Plot it: the two");
    puts("  positive cases sit on OPPOSITE CORNERS. Any single-layer model —");
    puts("  logistic regression included — draws exactly one straight boundary");
    puts("  and therefore CANNOT solve this.");
    puts("");
    puts("  Minsky and Papert pointed this out in 1969 and it stalled neural");
    puts("  network research for over a decade. The fix is a HIDDEN LAYER,");
    puts("  which lets the network build its own intermediate features.\n");

    double X_xor[8] = {0,0,  0,1,  1,0,  1,1};
    double Y_xor[4] = {0, 1, 1, 0};

    printf("  best possible accuracy for ANY linear boundary: %.0f%%\n",
           linear_xor_best_accuracy(X_xor, Y_xor, 4) * 100.0);
    puts("  (75% = it gets three of the four right and cannot do better)\n");

    /* ---------------- gradient check first ---------------- */
    puts("=== GRADIENT CHECK: is the backprop derivation correct? ===");
    {
        Network n;
        if (!nn_init(&n, 2, 4, 1, ACT_SIGMOID, ACT_SIGMOID)) return 1;
        double worst = nn_gradient_check(&n, X_xor, Y_xor, 4);
        printf("  worst relative error over all W1 gradients: %.3e\n", worst);
        printf("  %s\n", worst < 1e-6
               ? "  BACKPROP IS CORRECT (below 1e-6 means the analytic and"
               : "  *** BACKPROP IS WRONG ***");
        if (worst < 1e-6) puts("   numerical gradients agree to floating-point precision)");
        nn_free(&n);
        puts("");
        puts("  This compares each hand-derived gradient against a numerical");
        puts("  estimate (L(w+h) - L(w-h)) / 2h. It is O(parameters) forward");
        puts("  passes and far too slow for training — but run it ONCE after");
        puts("  writing any backward pass. It is the difference between");
        puts("  debugging in minutes and debugging for days.");
    }

    /* ---------------- train on XOR ---------------- */
    puts("\n=== TRAINING ON XOR ===");
    {
        Network n;
        if (!nn_init(&n, 2, 4, 1, ACT_SIGMOID, ACT_SIGMOID)) return 1;

        printf("  architecture: 2 inputs -> 4 hidden (sigmoid) -> 1 output (sigmoid)\n");
        printf("  parameters: %zu weights + %zu biases = %zu total\n\n",
               n.n_hidden * n.n_in + n.n_out * n.n_hidden,
               n.n_hidden + n.n_out,
               n.n_hidden * n.n_in + n.n_out * n.n_hidden + n.n_hidden + n.n_out);

        const int EPOCHS = 20000;
        for (int epoch = 0; epoch <= EPOCHS; epoch++) {
            double loss = nn_train_epoch(&n, X_xor, Y_xor, 4, 5.0);
            if (epoch % 2000 == 0)
                printf("    epoch %6d  loss %.8f\n", epoch, loss);
        }

        puts("\n  final predictions:");
        int correct = 0;
        for (size_t s = 0; s < 4; s++) {
            const double *out = nn_forward(&n, X_xor + s * 2);
            int pred = out[0] > 0.5 ? 1 : 0;
            if ((double)pred == Y_xor[s]) correct++;
            printf("    (%.0f, %.0f) -> %.6f  rounds to %d, expected %.0f  %s\n",
                   X_xor[s*2], X_xor[s*2+1], out[0], pred, Y_xor[s],
                   (double)pred == Y_xor[s] ? "OK" : "WRONG");
        }
        printf("  accuracy: %d/4 = %.0f%%\n", correct, correct * 25.0);

        puts("\n  what the HIDDEN NEURONS learned (their weights):");
        for (size_t j = 0; j < n.n_hidden; j++)
            printf("    h%zu: w = [%+.3f, %+.3f], b = %+.3f\n",
                   j, n.W1[j*2], n.W1[j*2+1], n.b1[j]);
        puts("    Each hidden neuron draws its OWN linear boundary. The output");
        puts("    layer then combines them, and a combination of straight lines");
        puts("    can carve out a non-linear region. THAT is what depth buys.");

        nn_free(&n);
    }

    /* ---------------- activation comparison ---------------- */
    puts("\n=== ACTIVATION FUNCTIONS COMPARED ===");
    {
        struct { Activation act; const char *name; double lr; } configs[] = {
            {ACT_SIGMOID, "sigmoid", 5.0},
            {ACT_TANH,    "tanh",    1.0},
            {ACT_RELU,    "relu",    0.5},
        };

        for (size_t c = 0; c < 3; c++) {
            Network n;
            if (!nn_init(&n, 2, 4, 1, configs[c].act, ACT_SIGMOID)) return 1;

            int epochs_to_converge = -1;
            double final_loss = 0.0;
            for (int epoch = 0; epoch < 20000; epoch++) {
                final_loss = nn_train_epoch(&n, X_xor, Y_xor, 4, configs[c].lr);
                if (final_loss < 0.001 && epochs_to_converge < 0) epochs_to_converge = epoch;
            }
            int correct = 0;
            for (size_t s = 0; s < 4; s++) {
                const double *out = nn_forward(&n, X_xor + s * 2);
                if ((out[0] > 0.5 ? 1.0 : 0.0) == Y_xor[s]) correct++;
            }
            printf("  %-8s (lr %.1f): final loss %.8f, %d/4 correct",
                   configs[c].name, configs[c].lr, final_loss, correct);
            if (epochs_to_converge >= 0) printf(", converged at epoch %d", epochs_to_converge);
            puts("");
            nn_free(&n);
        }
        puts("");
        puts("  SIGMOID  smooth, output in (0,1). Its derivative peaks at 0.25");
        puts("           and approaches 0 at both ends, so gradients SHRINK by");
        puts("           at least 4x per layer. That is the VANISHING GRADIENT");
        puts("           problem, and it is why deep sigmoid networks would not");
        puts("           train before 2010.");
        puts("  TANH     the same shape but centred on zero, which makes the");
        puts("           gradients better behaved. Derivative peaks at 1.0.");
        puts("  RELU     max(0, x). Derivative is exactly 1 for positive inputs,");
        puts("           so gradients DO NOT SHRINK with depth. Also trivially");
        puts("           cheap. This single change is most of why deep learning");
        puts("           started working. Its failure mode is DYING RELU: a");
        puts("           neuron pushed permanently negative has zero gradient");
        puts("           forever (hence leaky ReLU, GELU, and friends).");
        puts("");
        puts("  Note each needs a DIFFERENT learning rate. Activation and");
        puts("  learning rate are not independent choices.");
        puts("");
        puts("  AND NOTE WHAT ACTUALLY HAPPENED ABOVE: ReLU did WORSE than");
        puts("  sigmoid and tanh on this problem. That is not a mistake in the");
        puts("  code — it is DYING RELU, live. With only 4 hidden units and 4");
        puts("  training points, if a unit's pre-activation goes negative for");
        puts("  every input, its gradient is exactly zero forever and the unit");
        puts("  is dead. Lose one or two of four and the network no longer has");
        puts("  the capacity for XOR.");
        puts("");
        puts("  ReLU's advantage is about DEPTH, not tiny networks: it keeps");
        puts("  gradients from vanishing across many layers. On a 2-layer");
        puts("  4-unit toy, sigmoid is simply the better choice. Match the");
        puts("  activation to the architecture, not to fashion.");
    }

    /* ---------------- a harder, multi-class problem ---------------- */
    puts("\n=== A HARDER PROBLEM: TWO CONCENTRIC RINGS ===");
    {
        /* Points inside a radius are class 0, outside are class 1. The
         * boundary is a CIRCLE — not remotely linearly separable. */
        const size_t N = 400;
        double *X = malloc(N * 2 * sizeof *X);
        double *Y = malloc(N * 1 * sizeof *Y);

        for (size_t i = 0; i < N; i++) {
            double angle  = uniform_rand() * 2.0 * 3.14159265358979;
            bool   inner  = (i % 2 == 0);
            double radius = inner ? (0.0 + uniform_rand() * 0.8)
                                  : (1.4 + uniform_rand() * 0.8);
            X[i*2 + 0] = radius * cos(angle);
            X[i*2 + 1] = radius * sin(angle);
            Y[i]       = inner ? 0.0 : 1.0;
        }

        Network n;
        if (!nn_init(&n, 2, 8, 1, ACT_TANH, ACT_SIGMOID)) return 1;

        printf("  %zu points, 2 inputs -> 8 hidden (tanh) -> 1 output (sigmoid)\n", N);
        printf("  the true boundary is a CIRCLE of radius ~1.1\n\n");

        for (int epoch = 0; epoch <= 4000; epoch++) {
            double loss = nn_train_epoch(&n, X, Y, N, 0.5);
            if (epoch % 500 == 0) {
                size_t correct = 0;
                for (size_t s = 0; s < N; s++) {
                    const double *out = nn_forward(&n, X + s*2);
                    if ((out[0] > 0.5 ? 1.0 : 0.0) == Y[s]) correct++;
                }
                printf("    epoch %5d  loss %.6f  accuracy %.1f%%\n",
                       epoch, loss, 100.0 * (double)correct / (double)N);
            }
        }

        /* Draw the learned decision boundary as ASCII art. */
        puts("\n  the LEARNED decision boundary ('.' = class 0, '#' = class 1):");
        for (int row = -12; row <= 12; row += 2) {
            printf("      ");
            for (int col = -24; col <= 24; col += 1) {
                double pt[2] = { col / 12.0, row / 12.0 };
                const double *out = nn_forward(&n, pt);
                putchar(out[0] > 0.5 ? '#' : '.');
            }
            puts("");
        }
        puts("      ^ a circular region, learned from scratch. No feature");
        puts("        engineering, no explicit notion of radius — the hidden");
        puts("        layer discovered it from raw (x, y) coordinates.");

        free(X); free(Y);
        nn_free(&n);
    }

    puts("\n=== THE BACKPROPAGATION ALGORITHM, IN FIVE LINES ===");
    puts("  FORWARD:");
    puts("      z_l = W_l . a_{l-1} + b_l          a_l = f(z_l)");
    puts("  BACKWARD:");
    puts("      delta_L = (a_L - y) * f'(z_L)                 <- at the output");
    puts("      delta_l = (W_{l+1}^T . delta_{l+1}) * f'(z_l) <- propagate back");
    puts("      dW_l    = delta_l . a_{l-1}^T");
    puts("      db_l    = delta_l");
    puts("");
    puts("  The forward pass multiplies by W. The backward pass multiplies by");
    puts("  W TRANSPOSED. That is the entire symmetry, and it is why the");
    puts("  backward pass costs about the same as the forward pass.");
    puts("");
    puts("  WHY BACKWARDS AND NOT FORWARDS: computing dL/dw one parameter at a");
    puts("  time would need one full pass PER PARAMETER. Starting from");
    puts("  dL/dL = 1 and propagating toward the inputs gets EVERY gradient in");
    puts("  ONE pass. For a million parameters that is a million-fold");
    puts("  difference, and it is the only reason deep learning is possible.");

    puts("\n=== WHAT IS MISSING (deliberately) ===");
    puts("  This is full-batch gradient descent on a tiny network. Real");
    puts("  training adds:");
    puts("    MINI-BATCHES     update every 32-256 samples, not every epoch —");
    puts("                     faster, and the noise helps escape saddle points");
    puts("    MOMENTUM / ADAM  accumulate a velocity so the optimiser does not");
    puts("                     crawl along flat directions");
    puts("    REGULARISATION   weight decay, dropout — to stop it memorising");
    puts("    NORMALISATION    batch/layer norm, to keep activations well scaled");
    puts("    LR SCHEDULES     start large, decay over training");
    puts("    A VALIDATION SET this network is evaluated on its TRAINING data,");
    puts("                     which measures memorisation, not generalisation");
    puts("");
    puts("  NEXT: 06-autodiff removes the need to derive any of this by hand.");
    puts("  It builds a computation graph at run time and applies the chain");
    puts("  rule automatically — which is what PyTorch's .backward() is.");

    return 0;
}
