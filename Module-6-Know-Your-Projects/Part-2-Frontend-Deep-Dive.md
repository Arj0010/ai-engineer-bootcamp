# Part 2: Frontend Deep Dive

> **Your projects use**: React 18/19, Next.js 15/16, TypeScript, Tailwind CSS

---

## Table of Contents
1. [What is React?](#1-what-is-react)
2. [Components Explained](#2-components-explained)
3. [Props and State](#3-props-and-state)
4. [React Hooks Deep Dive](#4-react-hooks-deep-dive)
5. [Next.js Explained](#5-nextjs-explained)
6. [TypeScript Basics](#6-typescript-basics)
7. [State Management Patterns](#7-state-management-patterns)
8. [Your Projects' Frontend Architecture](#8-your-projects-frontend-architecture)

---

## 1. What is React?

### The Core Concept

React is a **JavaScript library for building user interfaces** using components.

**Key Ideas:**
- **Component-Based**: Build UIs from small, reusable pieces
- **Declarative**: Describe WHAT you want, not HOW to do it
- **Virtual DOM**: React efficiently updates only what changed

### Traditional vs React

```html
<!-- Traditional JavaScript -->
<div id="counter">0</div>
<button onclick="increment()">+1</button>

<script>
  let count = 0;
  function increment() {
    count++;
    document.getElementById('counter').textContent = count;
    // YOU manage DOM updates manually
  }
</script>
```

```jsx
// React - Declarative
function Counter() {
  const [count, setCount] = useState(0);

  return (
    <div>
      <div>{count}</div>
      <button onClick={() => setCount(count + 1)}>+1</button>
    </div>
  );
  // React handles DOM updates automatically
}
```

### Interview Answer:
> "React is a component-based JavaScript library for building user interfaces. It uses a virtual DOM to efficiently update only the parts of the page that changed, rather than re-rendering everything. In my projects, I used React with Next.js for features like server-side rendering and file-based routing."

---

## 2. Components Explained

### What is a Component?

A component is a **reusable piece of UI** that can have its own logic and styling.

```jsx
// A simple component
function WelcomeMessage({ name }) {
  return <h1>Hello, {name}!</h1>;
}

// Using the component
<WelcomeMessage name="Arjun" />
// Output: <h1>Hello, Arjun!</h1>
```

### Function Components vs Class Components

```jsx
// FUNCTION COMPONENT (Modern - What you used)
function Newsletter({ title, content }) {
  const [likes, setLikes] = useState(0);

  return (
    <article>
      <h1>{title}</h1>
      <p>{content}</p>
      <button onClick={() => setLikes(likes + 1)}>
        Like ({likes})
      </button>
    </article>
  );
}

// CLASS COMPONENT (Old way - Know for interviews)
class Newsletter extends React.Component {
  constructor(props) {
    super(props);
    this.state = { likes: 0 };
  }

  render() {
    return (
      <article>
        <h1>{this.props.title}</h1>
        <p>{this.props.content}</p>
        <button onClick={() => this.setState({ likes: this.state.likes + 1 })}>
          Like ({this.state.likes})
        </button>
      </article>
    );
  }
}
```

### Component Hierarchy in Your Projects

```
SoulThread Component Tree:
App
├── Layout
│   ├── Header
│   │   ├── Logo
│   │   └── Navigation
│   └── Footer
├── Dashboard
│   ├── VoiceProfileCard
│   ├── NewsletterGenerator
│   │   ├── TopicInput
│   │   ├── ToneSelector
│   │   └── GenerateButton
│   └── DraftsList
│       └── DraftCard (repeated)
└── CommunityFeed
    └── PostCard (repeated)
        ├── PostContent
        ├── UpvoteButton
        └── CommentSection
```

---

## 3. Props and State

### Props (Properties)

Props are **inputs to components** - data passed from parent to child.

```jsx
// Parent component passes props
function Dashboard() {
  return (
    <NewsletterCard
      title="AI Trends"
      date="Jan 29, 2026"
      isPublished={true}
    />
  );
}

// Child component receives props
function NewsletterCard({ title, date, isPublished }) {
  return (
    <div className="card">
      <h2>{title}</h2>
      <span>{date}</span>
      {isPublished && <span className="badge">Published</span>}
    </div>
  );
}
```

**Props Rules:**
- Props are **read-only** (cannot be modified by child)
- Props flow **down** (parent to child only)
- Changes to props cause re-render

### State

State is **internal component data** that can change over time.

```jsx
function NewsletterGenerator() {
  // State declaration with useState hook
  const [topic, setTopic] = useState('');           // Form input
  const [isLoading, setIsLoading] = useState(false); // Loading state
  const [newsletter, setNewsletter] = useState(null); // API response

  const handleGenerate = async () => {
    setIsLoading(true);                             // Update state
    const result = await api.generate(topic);
    setNewsletter(result);
    setIsLoading(false);
  };

  return (
    <div>
      <input
        value={topic}
        onChange={(e) => setTopic(e.target.value)}  // Update on change
      />
      <button onClick={handleGenerate} disabled={isLoading}>
        {isLoading ? 'Generating...' : 'Generate'}
      </button>
      {newsletter && <div>{newsletter}</div>}
    </div>
  );
}
```

**State Rules:**
- State is **local** to the component
- State changes trigger **re-renders**
- State updates are **asynchronous**
- Use `setState` function, never modify directly

### Props vs State

| Aspect | Props | State |
|--------|-------|-------|
| **Defined by** | Parent component | Component itself |
| **Mutable** | No (read-only) | Yes (via setState) |
| **Purpose** | Configure component | Track changes |
| **Updates cause** | Re-render | Re-render |

### Interview Question: "What's the difference between props and state?"

> "Props are read-only values passed from parent to child components for configuration, while state is internal mutable data managed within a component. Props flow downward and can't be changed by the receiving component, while state can be updated using setState functions, which triggers a re-render. In my SoulThread project, I passed newsletter data as props to display components, but managed form inputs and API loading states as internal state."

---

## 4. React Hooks Deep Dive

### What are Hooks?

Hooks are **functions that let you use React features** (like state) in function components.

### useState - Managing State

```jsx
// Basic usage
const [count, setCount] = useState(0);
//    ↑value  ↑setter       ↑initial value

// Different data types
const [name, setName] = useState('');           // String
const [items, setItems] = useState([]);         // Array
const [user, setUser] = useState(null);         // Object/null
const [isOpen, setIsOpen] = useState(false);    // Boolean

// Updating state
setCount(5);                    // Direct value
setCount(prev => prev + 1);     // Based on previous (PREFERRED)
setItems([...items, newItem]);  // Add to array (immutable update)
setUser({ ...user, name: 'New' }); // Update object property
```

### useEffect - Side Effects

useEffect runs code **after render** - for API calls, subscriptions, etc.

```jsx
function UserProfile({ userId }) {
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // This runs after component renders
    async function fetchUser() {
      setLoading(true);
      const data = await api.getUser(userId);
      setUser(data);
      setLoading(false);
    }
    fetchUser();
  }, [userId]);  // Dependency array - re-run when userId changes

  if (loading) return <div>Loading...</div>;
  return <div>{user.name}</div>;
}
```

**useEffect Dependency Array:**

```jsx
useEffect(() => {
  // Runs ONCE after first render
}, []);  // Empty array

useEffect(() => {
  // Runs after EVERY render
});  // No array

useEffect(() => {
  // Runs when `value` changes
}, [value]);  // Specific dependency

useEffect(() => {
  // Setup code
  return () => {
    // Cleanup code (runs before next effect or unmount)
  };
}, []);
```

### useContext - Global State

```jsx
// 1. Create context
const ThemeContext = createContext('light');

// 2. Provide context at top level
function App() {
  const [theme, setTheme] = useState('dark');

  return (
    <ThemeContext.Provider value={{ theme, setTheme }}>
      <Dashboard />
    </ThemeContext.Provider>
  );
}

// 3. Use context anywhere in tree
function DeepNestedComponent() {
  const { theme, setTheme } = useContext(ThemeContext);

  return (
    <button onClick={() => setTheme(theme === 'dark' ? 'light' : 'dark')}>
      Current: {theme}
    </button>
  );
}
```

### Your SoulThread Uses Context

```jsx
// Voice profile shared across components
const VoiceProfileContext = createContext(null);

function App() {
  const [voiceProfile, setVoiceProfile] = useState(null);

  return (
    <VoiceProfileContext.Provider value={{ voiceProfile, setVoiceProfile }}>
      <NewsletterGenerator />  {/* Can access voiceProfile */}
      <LinkedInGenerator />    {/* Can access voiceProfile */}
    </VoiceProfileContext.Provider>
  );
}
```

### Other Common Hooks

```jsx
// useRef - Persist value without re-render
const inputRef = useRef(null);
inputRef.current.focus();  // Access DOM element

// useMemo - Cache expensive calculations
const sortedItems = useMemo(() => {
  return items.sort((a, b) => a.date - b.date);
}, [items]);  // Only recalculate when items change

// useCallback - Cache function reference
const handleClick = useCallback(() => {
  console.log('Clicked');
}, []);  // Same function reference across renders
```

### Interview Question: "Explain useEffect and when to use it"

> "useEffect is a hook for handling side effects in functional components - things like API calls, subscriptions, or DOM manipulation that shouldn't happen during render. It takes a callback function and a dependency array. With an empty array, it runs once after mount - perfect for initial API calls. With dependencies, it runs when those values change. I used it extensively in my projects for fetching data on component mount and updating when user selections changed."

---

## 5. Next.js Explained

### What is Next.js?

Next.js is a **React framework** that adds:
- Server-side rendering (SSR)
- Static site generation (SSG)
- File-based routing
- API routes
- Built-in optimizations

### File-Based Routing (App Router)

```
app/
├── page.tsx              → /
├── about/
│   └── page.tsx          → /about
├── dashboard/
│   ├── page.tsx          → /dashboard
│   └── settings/
│       └── page.tsx      → /dashboard/settings
├── blog/
│   └── [slug]/
│       └── page.tsx      → /blog/any-post-title (dynamic)
└── api/
    └── generate/
        └── route.ts      → /api/generate (API endpoint)
```

### Page vs Layout

```tsx
// app/layout.tsx - Wraps ALL pages
export default function RootLayout({ children }) {
  return (
    <html>
      <body>
        <Header />      {/* Shows on every page */}
        {children}      {/* Page content goes here */}
        <Footer />      {/* Shows on every page */}
      </body>
    </html>
  );
}

// app/dashboard/page.tsx - Single page
export default function DashboardPage() {
  return (
    <main>
      <h1>Dashboard</h1>
      {/* Page-specific content */}
    </main>
  );
}
```

### Server vs Client Components

```tsx
// SERVER COMPONENT (Default in Next.js 13+)
// Runs on server, can access database directly
async function ServerComponent() {
  const data = await db.query('SELECT * FROM users');  // Direct DB access!
  return <div>{data.map(user => <p>{user.name}</p>)}</div>;
}

// CLIENT COMPONENT (Add "use client" directive)
// Runs in browser, can use hooks and interactivity
"use client";

function ClientComponent() {
  const [count, setCount] = useState(0);  // Hooks work here

  return (
    <button onClick={() => setCount(count + 1)}>
      Count: {count}
    </button>
  );
}
```

**When to use which:**

| Server Component | Client Component |
|-----------------|------------------|
| Fetching data | Interactive UI (onClick, etc.) |
| Accessing backend resources | Using hooks (useState, useEffect) |
| Keeping secrets secure | Browser APIs (localStorage) |
| Large dependencies | Forms with state |

### Data Fetching in Next.js

```tsx
// SERVER COMPONENT - Direct fetch
async function Page() {
  const res = await fetch('https://api.example.com/data');
  const data = await res.json();

  return <div>{data.title}</div>;
}

// CLIENT COMPONENT - useEffect
"use client";

function Page() {
  const [data, setData] = useState(null);

  useEffect(() => {
    fetch('/api/data')
      .then(res => res.json())
      .then(setData);
  }, []);

  return <div>{data?.title}</div>;
}
```

### Your Next.js Usage

**SoulThread (Next.js 16):**
```
app/
├── page.tsx              # Landing page
├── dashboard/
│   └── page.tsx          # User dashboard (client)
├── community/
│   └── page.tsx          # Community feed
├── api/
│   ├── ai-generate/
│   │   └── route.ts      # Newsletter generation
│   ├── linkedin-generate/
│   │   └── route.ts      # LinkedIn post generation
│   └── voice-profile/
│       └── route.ts      # Voice profile CRUD
└── components/
    ├── NewsletterGenerator.tsx
    └── VoiceProfileCard.tsx
```

### Interview Question: "Why did you choose Next.js over plain React?"

> "I chose Next.js for several reasons: First, the App Router provides intuitive file-based routing without configuration. Second, API routes let me create backend endpoints in the same project, simplifying deployment. Third, server components reduce client-side JavaScript bundle size. Fourth, built-in image optimization improves performance. For SoulThread, having frontend and API in one deployment unit made Vercel deployment seamless."

---

## 6. TypeScript Basics

### Why TypeScript?

TypeScript adds **static type checking** to JavaScript - catching errors before runtime.

```typescript
// JavaScript - Error at RUNTIME
function greet(name) {
  return "Hello " + name.toUpperCase();
}
greet(123);  // Runtime error: toUpperCase is not a function

// TypeScript - Error at COMPILE TIME
function greet(name: string): string {
  return "Hello " + name.toUpperCase();
}
greet(123);  // Compile error: Argument of type 'number' is not assignable
```

### Basic Types

```typescript
// Primitive types
let name: string = "Arjun";
let age: number = 22;
let isStudent: boolean = true;

// Arrays
let topics: string[] = ["AI", "React", "TypeScript"];
let scores: number[] = [85, 90, 78];

// Objects with interface
interface User {
  id: number;
  name: string;
  email: string;
  isActive?: boolean;  // Optional property (?)
}

const user: User = {
  id: 1,
  name: "Arjun",
  email: "arjun@example.com"
};
```

### Function Types

```typescript
// Function with typed parameters and return
function generateNewsletter(topic: string, tone: string): string {
  return `Newsletter about ${topic} in ${tone} tone`;
}

// Arrow function
const fetchUser = async (id: number): Promise<User> => {
  const response = await fetch(`/api/users/${id}`);
  return response.json();
};

// Function as parameter (callback)
function processItems(items: string[], callback: (item: string) => void) {
  items.forEach(callback);
}
```

### React with TypeScript

```tsx
// Typed props
interface NewsletterCardProps {
  title: string;
  content: string;
  publishedAt: Date;
  onDelete: (id: number) => void;
  tags?: string[];  // Optional
}

function NewsletterCard({
  title,
  content,
  publishedAt,
  onDelete,
  tags = []  // Default value
}: NewsletterCardProps) {
  return (
    <article>
      <h2>{title}</h2>
      <p>{content}</p>
      <time>{publishedAt.toLocaleDateString()}</time>
    </article>
  );
}

// Typed state
const [user, setUser] = useState<User | null>(null);
const [items, setItems] = useState<string[]>([]);
const [count, setCount] = useState<number>(0);
```

### Your TypeScript Usage

```typescript
// SoulThread - Voice Profile type
interface VoiceProfile {
  topics: string[];
  tone: 'professional' | 'casual' | 'friendly' | 'authoritative';
  feeling: string;
  writingSample?: string;
}

// API Response type
interface GenerateResponse {
  success: boolean;
  newsletter?: string;
  error?: string;
}

// Component with typed props
interface GeneratorProps {
  voiceProfile: VoiceProfile;
  onGenerate: (result: GenerateResponse) => void;
}

function NewsletterGenerator({ voiceProfile, onGenerate }: GeneratorProps) {
  // TypeScript ensures voiceProfile has correct shape
  console.log(voiceProfile.topics);  // Safe - TypeScript knows this exists
}
```

### Interview Question: "Why use TypeScript?"

> "TypeScript catches errors at compile time rather than runtime, improving code reliability. The type system serves as documentation and enables better IDE support with autocomplete and refactoring tools. In my projects, I defined interfaces for API responses and component props, which prevented many bugs and made the code more maintainable. For example, if I changed a prop name, TypeScript would immediately show all places that needed updating."

---

## 7. State Management Patterns

### Local State (useState)

For state used in **one component**:

```jsx
function Counter() {
  const [count, setCount] = useState(0);
  return <button onClick={() => setCount(count + 1)}>{count}</button>;
}
```

### Lifted State

For state shared by **sibling components**:

```jsx
function Parent() {
  const [selectedTopic, setSelectedTopic] = useState(null);

  return (
    <>
      <TopicSelector onSelect={setSelectedTopic} />
      <NewsletterPreview topic={selectedTopic} />
    </>
  );
}
```

### Context (Your Projects' Pattern)

For state needed **across many components**:

```jsx
// 1. Create context
const AuthContext = createContext(null);

// 2. Provider with state
function AuthProvider({ children }) {
  const [user, setUser] = useState(null);

  const login = async (email, password) => {
    const { data } = await supabase.auth.signInWithPassword({ email, password });
    setUser(data.user);
  };

  const logout = async () => {
    await supabase.auth.signOut();
    setUser(null);
  };

  return (
    <AuthContext.Provider value={{ user, login, logout }}>
      {children}
    </AuthContext.Provider>
  );
}

// 3. Custom hook for easy access
function useAuth() {
  const context = useContext(AuthContext);
  if (!context) throw new Error('useAuth must be within AuthProvider');
  return context;
}

// 4. Use anywhere
function Navbar() {
  const { user, logout } = useAuth();

  return user ? (
    <button onClick={logout}>Logout</button>
  ) : (
    <a href="/login">Login</a>
  );
}
```

### Your SoulThread State Architecture

```
App Level (Context Providers)
├── AuthContext (user session)
├── VoiceProfileContext (user's writing style)
└── ThemeContext (dark/light mode)
    │
    └── Dashboard
        ├── Local State: isGenerating, formInputs
        ├── From Context: voiceProfile, user
        │
        └── NewsletterCard
            ├── Props: newsletter data
            └── Local State: isExpanded
```

---

## 8. Your Projects' Frontend Architecture

### SoulThread Frontend Structure

```
app/
├── layout.tsx              # Root layout with providers
├── page.tsx                # Landing page
├── (auth)/
│   ├── login/page.tsx      # Login page
│   └── register/page.tsx   # Register page
├── dashboard/
│   ├── layout.tsx          # Dashboard layout
│   └── page.tsx            # Main dashboard
├── community/
│   └── page.tsx            # Community feed
└── components/
    ├── NewsletterGenerator.tsx
    ├── VoiceProfileCard.tsx
    ├── DraftCard.tsx
    ├── CommunityPost.tsx
    └── ui/
        ├── Button.tsx
        ├── Input.tsx
        └── Card.tsx
```

### Component Data Flow (SoulThread)

```
                    ┌─────────────────────────────┐
                    │     AuthProvider            │
                    │   (user session state)      │
                    └─────────────┬───────────────┘
                                  │
                    ┌─────────────▼───────────────┐
                    │   VoiceProfileProvider      │
                    │   (writing style state)     │
                    └─────────────┬───────────────┘
                                  │
        ┌─────────────────────────┼─────────────────────────┐
        │                         │                         │
        ▼                         ▼                         ▼
┌───────────────┐       ┌───────────────┐       ┌───────────────┐
│   Dashboard   │       │  Community    │       │   Profile     │
│               │       │               │       │               │
│ - Fetch drafts│       │ - Fetch posts │       │ - Edit voice  │
│ - Generate    │       │ - Upvote      │       │ - View stats  │
│               │       │ - Comment     │       │               │
└───────────────┘       └───────────────┘       └───────────────┘
```

### LAIPath Frontend Flow

```tsx
// User flow: Goal → Syllabus → Daily Learning → Reflection

function LearningFlow() {
  // 1. User enters learning goal
  const [goal, setGoal] = useState('');
  const [syllabus, setSyllabus] = useState(null);
  const [currentDay, setCurrentDay] = useState(1);

  // 2. Generate syllabus
  const generateSyllabus = async () => {
    const response = await fetch('/api/syllabus', {
      method: 'POST',
      body: JSON.stringify({ goal, hoursPerDay: 2 })
    });
    const data = await response.json();
    setSyllabus(data.syllabus);
  };

  // 3. Render based on state
  if (!syllabus) {
    return <GoalInput onSubmit={generateSyllabus} />;
  }

  return (
    <div>
      <Calendar syllabus={syllabus} currentDay={currentDay} />
      <DailyContent day={syllabus[currentDay]} />
      <ReflectionForm onComplete={() => setCurrentDay(d => d + 1)} />
    </div>
  );
}
```

### PlanLift Frontend Flow

```tsx
// User flow: Upload Blueprint → Select Style → Generate → View Result

function PlanLiftApp() {
  const [blueprint, setBlueprint] = useState<File | null>(null);
  const [style, setStyle] = useState('modern');
  const [renderUrl, setRenderUrl] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  const handleUpload = (file: File) => {
    setBlueprint(file);
    setRenderUrl(null);  // Clear previous render
  };

  const handleGenerate = async () => {
    if (!blueprint) return;

    setIsLoading(true);

    const formData = new FormData();
    formData.append('file', blueprint);
    formData.append('style', style);

    const response = await fetch('/api/render', {
      method: 'POST',
      body: formData  // Not JSON - it's form data with file
    });

    const data = await response.json();
    setRenderUrl(data.image_url);
    setIsLoading(false);
  };

  return (
    <div>
      <UploadZone onUpload={handleUpload} file={blueprint} />
      <StyleSelector selected={style} onSelect={setStyle} />
      <button onClick={handleGenerate} disabled={!blueprint || isLoading}>
        {isLoading ? 'Generating...' : 'Generate 3D Render'}
      </button>
      {renderUrl && <RenderPreview url={renderUrl} />}
    </div>
  );
}
```

---

## Quick Reference: Frontend Interview Answers

### "What is the virtual DOM?"
> "The virtual DOM is a lightweight JavaScript representation of the actual DOM. When state changes, React creates a new virtual DOM, compares it with the previous one (diffing), and updates only the changed elements in the real DOM. This makes updates efficient."

### "Explain component lifecycle"
> "In functional components, we use useEffect to handle lifecycle events. An effect with an empty dependency array runs once after mount - like componentDidMount. Returning a cleanup function handles unmounting - like componentWillUnmount. Adding dependencies makes it run when those values change - like componentDidUpdate."

### "What is prop drilling and how do you solve it?"
> "Prop drilling is when you pass props through multiple levels of components that don't use them, just to reach a deeply nested component. I solved this in SoulThread using React Context - I created providers at the app level for auth and voice profile data, so any component can access them directly without drilling."

### "Why did you use TypeScript?"
> "TypeScript provides compile-time type checking, which catches errors before runtime. It also improves developer experience with better autocomplete and refactoring tools. For example, if I changed an API response structure, TypeScript would immediately show all places in my code that needed updating."

---

Next: [Part-3-Database-Mastery.md](./Part-3-Database-Mastery.md)
