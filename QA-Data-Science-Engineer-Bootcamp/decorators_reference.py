"""
DECORATOR REFERENCE — run this file to see all three in action:
    python3 decorators_reference.py

Covers: @timer (wrap), @lru_cache (cache), @retry (resilience),
        plus the registration/metadata pattern pytest uses.
"""
import time
from functools import wraps, lru_cache


# ═══════════════════════════════════════════════════════════════
# THE CORE IDEA
#     @decorator  above a function  ==  func = decorator(func)
# ═══════════════════════════════════════════════════════════════


# ───────────────────────────────────────────────────────────────
# 1. @timer  — TWO layers (takes no arguments)
# ───────────────────────────────────────────────────────────────
def timer(func):                              # layer 1: takes the function
    @wraps(func)                              # preserves __name__ / __doc__
    def wrapper(*args, **kwargs):             # layer 2: the replacement
        start = time.perf_counter()
        result = func(*args, **kwargs)        # run the original
        print(f"    [timer] {func.__name__} took {time.perf_counter()-start:.4f}s")
        return result                         # MUST return, or caller gets None
    return wrapper                            # MUST return, or the func becomes None


# ───────────────────────────────────────────────────────────────
# 2. @lru_cache — built into functools
#    LRU = Least Recently Used (what gets evicted when full)
#    Cache KEY = the arguments.
#    ONLY safe on PURE functions (same input -> same output, no side effects).
# ───────────────────────────────────────────────────────────────
_api_calls = 0

@lru_cache(maxsize=128)
def lookup_cve_severity(cve_id):
    """Pretend this hits a slow external threat-intel API."""
    global _api_calls
    _api_calls += 1
    time.sleep(0.3)
    return {"CVE-2024-1234": 9.8}.get(cve_id, 5.0)


# ───────────────────────────────────────────────────────────────
# 3. @retry — THREE layers (because it takes arguments)
#    @retry(times=3) calls retry(3) FIRST, which returns a decorator,
#    which then wraps the function.
# ───────────────────────────────────────────────────────────────
def retry(times=3, delay=0.1, backoff=2, exceptions=(Exception,)):
    def decorator(func):                      # layer 2: takes the function
        @wraps(func)
        def wrapper(*args, **kwargs):         # layer 3: the replacement
            wait = delay
            for attempt in range(1, times + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    if attempt == times:
                        print(f"    all {times} attempts failed -> re-raising")
                        raise                 # give up, let the caller see it
                    print(f"    attempt {attempt} failed ({type(e).__name__}), "
                          f"retrying in {wait:.1f}s")
                    time.sleep(wait)
                    wait *= backoff           # EXPONENTIAL BACKOFF
        return wrapper
    return decorator                          # layer 1 returns the decorator


# ───────────────────────────────────────────────────────────────
# 4. REGISTRATION — what @pytest.fixture actually does
#    Doesn't modify the function at all. Just records it.
# ───────────────────────────────────────────────────────────────
FIXTURE_REGISTRY = {}

def my_fixture(func):
    FIXTURE_REGISTRY[func.__name__] = func     # remember by NAME
    return func                                # UNCHANGED


# ───────────────────────────────────────────────────────────────
# 5. METADATA — what @pytest.mark.parametrize does
#    Sticks a label on the function for a runner to read later.
# ───────────────────────────────────────────────────────────────
def parametrize(argname, values):
    def decorator(func):
        func._params = (argname, values)       # attach data
        return func                             # UNCHANGED
    return decorator


# ═══════════════════════════════════════════════════════════════
# DEMOS
# ═══════════════════════════════════════════════════════════════
if __name__ == "__main__":

    print("=" * 62)
    print("1. @timer — wraps behavior around a function")
    print("=" * 62)

    @timer
    def score_findings(n):
        time.sleep(0.05)
        return [i * 1.5 for i in range(n)]

    score_findings(1000)

    print("\n" + "=" * 62)
    print("2. @lru_cache — remembers results by argument")
    print("=" * 62)
    for label in ["1st (CVE-2024-1234)", "2nd (SAME)", "3rd (SAME)", "4th (DIFFERENT)"]:
        cve = "CVE-2024-9999" if "DIFFERENT" in label else "CVE-2024-1234"
        t = time.perf_counter()
        val = lookup_cve_severity(cve)
        print(f"  {label:22} -> {val}  ({time.perf_counter()-t:.3f}s)")
    print(f"\n  function actually ran {_api_calls}x for 4 calls")
    print(f"  {lookup_cve_severity.cache_info()}")
    print("  WARNING: stale results forever if the underlying data changes.")

    print("\n" + "=" * 62)
    print("3a. @retry — succeeds on the 3rd attempt")
    print("=" * 62)
    _attempts = 0

    @retry(times=4, delay=0.1, exceptions=(ConnectionError,))
    def fetch_scan_results():
        global _attempts
        _attempts += 1
        if _attempts < 3:
            raise ConnectionError("network hiccup")
        return {"findings": 42}

    print("  result:", fetch_scan_results())

    print("\n" + "=" * 62)
    print("3b. @retry — gives up and re-raises")
    print("=" * 62)

    @retry(times=3, delay=0.05, exceptions=(TimeoutError,))
    def upload_report():
        raise TimeoutError("upload service down")

    try:
        upload_report()
    except TimeoutError as e:
        print(f"  caller sees: {e}")

    print("\n" + "=" * 62)
    print("3c. @retry — WRONG exception type is NOT retried (correct!)")
    print("=" * 62)

    @retry(times=3, exceptions=(ConnectionError,))   # transient only
    def bad_config():
        raise ValueError("malformed config file")     # a real BUG

    try:
        bad_config()
    except ValueError as e:
        print(f"  failed instantly: {e}")
        print("  -> retrying a genuine bug 3x just wastes 3x the time")
        print("  -> exceptions=(Exception,) is the same mistake as bare 'except Exception:'")

    print("\n" + "=" * 62)
    print("4. REGISTRATION — how pytest fixtures actually work")
    print("=" * 62)

    @my_fixture
    def llm_client():
        return "FAKE_LLM_CLIENT"

    @my_fixture
    def db_connection():
        return "FAKE_DB"

    print("  registry:", list(FIXTURE_REGISTRY.keys()))
    print("  llm_client() unchanged:", llm_client())

    import inspect
    def run_test(test_func):
        needed = inspect.signature(test_func).parameters
        kwargs = {n: FIXTURE_REGISTRY[n]() for n in needed if n in FIXTURE_REGISTRY}
        print(f"  '{test_func.__name__}' wants {list(needed)} -> injecting {kwargs}")
        test_func(**kwargs)

    def test_something(llm_client, db_connection):
        assert llm_client == "FAKE_LLM_CLIENT"
        print("  test passed")

    run_test(test_something)

    print("\n" + "=" * 62)
    print("5. METADATA — how parametrize actually works")
    print("=" * 62)

    @parametrize("bad_input", [123, None, ["a"]])
    def test_rejects(bad_input):
        print(f"    ran with bad_input={bad_input!r}")

    argname, values = test_rejects._params
    print(f"  tag on the function: {test_rejects._params}")
    for v in values:
        test_rejects(v)

    print("\n" + "=" * 62)
    print("COMMON BUGS")
    print("=" * 62)

    def broken_no_args(func):
        def wrapper():                 # no *args!
            return func()
        return wrapper

    @broken_no_args
    def add(a, b): return a + b

    try:
        add(2, 3)
    except TypeError as e:
        print(f"  no *args/**kwargs -> {e}")

    def broken_no_return(func):
        def wrapper(*a, **k): return func(*a, **k)
        # forgot: return wrapper

    @broken_no_return
    def add2(a, b): return a + b

    print(f"  forgot 'return wrapper' -> add2 is {add2}")
    try:
        add2(2, 3)
    except TypeError as e:
        print(f"                          -> {e}")
        print("  ^ PROOF that @d literally does 'func = d(func)'")
