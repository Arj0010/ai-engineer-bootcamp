// Reference only -- you do NOT need to install/run k6 for this bootcamp.
// This exists so you can read it once and compare it to the Locust
// exercise: same testing goal, different language/runtime model.
//
// Key structural difference to be able to explain out loud:
// - Locust: Python, user behavior defined as a class with @task methods,
//   distributed load generation via multiple worker processes, a live web
//   UI for controlling/observing a run.
// - k6: JavaScript, load defined declaratively via "scenarios" /
//   "executors" (e.g. ramping-vus, constant-arrival-rate) in `options`,
//   built for CI-native usage (a k6 run is just a CLI exit code + JSON/
//   text summary -- no separate UI server needed), often the pick when a
//   team is already JS-heavy or wants load tests checked into CI with
//   pass/fail thresholds baked into the script itself (see `thresholds`
//   below -- this is the important k6-specific idea: THE SCRIPT ITSELF
//   can fail CI, not just a human reading a report).

import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  scenarios: {
    predict_load: {
      executor: 'ramping-vus',
      startVUs: 0,
      stages: [
        { duration: '30s', target: 20 },
        { duration: '1m', target: 20 },
        { duration: '30s', target: 0 },
      ],
    },
  },
  thresholds: {
    http_req_duration: ['p(95)<800'], // fails the whole k6 run (non-zero exit code) if p95 >= 800ms
    http_req_failed: ['rate<0.01'],   // fails the run if error rate >= 1%
  },
};

export default function () {
  const res = http.post(
    'http://localhost:8000/predict',
    JSON.stringify({ text: 'some sample ticket text' }),
    { headers: { 'Content-Type': 'application/json' } }
  );
  check(res, { 'status is 200': (r) => r.status === 200 });
  sleep(Math.random() * 1.5 + 0.5);
}

// Run it with: k6 run k6_script_reference.js
// Notice `thresholds` above: this is why k6 shows up in CI pipelines more
// often than Locust -- a CI step can just be `k6 run ... || exit 1` and
// the pass/fail is baked into the script, no separate log-parsing needed.
