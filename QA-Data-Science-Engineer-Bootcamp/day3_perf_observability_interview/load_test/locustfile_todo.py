"""
Day 3, Exercise A: Load-test the mock inference service with Locust.

Setup:
  1. Terminal 1 (from day3_perf_observability_interview/):
       uvicorn mock_service.inference_server:app --port 8000
  2. Terminal 2 (same directory):
       locust -f load_test/locustfile_todo.py --host http://localhost:8000
  3. Open http://localhost:8089 in a browser. Set "Number of users" to 20
     and "Spawn rate" to 5, hit Start, let it run ~60 seconds.
  4. Repeat with 100 users to see how the service behaves under more load.

--- TASK LIST ---

1. Define class InferenceUser(HttpUser) with:
   - wait_time = between(0.5, 2)   (look up locust.between -- simulates a
     real client pausing between requests instead of hammering nonstop)
   - a @task method that does
       self.client.post("/predict", json={"text": "some sample ticket text"})

2. Add a SECOND @task, weighted lower than the predict task (e.g.
   @task(5) on predict, @task(1) on health), that does
       self.client.get("/health")
   This mimics a realistic traffic mix: one expensive endpoint hit often,
   one cheap endpoint (health checks) hit rarely.

3. Run the test at 20 users, then again at 100 users, and answer these
   (write your answers as a comment at the bottom of this file, or bring
   them to the debrief):
   a. What was the p95 response time for /predict at 20 users? At 100?
   b. At what user count did requests/sec stop increasing as you added
      more users (i.e. where did the service saturate)?
   c. Did you see any failures? What status codes, and what would cause
      them in a REAL inference service (vs. this mock)?
   d. Name ONE way this load pattern is unrealistic compared to real
      production traffic (think: request arrival pattern -- is Locust's
      default distribution bursty like real traffic, or too smooth?
      payload variety -- is every request really identical text length in
      production? cold starts -- does a real model server behave the same
      on request #1 as request #1000?).

Reference: https://docs.locust.io/en/stable/quickstart.html
"""
from locust import HttpUser, task, between


class InferenceUser(HttpUser):
    pass
    # TODO: task 1 -- set wait_time
    # TODO: task 1 -- add @task for POST /predict
    # TODO: task 2 -- add a lower-weight @task for GET /health

# Your answers to task 3, once you've run it:
# a.
# b.
# c.
# d.
