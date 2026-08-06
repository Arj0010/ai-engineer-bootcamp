/* linreg.c — linear regression, and therefore GRADIENT DESCENT.
 *
 *   make run
 *   gcc -std=c17 -Wall -Wextra -O2 linreg.c -o linreg -lm && ./linreg
 *
 * This is the smallest possible setting in which the whole machinery of
 * machine learning appears:
 *
 *   1. A MODEL with parameters:  y_hat = w*x + b
 *   2. A LOSS measuring error:   L = mean((y_hat - y)^2)
 *   3. GRADIENT DESCENT:         w -= alpha * dL/dw
 *
 * Every model in this module — up to the neural network — is these same
 * three steps with a more complicated step 1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* ================================================================= *
 * THE MODEL
 * ================================================================= */
typedef struct { double w, b; } LinearModel;

static double predict(const LinearModel *m, double x) { return m->w * x + m->b; }

/* MEAN SQUARED ERROR.
 *
 *   L = (1/n) * sum (y_hat_i - y_i)^2
 *
 * Why SQUARED? Three reasons:
 *   - errors of either sign both count as bad (unlike a plain sum)
 *   - it is DIFFERENTIABLE everywhere (unlike absolute error at 0)
 *   - it punishes large errors disproportionately, which is usually what
 *     you want
 */
static double mse_loss(const LinearModel *m, const double *x, const double *y, size_t n)
{
    double total = 0.0;
    for (size_t i = 0; i < n; i++) {
        double err = predict(m, x[i]) - y[i];
        total += err * err;
    }
    return total / (double)n;
}

/* ================================================================= *
 * THE GRADIENT — derived by hand, once, so you never have to wonder.
 *
 *   L = (1/n) * sum (w*x_i + b - y_i)^2
 *
 * Differentiate with respect to w, using the chain rule. The outer function
 * is u^2 (derivative 2u), and u = w*x + b - y has du/dw = x. So:
 *
 *   dL/dw = (1/n) * sum 2 * (w*x_i + b - y_i) * x_i
 *         = (2/n) * sum (error_i * x_i)
 *
 * Likewise du/db = 1, so:
 *
 *   dL/db = (2/n) * sum error_i
 *
 * THAT IS THE WHOLE DERIVATION. Every gradient in this module is the same
 * chain rule applied more times.
 * ================================================================= */
static void compute_gradient(const LinearModel *m, const double *x, const double *y,
                             size_t n, double *dw, double *db)
{
    double sum_dw = 0.0, sum_db = 0.0;

    for (size_t i = 0; i < n; i++) {
        double error = predict(m, x[i]) - y[i];    /* dL/dy_hat, up to the 2/n */
        sum_dw += error * x[i];                    /* chain rule: * dy_hat/dw = x */
        sum_db += error;                           /* chain rule: * dy_hat/db = 1 */
    }
    *dw = 2.0 * sum_dw / (double)n;
    *db = 2.0 * sum_db / (double)n;
}

/* ================================================================= *
 * GRADIENT DESCENT: step downhill, repeatedly.
 *
 * The gradient points in the direction of steepest INCREASE, so we move
 * AGAINST it. `alpha` (the learning rate) is how far we step.
 * ================================================================= */
static void train(LinearModel *m, const double *x, const double *y, size_t n,
                  double alpha, int epochs, bool verbose)
{
    for (int epoch = 0; epoch <= epochs; epoch++) {
        double dw, db;
        compute_gradient(m, x, y, n, &dw, &db);

        m->w -= alpha * dw;                        /* THE UPDATE */
        m->b -= alpha * db;

        if (verbose && (epoch % (epochs / 10) == 0 || epoch == epochs))
            printf("    epoch %5d  loss %10.6f  w %8.4f  b %8.4f  |grad| %.6f\n",
                   epoch, mse_loss(m, x, y, n), m->w, m->b, sqrt(dw*dw + db*db));
    }
}

/* ================================================================= *
 * THE CLOSED-FORM SOLUTION.
 *
 * For simple linear regression the optimum can be computed EXACTLY:
 *   w = sum((x - x_bar)(y - y_bar)) / sum((x - x_bar)^2)
 *   b = y_bar - w * x_bar
 *
 * So why iterate at all? Because the closed form only exists for a handful
 * of models. The moment the model is non-linear — a neural network — there
 * is no formula, and gradient descent is all you have. This function exists
 * so we can CHECK that gradient descent converges to the right answer.
 * ================================================================= */
static void closed_form(const double *x, const double *y, size_t n, LinearModel *out)
{
    double x_mean = 0.0, y_mean = 0.0;
    for (size_t i = 0; i < n; i++) { x_mean += x[i]; y_mean += y[i]; }
    x_mean /= (double)n; y_mean /= (double)n;

    double num = 0.0, den = 0.0;
    for (size_t i = 0; i < n; i++) {
        num += (x[i] - x_mean) * (y[i] - y_mean);
        den += (x[i] - x_mean) * (x[i] - x_mean);
    }
    out->w = (den != 0.0) ? num / den : 0.0;
    out->b = y_mean - out->w * x_mean;
}

/* R^2: what fraction of the variance in y the model explains.
 * 1.0 is perfect, 0.0 is no better than predicting the mean. */
static double r_squared(const LinearModel *m, const double *x, const double *y, size_t n)
{
    double y_mean = 0.0;
    for (size_t i = 0; i < n; i++) y_mean += y[i];
    y_mean /= (double)n;

    double ss_res = 0.0, ss_tot = 0.0;
    for (size_t i = 0; i < n; i++) {
        double r = y[i] - predict(m, x[i]);
        double t = y[i] - y_mean;
        ss_res += r * r;
        ss_tot += t * t;
    }
    return (ss_tot != 0.0) ? 1.0 - ss_res / ss_tot : 0.0;
}

/* ================================================================= *
 * MULTIPLE FEATURES — the same algorithm, one loop deeper.
 *   y_hat = w0*x0 + w1*x1 + ... + b
 * ================================================================= */
typedef struct { double *w; double b; size_t n_features; } MultiModel;

static double multi_predict(const MultiModel *m, const double *x)
{
    double sum = m->b;
    for (size_t j = 0; j < m->n_features; j++) sum += m->w[j] * x[j];
    return sum;
}

static double multi_loss(const MultiModel *m, const double *X, const double *y, size_t n)
{
    double total = 0.0;
    for (size_t i = 0; i < n; i++) {
        double e = multi_predict(m, X + i * m->n_features) - y[i];
        total += e * e;
    }
    return total / (double)n;
}

static void multi_train(MultiModel *m, const double *X, const double *y, size_t n,
                        double alpha, int epochs)
{
    size_t f = m->n_features;
    double *dw = calloc(f, sizeof *dw);

    for (int epoch = 0; epoch < epochs; epoch++) {
        memset(dw, 0, f * sizeof *dw);
        double db = 0.0;

        for (size_t i = 0; i < n; i++) {
            const double *xi = X + i * f;
            double error = multi_predict(m, xi) - y[i];
            for (size_t j = 0; j < f; j++) dw[j] += error * xi[j];
            db += error;
        }
        for (size_t j = 0; j < f; j++) m->w[j] -= alpha * 2.0 * dw[j] / (double)n;
        m->b -= alpha * 2.0 * db / (double)n;
    }
    free(dw);
}

/* A tiny deterministic RNG so every run of this program is identical. */
static unsigned rng = 42;
static double uniform(void) { rng = rng * 1103515245u + 12345u; return (double)((rng >> 16) & 0x7FFF) / 32767.0; }
static double gaussian(void)
{
    /* Box-Muller: two uniforms in, one standard normal out. */
    double u1 = uniform(), u2 = uniform();
    if (u1 < 1e-12) u1 = 1e-12;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979 * u2);
}

int main(void)
{
    puts("=== THE THREE STEPS OF (NEARLY ALL) MACHINE LEARNING ===");
    puts("  1. A MODEL with parameters      y_hat = w*x + b");
    puts("  2. A LOSS measuring wrongness   L = mean((y_hat - y)^2)");
    puts("  3. GRADIENT DESCENT             w -= alpha * dL/dw");
    puts("");
    puts("  Everything from here to a transformer is these same three steps");
    puts("  with a more complicated step 1.\n");

    /* ---------------- generate data with a known answer ---------------- */
    const size_t N = 100;
    double x[100], y[100];
    const double TRUE_W = 2.5, TRUE_B = -1.0;

    for (size_t i = 0; i < N; i++) {
        x[i] = (double)i / 10.0;                       /* 0.0 .. 9.9 */
        y[i] = TRUE_W * x[i] + TRUE_B + gaussian() * 0.5;   /* + noise */
    }

    printf("=== THE DATA ===\n");
    printf("  %zu points generated from y = %.1f*x + %.1f, plus N(0, 0.5) noise\n",
           N, TRUE_W, TRUE_B);
    printf("  first five: ");
    for (int i = 0; i < 5; i++) printf("(%.1f, %.2f) ", x[i], y[i]);
    puts("\n  The model does NOT know w and b. It has to find them.\n");

    /* ---------------- gradient descent ---------------- */
    puts("=== TRAINING BY GRADIENT DESCENT ===");
    {
        LinearModel m = {0.0, 0.0};        /* start from nothing */
        printf("  initial: w=%.4f b=%.4f loss=%.6f\n\n", m.w, m.b, mse_loss(&m, x, y, N));
        train(&m, x, y, N, 0.01, 1000, true);

        LinearModel exact;
        closed_form(x, y, N, &exact);

        printf("\n  gradient descent : w=%.6f  b=%.6f  loss=%.6f\n",
               m.w, m.b, mse_loss(&m, x, y, N));
        printf("  closed form      : w=%.6f  b=%.6f  loss=%.6f\n",
               exact.w, exact.b, mse_loss(&exact, x, y, N));
        printf("  true parameters  : w=%.6f  b=%.6f\n", TRUE_W, TRUE_B);
        printf("  R^2 = %.4f  (1.0 = perfect; the gap is the noise we added)\n",
               r_squared(&m, x, y, N));
        puts("");
        puts("  Gradient descent converged to essentially the closed-form answer.");
        puts("  Neither recovers the TRUE parameters exactly, and neither should:");
        puts("  the data contains noise, and the model fits the DATA, not the");
        puts("  process that generated it.");
    }

    /* ---------------- the learning rate ---------------- */
    puts("\n=== THE LEARNING RATE IS THE WHOLE GAME ===");
    {
        double rates[] = {0.0001, 0.001, 0.01, 0.05, 0.1};
        printf("  %10s %12s %12s %12s\n", "alpha", "final loss", "w", "b");
        for (size_t r = 0; r < sizeof rates / sizeof rates[0]; r++) {
            LinearModel m = {0.0, 0.0};
            train(&m, x, y, N, rates[r], 1000, false);
            double loss = mse_loss(&m, x, y, N);

            if (isfinite(loss))
                printf("  %10.4f %12.6f %12.4f %12.4f\n", rates[r], loss, m.w, m.b);
            else
                printf("  %10.4f %12s %12s %12s\n", rates[r], "DIVERGED", "nan", "nan");
        }
        puts("");
        puts("  TOO SMALL  : correct direction, but it never arrives in the");
        puts("               epochs you gave it (see alpha = 0.0001).");
        puts("  TOO LARGE  : each step OVERSHOOTS the minimum, the error grows,");
        puts("               the next step overshoots further, and it diverges");
        puts("               to infinity or NaN.");
        puts("  JUST RIGHT : converges quickly and stays.");
        puts("");
        puts("  There is no formula for the right value. In practice: start at");
        puts("  0.01, multiply or divide by 3, and watch the loss curve. If it");
        puts("  rises, halve it. This is why adaptive optimisers (Adam, RMSprop)");
        puts("  exist — they adjust the effective rate per parameter.");
    }

    /* ---------------- feature scaling ---------------- */
    puts("\n=== WHY FEATURE SCALING MATTERS ===");
    {
        /* Two features on wildly different scales — the usual real situation:
         * "number of rooms" (1-10) and "square metres" (50-500). */
        const size_t M = 60;
        double X_raw[120], X_scaled[120], yy[60];

        for (size_t i = 0; i < M; i++) {
            double rooms = 1.0 + uniform() * 9.0;
            double area  = 50.0 + uniform() * 450.0;
            X_raw[i*2 + 0] = rooms;
            X_raw[i*2 + 1] = area;
            yy[i] = 30.0 * rooms + 2.0 * area + 100.0 + gaussian() * 10.0;
        }

        /* Standardise: (x - mean) / stddev, per feature. */
        double mean[2] = {0,0}, sd[2] = {0,0};
        for (size_t j = 0; j < 2; j++) {
            for (size_t i = 0; i < M; i++) mean[j] += X_raw[i*2+j];
            mean[j] /= (double)M;
            for (size_t i = 0; i < M; i++) { double d = X_raw[i*2+j] - mean[j]; sd[j] += d*d; }
            sd[j] = sqrt(sd[j] / (double)M);
            for (size_t i = 0; i < M; i++) X_scaled[i*2+j] = (X_raw[i*2+j] - mean[j]) / sd[j];
        }

        printf("  feature 0 (rooms): mean %.2f, sd %.2f\n", mean[0], sd[0]);
        printf("  feature 1 (area) : mean %.2f, sd %.2f   <- ~50x larger scale\n",
               mean[1], sd[1]);

        double w_raw[2] = {0,0}, w_scaled[2] = {0,0};
        MultiModel raw = {w_raw, 0.0, 2}, scaled = {w_scaled, 0.0, 2};

        multi_train(&raw,    X_raw,    yy, M, 0.00001, 20000);
        multi_train(&scaled, X_scaled, yy, M, 0.01,    20000);

        printf("\n  UNSCALED, alpha must be tiny (1e-5) or it diverges:\n");
        printf("    loss %.4f  w = [%.4f, %.4f]\n",
               multi_loss(&raw, X_raw, yy, M), raw.w[0], raw.w[1]);
        printf("  SCALED, alpha = 0.01 works comfortably:\n");
        printf("    loss %.4f  w = [%.4f, %.4f]\n",
               multi_loss(&scaled, X_scaled, yy, M), scaled.w[0], scaled.w[1]);

        puts("");
        puts("  WHY: the gradient for a feature is proportional to that feature's");
        puts("  MAGNITUDE. With `area` 50x larger than `rooms`, its gradient is");
        puts("  50x larger, so a learning rate small enough to keep `area` stable");
        puts("  is far too small to move `rooms` at all. The loss surface is a");
        puts("  long narrow valley, and gradient descent zig-zags down it.");
        puts("");
        puts("  Scaling makes the valley round, so one learning rate suits every");
        puts("  parameter. ALWAYS SCALE YOUR FEATURES. It is the single highest-");
        puts("  value preprocessing step in classical ML, and it matters just as");
        puts("  much for neural networks (which is what batch normalisation is for).");
        puts("");
        puts("  Remember to apply the SAME mean and sd at prediction time — and");
        puts("  to compute them on the TRAINING set only, never the test set.");
    }

    /* ---------------- gradient checking ---------------- */
    puts("\n=== GRADIENT CHECKING: verify your calculus ===");
    {
        LinearModel m = {1.3, 0.7};
        double dw, db;
        compute_gradient(&m, x, y, N, &dw, &db);

        /* The numerical gradient: (L(w+h) - L(w-h)) / 2h.
         * Slow (two full loss evaluations per parameter) but it needs NO
         * calculus, so it is an independent check on your derivation. */
        const double h = 1e-6;
        LinearModel plus = m, minus = m;
        plus.w += h; minus.w -= h;
        double dw_numeric = (mse_loss(&plus, x, y, N) - mse_loss(&minus, x, y, N)) / (2*h);

        plus = minus = m;
        plus.b += h; minus.b -= h;
        double db_numeric = (mse_loss(&plus, x, y, N) - mse_loss(&minus, x, y, N)) / (2*h);

        printf("  analytic dL/dw = %.9f\n", dw);
        printf("  numeric  dL/dw = %.9f   relative error %.2e\n",
               dw_numeric, fabs(dw - dw_numeric) / (fabs(dw) + 1e-12));
        printf("  analytic dL/db = %.9f\n", db);
        printf("  numeric  dL/db = %.9f   relative error %.2e\n",
               db_numeric, fabs(db - db_numeric) / (fabs(db) + 1e-12));
        puts("");
        puts("  A relative error below ~1e-7 means your hand-derived gradient is");
        puts("  correct. ANY error above 1e-4 means it is wrong.");
        puts("");
        puts("  DO THIS EVERY TIME you derive a gradient by hand. A wrong");
        puts("  gradient does not crash — the model just trains badly, and you");
        puts("  will blame the learning rate, the data, and the architecture");
        puts("  for a week before checking the calculus.");
        puts("");
        puts("  Use the CENTRAL difference (f(x+h) - f(x-h)) / 2h, not the");
        puts("  forward difference: the error is O(h^2) instead of O(h).");
        puts("  h = 1e-6 is a good compromise between truncation error (h too");
        puts("  large) and floating-point cancellation (h too small).");
    }

    puts("\n=== WHAT TO CARRY FORWARD ===");
    puts("  - the gradient points UPHILL, so subtract it");
    puts("  - the learning rate decides whether you converge, crawl, or diverge");
    puts("  - always scale your features");
    puts("  - always gradient-check a hand-derived derivative");
    puts("  - a closed form exists here, and will not exist for anything");
    puts("    non-linear. Gradient descent is the general answer.");
    puts("");
    puts("  NEXT: 02-logistic-regression turns this into a CLASSIFIER by adding");
    puts("  one function (the sigmoid) and changing the loss.");

    return 0;
}
