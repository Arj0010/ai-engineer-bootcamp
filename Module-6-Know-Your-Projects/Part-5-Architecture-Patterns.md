# Part 5: Architecture Patterns

> **Folder structures, system design, and architectural decisions in your projects**

---

## Table of Contents
1. [Project Folder Structures](#1-project-folder-structures)
2. [Frontend Architecture](#2-frontend-architecture)
3. [Backend Architecture](#3-backend-architecture)
4. [API Design Patterns](#4-api-design-patterns)
5. [Error Handling Patterns](#5-error-handling-patterns)
6. [Environment & Configuration](#6-environment--configuration)
7. [Full System Architectures](#7-full-system-architectures)

---

## 1. Project Folder Structures

### Next.js Project (SoulThread/PlanLift Frontend)

```
project/
├── app/                        # App Router (Next.js 13+)
│   ├── layout.tsx             # Root layout (header, footer, providers)
│   ├── page.tsx               # Home page (/)
│   ├── globals.css            # Global styles
│   │
│   ├── (auth)/                # Route group (doesn't affect URL)
│   │   ├── login/
│   │   │   └── page.tsx      # /login
│   │   └── register/
│   │       └── page.tsx      # /register
│   │
│   ├── dashboard/
│   │   ├── layout.tsx        # Dashboard-specific layout
│   │   └── page.tsx          # /dashboard
│   │
│   └── api/                   # API routes (backend endpoints)
│       ├── generate/
│       │   └── route.ts      # POST /api/generate
│       └── voice-profile/
│           └── route.ts      # GET/POST /api/voice-profile
│
├── components/                 # Reusable UI components
│   ├── ui/                    # Base components (Button, Input, Card)
│   │   ├── Button.tsx
│   │   ├── Input.tsx
│   │   └── Card.tsx
│   ├── NewsletterGenerator.tsx
│   ├── VoiceProfileCard.tsx
│   └── Header.tsx
│
├── lib/                        # Utility functions and clients
│   ├── supabase.ts           # Supabase client setup
│   ├── openai.ts             # OpenAI client setup
│   └── utils.ts              # Helper functions
│
├── hooks/                      # Custom React hooks
│   ├── useAuth.ts
│   └── useVoiceProfile.ts
│
├── types/                      # TypeScript type definitions
│   ├── index.ts
│   └── api.ts
│
├── public/                     # Static files (images, fonts)
│   └── logo.png
│
├── .env.local                  # Environment variables (gitignored)
├── next.config.js              # Next.js configuration
├── tailwind.config.js          # Tailwind configuration
├── tsconfig.json               # TypeScript configuration
└── package.json                # Dependencies and scripts
```

**Why this structure?**
- `app/` - Next.js 13+ convention for file-based routing
- `components/` - Reusable pieces separated from pages
- `lib/` - Non-React utilities, API clients
- `hooks/` - Custom hooks for shared logic
- `types/` - Central type definitions

---

### Express Backend (PlanLift/LAIPath)

```
backend/
├── src/
│   ├── index.js               # Entry point, server setup
│   ├── routes/
│   │   ├── index.js          # Route aggregator
│   │   ├── render.js         # /api/render routes
│   │   └── health.js         # /health route
│   │
│   ├── middleware/
│   │   ├── auth.js           # Authentication middleware
│   │   ├── upload.js         # Multer configuration
│   │   ├── validation.js     # Input validation
│   │   └── errorHandler.js   # Global error handler
│   │
│   ├── services/
│   │   ├── cloudinary.js     # Cloudinary upload logic
│   │   ├── replicate.js      # AI model calls
│   │   └── openai.js         # OpenAI API calls
│   │
│   ├── config/
│   │   ├── index.js          # Configuration loader
│   │   └── aiConfig.js       # AI-specific config (tokens, models)
│   │
│   └── utils/
│       ├── logger.js         # Logging utility
│       └── helpers.js        # Helper functions
│
├── .env                        # Environment variables
├── vercel.json                 # Vercel deployment config
└── package.json
```

**Why this structure?**
- **Separation of concerns**: Routes, middleware, services are separate
- **Services layer**: Business logic isolated from HTTP handling
- **Config layer**: Centralized configuration management

---

### Flask Backend (Drug-Likeness Predictor)

```
drug-likeness-predictor/
├── app.py                      # Main Flask application
├── model/
│   ├── model.h5               # Trained TensorFlow model
│   ├── tokenizer.pkl          # Saved tokenizer
│   └── train.py               # Training script
│
├── utils/
│   ├── preprocessing.py       # SMILES encoding functions
│   ├── visualization.py       # Molecule drawing
│   └── validation.py          # Input validation
│
├── templates/
│   └── index.html             # Frontend HTML
│
├── static/
│   ├── css/
│   │   └── styles.css
│   └── js/
│       └── main.js
│
├── data/
│   └── zinc_dataset.csv       # Training data
│
├── requirements.txt            # Python dependencies
└── README.md
```

---

### Interview Question: "Explain your folder structure"

> "I organize my projects by concern. In the Next.js frontend, pages live in the app directory following file-based routing, reusable components are in components/, shared utilities in lib/, and type definitions in types/. For the Express backend, I separate routes from business logic - routes handle HTTP concerns while services contain the actual logic. This makes the codebase maintainable and testable."

---

## 2. Frontend Architecture

### Component Design Patterns

**1. Container/Presentational Pattern**

```tsx
// CONTAINER: Handles logic and data
function NewsletterGeneratorContainer() {
  const [topic, setTopic] = useState('');
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);

  const handleGenerate = async () => {
    setLoading(true);
    const data = await api.generate(topic);
    setResult(data);
    setLoading(false);
  };

  // Pass data and handlers to presentational component
  return (
    <NewsletterGeneratorUI
      topic={topic}
      onTopicChange={setTopic}
      onGenerate={handleGenerate}
      result={result}
      loading={loading}
    />
  );
}

// PRESENTATIONAL: Just renders UI
function NewsletterGeneratorUI({ topic, onTopicChange, onGenerate, result, loading }) {
  return (
    <div>
      <input value={topic} onChange={(e) => onTopicChange(e.target.value)} />
      <button onClick={onGenerate} disabled={loading}>
        {loading ? 'Generating...' : 'Generate'}
      </button>
      {result && <div>{result}</div>}
    </div>
  );
}
```

**2. Custom Hooks Pattern**

```tsx
// Extract logic into reusable hook
function useNewsletterGenerator() {
  const [topic, setTopic] = useState('');
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  const generate = async () => {
    setLoading(true);
    setError(null);
    try {
      const data = await api.generate(topic);
      setResult(data);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return { topic, setTopic, result, loading, error, generate };
}

// Use in any component
function NewsletterGenerator() {
  const { topic, setTopic, result, loading, generate } = useNewsletterGenerator();
  // Render UI...
}
```

**3. Context for Global State**

```tsx
// contexts/AuthContext.tsx
const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Check session on mount
    supabase.auth.getSession().then(({ data: { session } }) => {
      setUser(session?.user ?? null);
      setLoading(false);
    });

    // Listen for auth changes
    const { data: { subscription } } = supabase.auth.onAuthStateChange(
      (event, session) => {
        setUser(session?.user ?? null);
      }
    );

    return () => subscription.unsubscribe();
  }, []);

  return (
    <AuthContext.Provider value={{ user, loading }}>
      {children}
    </AuthContext.Provider>
  );
}

export const useAuth = () => useContext(AuthContext);
```

---

### Data Flow Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    REACT APPLICATION                         │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                 CONTEXT PROVIDERS                    │    │
│  │  ┌──────────┐  ┌──────────────┐  ┌──────────────┐  │    │
│  │  │AuthContext│  │VoiceContext │  │ThemeContext  │  │    │
│  │  └──────────┘  └──────────────┘  └──────────────┘  │    │
│  └─────────────────────────────────────────────────────┘    │
│                           │                                  │
│                           ▼                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                      PAGES                           │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │    │
│  │  │Dashboard │  │Community │  │Profile           │  │    │
│  │  └────┬─────┘  └────┬─────┘  └────────┬─────────┘  │    │
│  └───────│─────────────│────────────────│─────────────┘    │
│          │             │                │                    │
│          ▼             ▼                ▼                    │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                   COMPONENTS                         │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │    │
│  │  │NewsletterGen│  │CommunityFeed│  │VoiceProfile│  │    │
│  │  │ - useState  │  │ - useFetch  │  │ - useForm  │  │    │
│  │  │ - handlers  │  │ - handlers  │  │ - handlers │  │    │
│  │  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘  │    │
│  └─────────│────────────────│───────────────│─────────┘    │
│            │                │               │                │
└────────────│────────────────│───────────────│────────────────┘
             │                │               │
             ▼                ▼               ▼
┌────────────────────────────────────────────────────────────┐
│                      API LAYER (fetch/axios)                │
│                                                             │
│  POST /api/generate    GET /api/community    PUT /api/voice │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Backend Architecture

### Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       ROUTES LAYER                           │
│         Handles HTTP requests, validates input               │
│   ┌─────────────────────────────────────────────────────┐   │
│   │  app.post('/api/render', validate, renderController) │   │
│   └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    CONTROLLER LAYER                          │
│           Orchestrates the request processing                │
│   ┌─────────────────────────────────────────────────────┐   │
│   │  async function renderController(req, res) {         │   │
│   │    const imageUrl = await cloudinaryService.upload() │   │
│   │    const render = await replicateService.generate()  │   │
│   │    res.json({ image_url: render })                   │   │
│   │  }                                                   │   │
│   └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      SERVICE LAYER                           │
│              Contains business logic                         │
│   ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   │
│   │ cloudinaryServ│  │ replicateServ │  │  openaiServ   │   │
│   │ - upload()    │  │ - generate()  │  │ - complete()  │   │
│   │ - delete()    │  │ - getStatus() │  │ - embed()     │   │
│   └───────────────┘  └───────────────┘  └───────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     EXTERNAL SERVICES                        │
│   ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   │
│   │   Cloudinary  │  │   Replicate   │  │    OpenAI     │   │
│   └───────────────┘  └───────────────┘  └───────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Middleware Chain

```javascript
// Order matters! Middleware executes in order defined

const app = express();

// 1. Logging (first - logs all requests)
app.use(morgan('dev'));

// 2. Security headers
app.use(helmet());

// 3. CORS (must be before routes)
app.use(cors({ origin: allowedOrigins }));

// 4. Rate limiting
app.use('/api/', rateLimit({ windowMs: 15 * 60 * 1000, max: 100 }));

// 5. Body parsing
app.use(express.json());

// 6. Routes (the actual endpoints)
app.use('/api/render', renderRoutes);
app.use('/api/syllabus', syllabusRoutes);

// 7. 404 handler (no route matched)
app.use((req, res) => {
  res.status(404).json({ error: 'Not found' });
});

// 8. Error handler (catches all errors)
app.use((err, req, res, next) => {
  console.error(err);
  res.status(500).json({ error: 'Internal server error' });
});
```

---

## 4. API Design Patterns

### RESTful Conventions

```
Resource-based URLs with HTTP methods:

GET    /api/drafts          # List all drafts
GET    /api/drafts/:id      # Get single draft
POST   /api/drafts          # Create draft
PUT    /api/drafts/:id      # Update entire draft
PATCH  /api/drafts/:id      # Update partial draft
DELETE /api/drafts/:id      # Delete draft

Nested resources:
GET    /api/users/:userId/drafts     # Get user's drafts
POST   /api/drafts/:draftId/comments # Add comment to draft
```

### Request/Response Patterns

```javascript
// SUCCESS RESPONSES
// 200 OK - Successful read/update
res.status(200).json({
  success: true,
  data: { id: 1, title: 'Newsletter' }
});

// 201 Created - Successful creation
res.status(201).json({
  success: true,
  data: { id: 2, title: 'New Draft' }
});

// 204 No Content - Successful delete
res.status(204).send();

// ERROR RESPONSES
// 400 Bad Request - Invalid input
res.status(400).json({
  success: false,
  error: 'Validation failed',
  details: [{ field: 'email', message: 'Invalid email format' }]
});

// 401 Unauthorized - Not logged in
res.status(401).json({
  success: false,
  error: 'Authentication required'
});

// 404 Not Found - Resource doesn't exist
res.status(404).json({
  success: false,
  error: 'Draft not found'
});

// 500 Internal Server Error - Server problem
res.status(500).json({
  success: false,
  error: process.env.NODE_ENV === 'production'
    ? 'Internal server error'
    : err.message
});
```

### Your API Patterns

```javascript
// LAIPath - Syllabus generation
POST /api/syllabus
Request:  { goal: "Learn React", hoursPerDay: 2, totalDays: 30 }
Response: { success: true, syllabus: [...days] }

// LAIPath - Chat with scope validation
POST /api/chat
Request:  { question: "What is useState?", topic: "React Hooks", subtasks: [...] }
Response: { success: true, answer: "...", isOnTopic: true }

// PlanLift - Generate render
POST /api/render
Request:  FormData { file: <blueprint.png>, style: "modern" }
Response: { success: true, image_url: "https://..." }

// SoulThread - Save voice profile
POST /api/voice-profile
Request:  { topics: ["AI"], tone: "professional", feeling: "enthusiastic" }
Response: { success: true, data: { id: "...", ...profile } }
```

---

## 5. Error Handling Patterns

### Frontend Error Handling

```tsx
// 1. Try-catch in async functions
async function fetchData() {
  try {
    const response = await fetch('/api/data');

    if (!response.ok) {
      throw new Error(`HTTP error: ${response.status}`);
    }

    const data = await response.json();
    setData(data);
  } catch (error) {
    setError(error.message);
    console.error('Fetch failed:', error);
  } finally {
    setLoading(false);
  }
}

// 2. Error Boundary for React errors
class ErrorBoundary extends React.Component {
  state = { hasError: false };

  static getDerivedStateFromError(error) {
    return { hasError: true };
  }

  componentDidCatch(error, errorInfo) {
    console.error('React error:', error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return <h1>Something went wrong.</h1>;
    }
    return this.props.children;
  }
}

// Usage
<ErrorBoundary>
  <App />
</ErrorBoundary>
```

### Backend Error Handling

```javascript
// 1. Route-level try-catch
app.post('/api/render', async (req, res, next) => {
  try {
    const result = await processRender(req.file);
    res.json({ success: true, data: result });
  } catch (error) {
    next(error);  // Pass to error handler
  }
});

// 2. Async wrapper (cleaner)
const asyncHandler = (fn) => (req, res, next) => {
  Promise.resolve(fn(req, res, next)).catch(next);
};

app.post('/api/render', asyncHandler(async (req, res) => {
  const result = await processRender(req.file);
  res.json({ success: true, data: result });
}));

// 3. Global error handler
app.use((err, req, res, next) => {
  console.error(err.stack);

  // Operational errors (expected)
  if (err.isOperational) {
    return res.status(err.statusCode).json({
      success: false,
      error: err.message
    });
  }

  // Programming errors (unexpected)
  res.status(500).json({
    success: false,
    error: process.env.NODE_ENV === 'production'
      ? 'An unexpected error occurred'
      : err.message
  });
});

// 4. Custom error class
class AppError extends Error {
  constructor(message, statusCode) {
    super(message);
    this.statusCode = statusCode;
    this.isOperational = true;
  }
}

// Usage
throw new AppError('Draft not found', 404);
```

### Your SoulThread Fallback Pattern

```javascript
// Multi-level fallback for reliability
async function fetchTrends() {
  // Level 1: Try live APIs
  try {
    const reddit = await fetchReddit();
    if (reddit?.length > 0) return reddit;
  } catch (e) {
    console.log('Reddit failed, trying fallback');
  }

  try {
    const hn = await fetchHackerNews();
    if (hn?.length > 0) return hn;
  } catch (e) {
    console.log('HN failed, trying fallback');
  }

  // Level 2: Curated data
  try {
    const curated = await loadCuratedTrends();
    if (curated?.length > 0) return curated;
  } catch (e) {
    console.log('Curated failed, using mock');
  }

  // Level 3: Mock data (guaranteed)
  return getMockTrends();  // Always returns valid data
}
```

---

## 6. Environment & Configuration

### Environment Variables Structure

```bash
# .env.local (Frontend - Next.js)
# Only NEXT_PUBLIC_* are exposed to browser
NEXT_PUBLIC_SUPABASE_URL=https://xxx.supabase.co
NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJ...
NEXT_PUBLIC_API_URL=https://api.example.com

# Server-only (not exposed to browser)
SUPABASE_SERVICE_KEY=eyJ...  # Full access key
OPENAI_API_KEY=sk-...

# .env (Backend - Express)
NODE_ENV=production
PORT=4000

# API Keys
OPENAI_API_KEY=sk-...
REPLICATE_API_TOKEN=r8_...
CLOUDINARY_CLOUD_NAME=xxx
CLOUDINARY_API_KEY=xxx
CLOUDINARY_API_SECRET=xxx

# Security
ALLOWED_ORIGINS=https://frontend.vercel.app
```

### Configuration Pattern

```javascript
// config/index.js
const config = {
  env: process.env.NODE_ENV || 'development',
  port: parseInt(process.env.PORT, 10) || 4000,

  openai: {
    apiKey: process.env.OPENAI_API_KEY,
    model: process.env.OPENAI_MODEL || 'gpt-4o-mini',
    maxTokens: parseInt(process.env.OPENAI_MAX_TOKENS, 10) || 900,
  },

  replicate: {
    apiToken: process.env.REPLICATE_API_TOKEN,
    model: process.env.RENDER_MODEL || 'qwen/qwen-image-edit',
  },

  cloudinary: {
    cloudName: process.env.CLOUDINARY_CLOUD_NAME,
    apiKey: process.env.CLOUDINARY_API_KEY,
    apiSecret: process.env.CLOUDINARY_API_SECRET,
  },

  cors: {
    allowedOrigins: process.env.ALLOWED_ORIGINS?.split(',') || [],
  },
};

// Validate required config
const required = ['openai.apiKey', 'replicate.apiToken'];
for (const key of required) {
  const value = key.split('.').reduce((obj, k) => obj?.[k], config);
  if (!value) {
    throw new Error(`Missing required config: ${key}`);
  }
}

module.exports = config;
```

---

## 7. Full System Architectures

### LAIPath Complete Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                           USER BROWSER                               │
│                                                                      │
│   ┌──────────────────────────────────────────────────────────────┐  │
│   │                    REACT FRONTEND (Vite)                      │  │
│   │  ┌────────────┐  ┌────────────┐  ┌────────────┐              │  │
│   │  │   Landing  │  │  Calendar  │  │   Daily    │              │  │
│   │  │   Page     │  │   View     │  │  Learning  │              │  │
│   │  └────────────┘  └────────────┘  └────────────┘              │  │
│   │         │               │               │                     │  │
│   │         └───────────────┴───────────────┘                     │  │
│   │                         │                                      │  │
│   │               ┌─────────▼─────────┐                           │  │
│   │               │   Supabase Client │                           │  │
│   │               │   (Auth + Data)   │                           │  │
│   │               └─────────┬─────────┘                           │  │
│   └─────────────────────────│────────────────────────────────────┘  │
└─────────────────────────────│────────────────────────────────────────┘
                              │
         ┌────────────────────┼────────────────────┐
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│   EXPRESS API   │  │    SUPABASE     │  │    OPENAI API   │
│   (Port 4000)   │  │   (PostgreSQL)  │  │                 │
│                 │  │                 │  │                 │
│ /api/syllabus   │  │  - auth.users   │  │  - GPT-4o-mini  │
│ /api/chat       │  │  - syllabus     │  │  - Embeddings   │
│ /api/evaluate   │  │  - reflections  │  │                 │
└────────┬────────┘  └─────────────────┘  └────────┬────────┘
         │                                          │
         └──────────────────┬───────────────────────┘
                            │
                  ┌─────────▼─────────┐
                  │      VERCEL       │
                  │   (Deployment)    │
                  │                   │
                  │  - Frontend CDN   │
                  │  - API Serverless │
                  └───────────────────┘
```

### PlanLift Complete Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                           USER BROWSER                               │
│                                                                      │
│   ┌──────────────────────────────────────────────────────────────┐  │
│   │                  NEXT.JS FRONTEND (Port 3000)                 │  │
│   │                                                               │  │
│   │  User uploads blueprint.png → Selects "Modern" → Clicks Gen  │  │
│   │                              │                                │  │
│   │                    POST /api/render                           │  │
│   │                    FormData: { file, style }                  │  │
│   │                              │                                │  │
│   └──────────────────────────────│────────────────────────────────┘  │
└──────────────────────────────────│────────────────────────────────────┘
                                   │
                                   ▼
┌──────────────────────────────────────────────────────────────────────┐
│                      EXPRESS BACKEND (Port 4000)                      │
│                                                                       │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │                     MIDDLEWARE CHAIN                             │ │
│  │  morgan → helmet → cors → rateLimit → express.json → multer     │ │
│  └─────────────────────────────────────────────────────────────────┘ │
│                                   │                                   │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │                   POST /api/render HANDLER                       │ │
│  │                                                                  │ │
│  │  1. Validate file (multer)                                       │ │
│  │  2. Upload to Cloudinary  ──────────────────────────────────┐   │ │
│  │  3. Get secure URL         ◄────────────────────────────────┘   │ │
│  │  4. Call Replicate API    ──────────────────────────────────┐   │ │
│  │  5. Return render URL      ◄────────────────────────────────┘   │ │
│  │                                                                  │ │
│  └─────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────┘
         │                                         │
         ▼                                         ▼
┌─────────────────────┐                 ┌─────────────────────┐
│     CLOUDINARY      │                 │    REPLICATE API    │
│                     │                 │                     │
│  - Store blueprint  │    image URL    │  - qwen-image-edit  │
│  - Return HTTPS URL │ ───────────────▶│  - 3D render gen    │
│  - CDN delivery     │                 │  - Return result    │
└─────────────────────┘                 └─────────────────────┘
```

### SoulThread Complete Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│                    NEXT.JS 16 APPLICATION                             │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                        APP ROUTER                               │  │
│  │                                                                 │  │
│  │  ┌─────────┐  ┌───────────┐  ┌───────────┐  ┌──────────────┐  │  │
│  │  │ Landing │  │ Dashboard │  │ Community │  │   Profile    │  │  │
│  │  │  page   │  │   page    │  │   page    │  │    page      │  │  │
│  │  └─────────┘  └─────┬─────┘  └─────┬─────┘  └──────┬───────┘  │  │
│  │                     │              │               │           │  │
│  └─────────────────────│──────────────│───────────────│───────────┘  │
│                        │              │               │              │
│  ┌─────────────────────│──────────────│───────────────│───────────┐  │
│  │                     ▼              ▼               ▼            │  │
│  │                    API ROUTES (Serverless)                      │  │
│  │  ┌────────────────┐ ┌──────────────────┐ ┌───────────────────┐ │  │
│  │  │/api/ai-generate│ │/api/linkedin-gen │ │/api/voice-profile │ │  │
│  │  │                │ │                  │ │                   │ │  │
│  │  │ 1. Get voice   │ │ 1. Get voice     │ │ 1. Upsert to      │ │  │
│  │  │ 2. Fetch trends│ │ 2. Generate post │ │    Supabase       │ │  │
│  │  │ 3. Template gen│ │ 3. Return        │ │ 2. Return data    │ │  │
│  │  │ 4. (or AI gen) │ │                  │ │                   │ │  │
│  │  └───────┬────────┘ └────────┬─────────┘ └─────────┬─────────┘ │  │
│  └──────────│───────────────────│─────────────────────│───────────┘  │
└─────────────│───────────────────│─────────────────────│──────────────┘
              │                   │                     │
    ┌─────────┼───────────────────┼─────────────────────┼─────────┐
    │         ▼                   ▼                     ▼         │
    │  ┌─────────────┐    ┌─────────────┐      ┌─────────────┐   │
    │  │   OPENAI    │    │   REDDIT/   │      │  SUPABASE   │   │
    │  │  (Optional) │    │   HN APIs   │      │ PostgreSQL  │   │
    │  │             │    │  (Trends)   │      │             │   │
    │  │ GPT-4o-mini │    │             │      │ - voicedna  │   │
    │  │ Embeddings  │    │  Fallback:  │      │ - drafts    │   │
    │  └─────────────┘    │  Curated →  │      │ - stats     │   │
    │                     │  Mock data  │      │ - upvotes   │   │
    │                     └─────────────┘      └─────────────┘   │
    │                                                             │
    │                        EXTERNAL SERVICES                    │
    └─────────────────────────────────────────────────────────────┘
```

---

## Quick Reference: Architecture Interview Answers

### "How do you structure your projects?"
> "I separate concerns into layers. Frontend has pages, components, hooks, and utilities. Backend has routes for HTTP handling, services for business logic, and middleware for cross-cutting concerns. This makes the code testable, maintainable, and allows team members to work on different parts independently."

### "Why separate frontend and backend?"
> "For PlanLift, I separated them because they have different scaling needs - AI processing is compute-intensive and might need different resources than the frontend. It also allows independent deployments - I can update the frontend without touching the backend. However, for simpler apps like SoulThread, I used Next.js API routes to keep everything in one deployment unit."

### "How do you handle errors?"
> "I use multiple layers: try-catch in route handlers, a global error handler middleware that catches anything missed, and custom error classes for operational errors. In production, I mask error details for security while logging full errors server-side. On the frontend, I use Error Boundaries for React errors and try-catch for async operations."

---

Next: [Part-6-Interview-Drills.md](./Part-6-Interview-Drills.md)
