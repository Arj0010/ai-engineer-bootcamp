"""Reference solution for load_test/locustfile_todo.py -- see Day 1/2 solutions/README.md convention: attempt first."""
from locust import HttpUser, task, between


class InferenceUser(HttpUser):
    wait_time = between(0.5, 2)

    @task(5)
    def predict(self):
        self.client.post("/predict", json={"text": "some sample ticket text"})

    @task(1)
    def health(self):
        self.client.get("/health")
