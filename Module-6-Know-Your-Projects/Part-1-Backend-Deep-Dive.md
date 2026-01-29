# Part 1: Backend Deep Dive

> **Your projects use**: Express.js (LAIPath, PlanLift), Flask (Drug-Likeness), Next.js API Routes (SoulThread)

---

## Table of Contents
1. [What is a Backend?](#1-what-is-a-backend)
2. [How the Internet Works](#2-how-the-internet-works)
3. [Express.js Deep Dive](#3-expressjs-deep-dive)
4. [Flask Deep Dive](#4-flask-deep-dive)
5. [Next.js API Routes](#5-nextjs-api-routes)
6. [REST API Design](#6-rest-api-design)
7. [Middleware Explained](#7-middleware-explained)
8. [Authentication & Security](#8-authentication--security)
9. [Your Projects' Backend Architecture](#9-your-projects-backend-architecture)

---

## 1. What is a Backend?

### The Restaurant Analogy

Think of a web application like a restaurant:

```
FRONTEND (Dining Area)     BACKEND (Kitchen)          DATABASE (Storage)
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│                 │       │                 │       │                 │
│  Customer sees  │ ───►  │  Chef processes │ ───►  │  Ingredients    │
│  the menu, UI   │       │  the order      │       │  are stored     │
│                 │ ◄───  │  Prepares food  │ ◄───  │  Retrieved      │
│                 │       │                 │       │                 │
└─────────────────┘       └─────────────────┘       └─────────────────┘
     (React)                  (Express)               (PostgreSQL)
```

**Backend responsibilities:**
- Process requests from the frontend
- Handle business logic (calculations, validations)
- Communicate with databases
- Integrate with external services (OpenAI, Cloudinary)
- Send responses back to the frontend

### Interview Answer:
> "The backend is the server-side of an application that handles business logic, data processing, and communication with databases. In my projects, I used Express.js for LAIPath and PlanLift, Flask for the Drug-Likeness Predictor, and Next.js API routes for SoulThread."

---

## 2. How the Internet Works

### The Request-Response Cycle

When a user clicks a button in your app:

```
1. USER ACTION
   User clicks "Generate Newsletter" button

2. FRONTEND CREATES REQUEST
   fetch('/api/generate', {
     method: 'POST',
     body: JSON.stringify({ topic: 'AI trends' })
   })

3. REQUEST TRAVELS TO SERVER
   Browser → DNS → Internet → Your Server (Vercel)

4. SERVER PROCESSES REQUEST
   Express receives request → Validates data → Calls OpenAI → Gets response

5. SERVER SENDS RESPONSE
   return res.json({ newsletter: '...' })

6. FRONTEND DISPLAYS RESULT
   User sees the generated newsletter
```

### HTTP Methods (CRUD Operations)

| Method | Purpose | Example | Your Usage |
|--------|---------|---------|------------|
| **GET** | Read data | Get user profile | Fetch voice profile |
| **POST** | Create data | Create newsletter | Generate content |
| **PUT** | Update entire resource | Replace user data | Update profile |
| **PATCH** | Update partial resource | Change email only | Partial updates |
| **DELETE** | Remove data | Delete draft | Remove content |

### HTTP Status Codes You Should Know

```javascript
// Success codes
200 OK          // Request successful
201 Created     // Resource created (after POST)
204 No Content  // Success but no data to return

// Client error codes
400 Bad Request    // Invalid data sent
401 Unauthorized   // Not logged in
403 Forbidden      // Logged in but not allowed
404 Not Found      // Resource doesn't exist
429 Too Many Requests // Rate limited

// Server error codes
500 Internal Server Error // Server crashed
503 Service Unavailable   // Server overloaded
```

### Interview Question: "What happens when you type a URL?"

> "When you type a URL, the browser first checks its cache, then queries DNS to convert the domain to an IP address. It then establishes a TCP connection (and TLS for HTTPS), sends an HTTP request, receives the response, and renders the HTML. For my SoulThread app on Vercel, requests go through Vercel's edge network, which routes to the nearest server for low latency."

---

## 3. Express.js Deep Dive

### What is Express.js?

Express is a **minimal web framework for Node.js**. It provides:
- Routing (handling different URLs)
- Middleware (processing requests)
- Easy request/response handling

### Basic Express Server Structure

```javascript
// server.js - Basic Express setup
const express = require('express');  // Import Express
const app = express();               // Create Express application
const PORT = 4000;

// MIDDLEWARE - Runs on EVERY request
app.use(express.json());  // Parse JSON bodies

// ROUTE - Handle specific URL
app.get('/', (req, res) => {
  res.send('Hello World');
});

// START SERVER
app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});
```

### Understanding Routes

A route is: **HTTP Method + URL Path + Handler Function**

```javascript
// Route structure
app.METHOD(PATH, HANDLER);

// Examples from YOUR projects:

// LAIPath - Generate syllabus
app.post('/api/syllabus', async (req, res) => {
  const { goal, hoursPerDay } = req.body;  // Get data from request
  const syllabus = await generateSyllabus(goal, hoursPerDay);
  res.json(syllabus);  // Send response
});

// PlanLift - Upload and render blueprint
app.post('/api/render', upload.single('file'), async (req, res) => {
  const file = req.file;           // Uploaded file from multer
  const style = req.body.style;    // Form data
  const renderUrl = await processBlueprint(file, style);
  res.json({ image_url: renderUrl });
});
```

### The Request Object (req)

```javascript
app.post('/api/generate', (req, res) => {
  // req.body - Data sent in POST request body
  const { topic, tone } = req.body;

  // req.params - URL parameters
  // Route: /api/users/:id
  // URL: /api/users/123
  const userId = req.params.id;  // "123"

  // req.query - Query string parameters
  // URL: /api/search?term=react&page=2
  const searchTerm = req.query.term;  // "react"
  const page = req.query.page;        // "2"

  // req.headers - HTTP headers
  const authToken = req.headers.authorization;

  // req.file - Uploaded file (with multer)
  const uploadedFile = req.file;
});
```

### The Response Object (res)

```javascript
app.get('/api/data', (req, res) => {
  // res.json() - Send JSON response
  res.json({ message: 'Success', data: [...] });

  // res.status() - Set HTTP status code
  res.status(201).json({ message: 'Created' });

  // res.send() - Send text/HTML response
  res.send('Hello World');

  // res.redirect() - Redirect to another URL
  res.redirect('/login');

  // Chaining status and json
  return res.status(400).json({ error: 'Invalid input' });
});
```

### Interview Question: "Explain how Express handles a request"

> "When a request comes in, Express matches it against defined routes in order. Before reaching the route handler, it passes through middleware functions like body parsers, authentication checks, and CORS handlers. Each middleware can either pass control to the next function using `next()` or end the request-response cycle. Once the route handler processes the request, it sends back a response using methods like `res.json()` or `res.send()`."

---

## 4. Flask Deep Dive

### What is Flask?

Flask is a **lightweight Python web framework**. You used it in your Drug-Likeness Predictor.

### Basic Flask Structure

```python
# app.py - Basic Flask setup
from flask import Flask, request, jsonify

app = Flask(__name__)  # Create Flask application

# ROUTE - Handle specific URL
@app.route('/')
def home():
    return 'Hello World'

# POST route with JSON data
@app.route('/api/predict', methods=['POST'])
def predict():
    data = request.get_json()        # Get JSON from request body
    smiles = data.get('smiles')      # Extract SMILES string
    prediction = model.predict(smiles)  # Run ML model
    return jsonify({'prediction': prediction})  # Return JSON

# Run the server
if __name__ == '__main__':
    app.run(debug=True, port=5000)
```

### Flask vs Express Comparison

| Feature | Flask (Python) | Express (Node.js) |
|---------|---------------|-------------------|
| **Create app** | `app = Flask(__name__)` | `const app = express()` |
| **Define route** | `@app.route('/path')` | `app.get('/path', handler)` |
| **Get JSON body** | `request.get_json()` | `req.body` |
| **Send JSON** | `jsonify(data)` | `res.json(data)` |
| **Run server** | `app.run(port=5000)` | `app.listen(5000)` |

### Your Drug-Likeness Predictor Backend

```python
# Simplified version of your Flask backend
from flask import Flask, request, jsonify, render_template
from rdkit import Chem
from rdkit.Chem import Draw
import tensorflow as tf

app = Flask(__name__)

# Load pre-trained model once at startup
model = tf.keras.models.load_model('model.h5')

@app.route('/')
def home():
    return render_template('index.html')  # Serve HTML page

@app.route('/predict', methods=['POST'])
def predict():
    # 1. Get SMILES string from form
    smiles = request.form.get('smiles')

    # 2. Validate SMILES (is it a real molecule?)
    mol = Chem.MolFromSmiles(smiles)
    if mol is None:
        return jsonify({'error': 'Invalid SMILES'})

    # 3. Convert SMILES to model input (one-hot encoding)
    encoded = encode_smiles(smiles)  # Your preprocessing function

    # 4. Run prediction
    prediction = model.predict(encoded)
    probability = float(prediction[0][0])

    # 5. Generate molecule image
    img = Draw.MolToImage(mol)

    # 6. Return results
    return jsonify({
        'drug_likeness': probability,
        'is_drug_like': probability > 0.5
    })
```

### Interview Question: "Why did you choose Flask for this project?"

> "I chose Flask for the Drug-Likeness Predictor because it's Python-based, which integrates seamlessly with TensorFlow and RDKit - the ML and chemistry libraries I needed. Flask is also lightweight and perfect for serving a single-purpose ML model without the overhead of a larger framework like Django."

---

## 5. Next.js API Routes

### What are API Routes?

Next.js lets you create backend endpoints **inside your frontend project**. No separate server needed.

```
your-nextjs-app/
├── app/
│   ├── page.tsx           # Frontend page
│   └── api/
│       └── generate/
│           └── route.ts   # Backend endpoint: /api/generate
```

### How They Work

```typescript
// app/api/generate/route.ts (SoulThread)
import { NextResponse } from 'next/server';

export async function POST(request: Request) {
  try {
    // 1. Get data from request
    const body = await request.json();
    const { topic, voiceProfile } = body;

    // 2. Process (call OpenAI, database, etc.)
    const newsletter = await generateNewsletter(topic, voiceProfile);

    // 3. Return response
    return NextResponse.json({
      success: true,
      newsletter
    });

  } catch (error) {
    // 4. Handle errors
    return NextResponse.json(
      { error: 'Generation failed' },
      { status: 500 }
    );
  }
}

export async function GET(request: Request) {
  // Handle GET requests to /api/generate
  return NextResponse.json({ message: 'Use POST method' });
}
```

### API Routes vs Separate Backend

| Aspect | Next.js API Routes | Separate Express Server |
|--------|-------------------|------------------------|
| **Setup** | Built-in, no config | Separate project needed |
| **Deployment** | Single deploy | Two deployments |
| **Scaling** | Serverless (auto) | Manual configuration |
| **Best for** | Simple APIs, BFF | Complex backends |
| **Your usage** | SoulThread | LAIPath, PlanLift |

### Interview Question: "Why use API routes vs a separate backend?"

> "For SoulThread, I used Next.js API routes because the backend logic was relatively simple - mainly calling OpenAI and Supabase. This gave me a single deployment unit and serverless scaling. For PlanLift, I used a separate Express backend because image processing required more complex middleware like Multer for file uploads and longer-running operations that benefit from a dedicated server."

---

## 6. REST API Design

### What is REST?

REST (Representational State Transfer) is a set of conventions for designing web APIs.

### REST Principles You Should Know

**1. Use nouns, not verbs in URLs**
```
GOOD: GET /api/newsletters
BAD:  GET /api/getNewsletters
```

**2. Use HTTP methods for actions**
```
GET    /api/newsletters      # Get all newsletters
GET    /api/newsletters/123  # Get one newsletter
POST   /api/newsletters      # Create newsletter
PUT    /api/newsletters/123  # Update newsletter
DELETE /api/newsletters/123  # Delete newsletter
```

**3. Use plural nouns**
```
GOOD: /api/users
BAD:  /api/user
```

**4. Nest related resources**
```
GET /api/users/123/newsletters  # Get newsletters for user 123
```

### Your API Design Examples

```javascript
// LAIPath API endpoints
POST /api/syllabus           // Generate syllabus
POST /api/chat               // Topic-scoped AI chat
POST /api/evaluate           // Evaluate learning reflection
PUT  /api/syllabus/:id       // Update syllabus

// PlanLift API endpoints
POST /api/render             // Generate 3D render
GET  /health                 // Health check endpoint

// SoulThread API endpoints
POST /api/ai-generate        // Generate newsletter
POST /api/linkedin-generate  // Generate LinkedIn post
GET  /api/voice-profile      // Get user's voice profile
POST /api/voice-profile      // Save voice profile
```

---

## 7. Middleware Explained

### What is Middleware?

Middleware = Functions that run **between** receiving a request and sending a response.

```
Request → [Middleware 1] → [Middleware 2] → [Middleware 3] → Route Handler → Response
             CORS           Body Parser      Auth Check        Your Code
```

### Common Middleware in Your Projects

```javascript
// server.js - PlanLift backend
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const multer = require('multer');
const morgan = require('morgan');

const app = express();

// 1. MORGAN - Logging
// Logs every request: "POST /api/render 200 1532ms"
app.use(morgan('dev'));

// 2. HELMET - Security headers
// Adds headers like X-Frame-Options, Content-Security-Policy
app.use(helmet());

// 3. CORS - Cross-Origin Resource Sharing
// Allows your frontend (different domain) to call this API
app.use(cors({
  origin: ['https://planlift-frontend.vercel.app'],
  credentials: true
}));

// 4. RATE LIMITER - Prevent abuse
// Max 100 requests per 15 minutes per IP
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000,  // 15 minutes
  max: 100
});
app.use('/api/', limiter);

// 5. BODY PARSER - Parse JSON requests
// Converts JSON string to JavaScript object
app.use(express.json());

// 6. MULTER - File uploads
// Handles multipart/form-data (file uploads)
const upload = multer({
  storage: multer.memoryStorage(),
  limits: { fileSize: 10 * 1024 * 1024 }  // 10MB
});

// Route with multer middleware
app.post('/api/render', upload.single('file'), (req, res) => {
  // req.file now contains the uploaded file
});
```

### How Middleware Chain Works

```javascript
// Custom middleware example
const logRequest = (req, res, next) => {
  console.log(`${req.method} ${req.path}`);
  next();  // IMPORTANT: Pass control to next middleware
};

const checkAuth = (req, res, next) => {
  const token = req.headers.authorization;
  if (!token) {
    return res.status(401).json({ error: 'No token' });  // Stop here
  }
  next();  // Continue to next middleware
};

// Apply middleware
app.use(logRequest);  // Runs on ALL routes
app.use('/api/protected', checkAuth);  // Runs only on /api/protected/*
```

### Interview Question: "What middleware did you use and why?"

> "In PlanLift, I used several middleware layers: Helmet for security headers to prevent common attacks, CORS to allow my frontend to communicate with the backend, express-rate-limit to prevent API abuse at 100 requests per 15 minutes, Multer for handling file uploads of blueprints, and Morgan for request logging. Each middleware serves a specific security or functionality purpose."

---

## 8. Authentication & Security

### Authentication in Your Projects

**LAIPath & SoulThread use Supabase Auth:**

```typescript
// Frontend: User signs up
const { data, error } = await supabase.auth.signUp({
  email: 'user@example.com',
  password: 'securepassword'
});

// Frontend: User signs in
const { data, error } = await supabase.auth.signInWithPassword({
  email: 'user@example.com',
  password: 'securepassword'
});

// Frontend: Get current user
const { data: { user } } = await supabase.auth.getUser();

// Backend/API: Verify user from request
const authHeader = req.headers.authorization;
const token = authHeader?.split(' ')[1];
const { data: { user }, error } = await supabase.auth.getUser(token);
```

### Security Measures in PlanLift

```javascript
// 1. Helmet - Security headers
app.use(helmet());
// Sets: X-Frame-Options, X-Content-Type-Options, etc.

// 2. CORS - Whitelist allowed origins
const allowedOrigins = process.env.ALLOWED_ORIGINS.split(',');
app.use(cors({
  origin: (origin, callback) => {
    if (allowedOrigins.includes(origin)) {
      callback(null, true);
    } else {
      callback(new Error('Not allowed by CORS'));
    }
  }
}));

// 3. Input validation
const { body, validationResult } = require('express-validator');

app.post('/api/render',
  body('style').isString().trim().escape(),
  (req, res) => {
    const errors = validationResult(req);
    if (!errors.isEmpty()) {
      return res.status(400).json({ errors: errors.array() });
    }
    // Process valid request
  }
);

// 4. Error masking in production
app.use((err, req, res, next) => {
  res.status(err.status || 500).json({
    error: process.env.NODE_ENV === 'production'
      ? 'An error occurred'      // Hide details in production
      : err.message              // Show details in development
  });
});
```

### Interview Question: "How did you handle security?"

> "I implemented multiple security layers. For authentication, I used Supabase Auth which handles password hashing, sessions, and JWT tokens. On the API side, I used Helmet for security headers, implemented CORS whitelisting to only allow requests from my frontend domain, added rate limiting to prevent abuse, and used input validation with express-validator. In production, I also mask error messages to avoid exposing sensitive information."

---

## 9. Your Projects' Backend Architecture

### LAIPath Backend Flow

```
User Request: "Generate syllabus for learning React in 30 days"
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  EXPRESS SERVER (Port 4000)                                  │
│                                                              │
│  1. MIDDLEWARE CHAIN                                         │
│     cors() → express.json() → morgan()                      │
│                              │                               │
│  2. ROUTE: POST /api/syllabus                               │
│     ┌────────────────────────────────────────┐              │
│     │ // Validate input                       │              │
│     │ const { goal, hoursPerDay } = req.body │              │
│     │                                         │              │
│     │ // Safety check                         │              │
│     │ if (isUnsafeTopic(goal)) {             │              │
│     │   return res.status(400).json(...)     │              │
│     │ }                                       │              │
│     │                                         │              │
│     │ // Call OpenAI                          │              │
│     │ const syllabus = await openai.chat...   │              │
│     │                                         │              │
│     │ // Return result                        │              │
│     │ res.json({ syllabus })                  │              │
│     └────────────────────────────────────────┘              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
              Response: { syllabus: [...30 days...] }
```

### PlanLift Backend Flow

```
User uploads blueprint.png + selects "Modern" style
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  EXPRESS SERVER (Port 4000)                                  │
│                                                              │
│  1. MIDDLEWARE CHAIN                                         │
│     helmet() → cors() → rateLimit() → morgan()              │
│                              │                               │
│  2. ROUTE: POST /api/render                                  │
│     ┌────────────────────────────────────────┐              │
│     │ // Multer handles file upload          │              │
│     │ upload.single('file')                  │              │
│     │                                         │              │
│     │ // File available as req.file          │              │
│     │ const imageBuffer = req.file.buffer    │              │
│     │ const style = req.body.style           │              │
│     │                                         │              │
│     │ // Upload to Cloudinary                 │              │
│     │ const cloudinaryUrl = await upload...   │──────────┐  │
│     │                                         │          │  │
│     │ // Call Replicate AI                    │          │  │
│     │ const render = await replicate.run(...) │──────────┤  │
│     │                                         │          │  │
│     │ // Return render URL                    │          │  │
│     │ res.json({ image_url: render })         │          │  │
│     └────────────────────────────────────────┘          │  │
└─────────────────────────────────────────────────────────│──┘
                                                          │
                    ┌─────────────────────────────────────┘
                    │
                    ▼
        ┌──────────────────┐    ┌──────────────────┐
        │   CLOUDINARY     │    │   REPLICATE AI   │
        │   Image CDN      │    │   qwen-image-edit│
        │   Storage        │───▶│   3D Rendering   │
        └──────────────────┘    └──────────────────┘
```

### SoulThread Backend Flow (Next.js API Routes)

```
User clicks "Generate Newsletter"
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  NEXT.JS API ROUTE: /api/ai-generate                         │
│                                                              │
│  export async function POST(request: Request) {              │
│    // 1. Parse request body                                  │
│    const { topic, voiceProfile } = await request.json()     │
│                                                              │
│    // 2. Fetch trending topics (with fallback)              │
│    const trends = await fetchTrends()  // Reddit, HN, etc.  │
│         │                                                    │
│         ├── Try: Reddit API                                  │
│         ├── Fallback: Hacker News API                       │
│         ├── Fallback: Curated Trends                        │
│         └── Final: Mock Data (guaranteed)                   │
│                                                              │
│    // 3. Generate newsletter                                 │
│    if (process.env.OPENAI_ENABLED) {                        │
│      newsletter = await generateWithAI(trends, voiceProfile)│
│    } else {                                                  │
│      newsletter = generateWithTemplate(trends, voiceProfile)│
│    }                                                         │
│                                                              │
│    // 4. Return response                                     │
│    return NextResponse.json({ newsletter })                  │
│  }                                                           │
└─────────────────────────────────────────────────────────────┘
```

---

## Quick Reference: Backend Interview Answers

### "What is Express.js?"
> "Express is a minimal Node.js web framework that provides routing, middleware support, and easy request/response handling. I used it for LAIPath and PlanLift backends."

### "What is middleware?"
> "Middleware are functions that execute between receiving a request and sending a response. They can modify the request, validate data, handle authentication, or end the request early. In my projects, I used middleware for CORS, security headers, rate limiting, and file uploads."

### "How does your API handle errors?"
> "I implemented centralized error handling with try-catch blocks in route handlers and a global error middleware. In production, I mask error details to avoid exposing sensitive information, while in development I show full error messages for debugging."

### "Why did you separate frontend and backend in PlanLift?"
> "I separated them for independent scaling - the AI processing is compute-intensive and might need different scaling than the frontend. It also allows different deployment configurations and easier maintenance."

---

Next: [Part-2-Frontend-Deep-Dive.md](./Part-2-Frontend-Deep-Dive.md)
