# Part 3: Database Mastery

> **Your projects use**: PostgreSQL (via Supabase), SQLite (ChromaDB for vectors)

---

## Table of Contents
1. [What is a Database?](#1-what-is-a-database)
2. [SQL Fundamentals](#2-sql-fundamentals)
3. [PostgreSQL Deep Dive](#3-postgresql-deep-dive)
4. [Supabase Explained](#4-supabase-explained)
5. [Database Design Principles](#5-database-design-principles)
6. [Row Level Security (RLS)](#6-row-level-security-rls)
7. [Indexing & Performance](#7-indexing--performance)
8. [Your Projects' Database Architecture](#8-your-projects-database-architecture)

---

## 1. What is a Database?

### The Filing Cabinet Analogy

```
DATABASE = Digital Filing Cabinet
├── TABLE = Drawer (users, newsletters, drafts)
│   ├── ROW = Single File (one user, one newsletter)
│   └── COLUMN = Field on the file (name, email, date)
└── QUERY = "Find me all files where..."
```

### Types of Databases

| Type | Examples | Best For | Your Usage |
|------|----------|----------|------------|
| **Relational (SQL)** | PostgreSQL, MySQL | Structured data, relationships | Supabase (LAIPath, SoulThread) |
| **Document (NoSQL)** | MongoDB, Firestore | Flexible schemas, JSON | - |
| **Vector** | ChromaDB, Pinecone | AI embeddings, similarity search | RAG project |
| **Key-Value** | Redis | Caching, sessions | - |

### Interview Answer:
> "I used PostgreSQL through Supabase for my projects because it provides relational data modeling with ACID compliance, which is important for user data integrity. For my RAG project, I used ChromaDB, a vector database, to store and search document embeddings for semantic similarity."

---

## 2. SQL Fundamentals

### CRUD Operations

**C**reate, **R**ead, **U**pdate, **D**elete - the four basic operations.

### SELECT - Reading Data

```sql
-- Basic select: Get all columns from users table
SELECT * FROM users;

-- Select specific columns
SELECT name, email FROM users;

-- With condition (WHERE)
SELECT * FROM users WHERE is_active = true;

-- Multiple conditions
SELECT * FROM drafts
WHERE user_id = '123'
  AND is_published = true
  AND created_at > '2025-01-01';

-- Ordering results
SELECT * FROM newsletters
ORDER BY created_at DESC;  -- Newest first

-- Limiting results
SELECT * FROM newsletters
ORDER BY created_at DESC
LIMIT 10;  -- Top 10 only

-- Counting
SELECT COUNT(*) FROM users WHERE is_active = true;

-- Aggregations
SELECT user_id, COUNT(*) as draft_count
FROM drafts
GROUP BY user_id
ORDER BY draft_count DESC;
```

### INSERT - Creating Data

```sql
-- Insert single row
INSERT INTO users (name, email, created_at)
VALUES ('Arjun', 'arjun@example.com', NOW());

-- Insert with returning (get the created row back)
INSERT INTO newsletters (title, content, user_id)
VALUES ('AI Trends', 'Content here...', '123')
RETURNING *;

-- Insert multiple rows
INSERT INTO topics (name) VALUES
  ('AI'),
  ('Machine Learning'),
  ('Backend Development');
```

### UPDATE - Modifying Data

```sql
-- Update single field
UPDATE users
SET name = 'Arjun V'
WHERE id = '123';

-- Update multiple fields
UPDATE drafts
SET is_published = true,
    published_at = NOW()
WHERE id = '456';

-- Update with condition
UPDATE users
SET last_login = NOW()
WHERE email = 'arjun@example.com';

-- IMPORTANT: Always use WHERE clause!
-- Without WHERE, ALL rows get updated
UPDATE users SET is_active = false;  -- DANGEROUS: Updates everyone!
```

### DELETE - Removing Data

```sql
-- Delete specific row
DELETE FROM drafts WHERE id = '789';

-- Delete with condition
DELETE FROM sessions
WHERE expires_at < NOW();  -- Delete expired sessions

-- IMPORTANT: Always use WHERE clause!
-- Without WHERE, ALL rows get deleted
DELETE FROM users;  -- DANGEROUS: Deletes everyone!
```

### Interview Question: "Write a query to get the top 5 users by number of published newsletters"

```sql
SELECT
  u.name,
  u.email,
  COUNT(n.id) as newsletter_count
FROM users u
LEFT JOIN newsletters n ON u.id = n.user_id
WHERE n.is_published = true
GROUP BY u.id, u.name, u.email
ORDER BY newsletter_count DESC
LIMIT 5;
```

---

## 3. PostgreSQL Deep Dive

### Why PostgreSQL?

- **Open Source**: Free and widely used
- **ACID Compliant**: Data integrity guaranteed
- **Rich Features**: JSON support, full-text search, extensions
- **Scalable**: Handles millions of rows
- **Supabase Default**: Your projects use it

### Data Types

```sql
-- Common types used in your projects
CREATE TABLE users (
  -- UUID: Unique identifier (better than auto-increment for distributed systems)
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),

  -- Text types
  name VARCHAR(255),        -- Variable length, max 255 chars
  email TEXT UNIQUE,        -- Unlimited length
  bio TEXT,

  -- Numeric types
  xp_points INTEGER DEFAULT 0,
  completion_rate DECIMAL(5,2),  -- 5 digits, 2 after decimal (99.99)

  -- Boolean
  is_active BOOLEAN DEFAULT true,
  email_verified BOOLEAN DEFAULT false,

  -- Timestamps
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
  updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

  -- JSON (used heavily in your projects)
  voice_profile JSONB,      -- Binary JSON, faster queries
  settings JSON,            -- Regular JSON

  -- Arrays
  topics TEXT[]             -- Array of strings
);
```

### JSONB - Your Projects Use This!

```sql
-- Store complex data in a single column
INSERT INTO voicedna (user_id, data)
VALUES (
  '123',
  '{
    "topics": ["AI", "Backend", "Python"],
    "tone": "professional",
    "feeling": "enthusiastic",
    "writingSample": "I love building..."
  }'::jsonb
);

-- Query JSON fields
SELECT * FROM voicedna
WHERE data->>'tone' = 'professional';

-- Query array in JSON
SELECT * FROM voicedna
WHERE data->'topics' ? 'AI';  -- Contains 'AI'

-- Update JSON field
UPDATE voicedna
SET data = jsonb_set(data, '{tone}', '"casual"')
WHERE user_id = '123';
```

### JOINs - Connecting Tables

```sql
-- INNER JOIN: Only matching rows from both tables
SELECT u.name, d.title
FROM users u
INNER JOIN drafts d ON u.id = d.user_id;
-- Only users WITH drafts appear

-- LEFT JOIN: All rows from left table, matching from right
SELECT u.name, d.title
FROM users u
LEFT JOIN drafts d ON u.id = d.user_id;
-- ALL users appear, even those without drafts (d.title = NULL)

-- Your SoulThread example: Get user with their stats
SELECT
  u.id,
  u.name,
  s.total_drafts,
  s.total_xp,
  s.current_streak
FROM users u
LEFT JOIN user_stats s ON u.id = s.user_id
WHERE u.id = '123';
```

### Interview Question: "Explain the difference between INNER JOIN and LEFT JOIN"

> "INNER JOIN returns only rows that have matching values in both tables - if there's no match, the row is excluded. LEFT JOIN returns all rows from the left table, and if there's no match in the right table, it fills those columns with NULL. In my SoulThread project, I used LEFT JOIN when fetching users with their stats because I wanted to show all users even if they hadn't generated any stats yet."

---

## 4. Supabase Explained

### What is Supabase?

Supabase is an **open-source Firebase alternative** providing:
- **PostgreSQL Database** (hosted and managed)
- **Authentication** (email, OAuth, magic links)
- **Real-time Subscriptions** (live data updates)
- **Storage** (file uploads)
- **Edge Functions** (serverless)

### Why You Used Supabase

```
Traditional Setup:                  Supabase:
┌─────────────────────┐            ┌─────────────────────┐
│ Set up PostgreSQL   │            │                     │
│ Configure auth      │            │  One dashboard      │
│ Build auth APIs     │    vs      │  Instant database   │
│ Set up storage      │            │  Auth included      │
│ Deploy & maintain   │            │  Auto-generated API │
└─────────────────────┘            └─────────────────────┘
     Weeks of work                     Minutes to start
```

### Supabase in Your Code

```typescript
// 1. Initialize Supabase client
import { createClient } from '@supabase/supabase-js';

const supabase = createClient(
  process.env.NEXT_PUBLIC_SUPABASE_URL!,
  process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!
);

// 2. Authentication
// Sign up
const { data, error } = await supabase.auth.signUp({
  email: 'user@example.com',
  password: 'secure-password'
});

// Sign in
const { data, error } = await supabase.auth.signInWithPassword({
  email: 'user@example.com',
  password: 'secure-password'
});

// Get current user
const { data: { user } } = await supabase.auth.getUser();

// Sign out
await supabase.auth.signOut();

// 3. Database Operations (auto-generated API)
// SELECT * FROM drafts WHERE user_id = '123'
const { data, error } = await supabase
  .from('drafts')
  .select('*')
  .eq('user_id', user.id);

// INSERT INTO drafts (title, content, user_id)
const { data, error } = await supabase
  .from('drafts')
  .insert({
    title: 'My Newsletter',
    content: 'Content here...',
    user_id: user.id
  })
  .select();  // Return the inserted row

// UPDATE drafts SET is_published = true WHERE id = '456'
const { data, error } = await supabase
  .from('drafts')
  .update({ is_published: true })
  .eq('id', '456');

// DELETE FROM drafts WHERE id = '789'
const { error } = await supabase
  .from('drafts')
  .delete()
  .eq('id', '789');

// Complex queries
const { data } = await supabase
  .from('drafts')
  .select(`
    id,
    title,
    created_at,
    users (name, email)
  `)  // Join with users table
  .eq('is_published', true)
  .order('created_at', { ascending: false })
  .limit(10);
```

### Interview Question: "Why did you choose Supabase?"

> "I chose Supabase because it provides a complete backend-as-a-service with PostgreSQL, authentication, and real-time capabilities out of the box. For a solo developer building MVPs, it dramatically reduced time-to-market - I didn't need to set up and maintain separate database servers, build authentication systems, or configure hosting. The auto-generated REST and GraphQL APIs meant I could focus on application logic rather than boilerplate."

---

## 5. Database Design Principles

### Normalization

Normalization = Organizing data to **reduce redundancy**.

```sql
-- BAD: Denormalized (repeated data)
CREATE TABLE newsletters (
  id UUID PRIMARY KEY,
  title TEXT,
  content TEXT,
  author_name TEXT,       -- Repeated for every newsletter
  author_email TEXT,      -- Repeated for every newsletter
  author_bio TEXT         -- Repeated for every newsletter
);
-- If author changes email, need to update ALL their newsletters!

-- GOOD: Normalized (separate tables)
CREATE TABLE users (
  id UUID PRIMARY KEY,
  name TEXT,
  email TEXT,
  bio TEXT
);

CREATE TABLE newsletters (
  id UUID PRIMARY KEY,
  title TEXT,
  content TEXT,
  user_id UUID REFERENCES users(id)  -- Just store reference
);
-- Change email once in users table, done!
```

### Foreign Keys

Foreign keys = **References to other tables** that enforce data integrity.

```sql
-- Creating foreign key relationship
CREATE TABLE drafts (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  title TEXT NOT NULL,
  content TEXT,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  --                                          ↑
  --                    If user deleted, their drafts are deleted too
  created_at TIMESTAMP DEFAULT NOW()
);

-- ON DELETE options:
-- CASCADE: Delete related rows
-- SET NULL: Set foreign key to NULL
-- RESTRICT: Prevent deletion if related rows exist
```

### Your SoulThread Database Schema

```sql
-- Users (managed by Supabase Auth)
-- auth.users table is automatic

-- Voice DNA (One per user)
CREATE TABLE voicedna (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  user_id UUID UNIQUE NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
  data JSONB NOT NULL,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
  updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Drafts (Many per user)
CREATE TABLE drafts (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  user_id UUID NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
  title TEXT NOT NULL,
  content TEXT,
  is_published BOOLEAN DEFAULT false,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
  updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- User Stats (One per user)
CREATE TABLE user_stats (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  user_id UUID UNIQUE NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
  total_drafts INTEGER DEFAULT 0,
  total_words INTEGER DEFAULT 0,
  total_xp INTEGER DEFAULT 0,
  current_streak INTEGER DEFAULT 0,
  longest_streak INTEGER DEFAULT 0,
  last_activity_date DATE
);

-- Draft Upvotes (Many-to-many: users can upvote many drafts)
CREATE TABLE draft_upvotes (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  draft_id UUID NOT NULL REFERENCES drafts(id) ON DELETE CASCADE,
  user_id UUID NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
  UNIQUE(draft_id, user_id)  -- One upvote per user per draft
);

-- Comments
CREATE TABLE comments (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  draft_id UUID NOT NULL REFERENCES drafts(id) ON DELETE CASCADE,
  user_id UUID NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
  content TEXT NOT NULL,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);
```

### Entity Relationship Diagram

```
┌──────────────┐       ┌──────────────┐       ┌──────────────┐
│    users     │       │    drafts    │       │   comments   │
├──────────────┤       ├──────────────┤       ├──────────────┤
│ id (PK)      │──┐    │ id (PK)      │──┐    │ id (PK)      │
│ email        │  │    │ user_id (FK) │──│───▶│ draft_id (FK)│
│ name         │  │    │ title        │  │    │ user_id (FK) │
│ ...          │  │    │ content      │  │    │ content      │
└──────────────┘  │    │ is_published │  │    │ created_at   │
                  │    └──────────────┘  │    └──────────────┘
                  │           │          │
                  │           │          │
                  │    ┌──────▼──────┐   │
                  │    │draft_upvotes│   │
                  │    ├─────────────┤   │
                  │    │ id (PK)     │   │
                  └───▶│ user_id (FK)│   │
                       │ draft_id(FK)│◀──┘
                       └─────────────┘

Legend:
PK = Primary Key
FK = Foreign Key
──▶ = References (Foreign Key relationship)
```

---

## 6. Row Level Security (RLS)

### What is RLS?

Row Level Security = **Database-level access control** where you define who can see/modify which rows.

### Why It's Important

```
Without RLS:                         With RLS:
┌────────────────────────┐          ┌────────────────────────┐
│ SELECT * FROM drafts   │          │ SELECT * FROM drafts   │
│ Returns ALL drafts     │          │ Returns ONLY your      │
│ from ALL users!        │    vs    │ drafts automatically   │
│                        │          │                        │
│ Security hole!         │          │ Secure by default      │
└────────────────────────┘          └────────────────────────┘
```

### RLS in Your SoulThread Project

```sql
-- 1. Enable RLS on table
ALTER TABLE drafts ENABLE ROW LEVEL SECURITY;

-- 2. Create policies

-- Users can only SELECT their own drafts
CREATE POLICY "Users can view own drafts"
ON drafts FOR SELECT
USING (auth.uid() = user_id);
--     ↑ Supabase function that returns current user's ID

-- Users can only INSERT drafts for themselves
CREATE POLICY "Users can create own drafts"
ON drafts FOR INSERT
WITH CHECK (auth.uid() = user_id);

-- Users can only UPDATE their own drafts
CREATE POLICY "Users can update own drafts"
ON drafts FOR UPDATE
USING (auth.uid() = user_id);

-- Users can only DELETE their own drafts
CREATE POLICY "Users can delete own drafts"
ON drafts FOR DELETE
USING (auth.uid() = user_id);

-- Public drafts can be viewed by anyone
CREATE POLICY "Anyone can view published drafts"
ON drafts FOR SELECT
USING (is_published = true);
```

### How RLS Works

```
User A requests: SELECT * FROM drafts

1. Request reaches Supabase
2. Supabase identifies user (from auth token)
3. RLS policy is applied automatically:
   SELECT * FROM drafts WHERE user_id = 'user_a_id'
4. Only User A's drafts are returned

User B trying same query gets only their drafts!
```

### Interview Question: "How did you secure user data in your database?"

> "I implemented Row Level Security in Supabase. RLS enforces access control at the database level, not just the application level. I created policies so users can only read, update, and delete their own data. For example, even if there's a bug in my frontend code that requests all drafts, the database will only return drafts belonging to the authenticated user. This is defense in depth - security isn't just in the application layer."

---

## 7. Indexing & Performance

### What is an Index?

An index is like a **book's index** - helps find data quickly without scanning every page.

```sql
-- Without index: Database scans ALL rows
SELECT * FROM drafts WHERE user_id = '123';
-- Checks row 1, row 2, row 3... row 1,000,000 (SLOW)

-- With index: Database jumps directly to matching rows
CREATE INDEX idx_drafts_user_id ON drafts(user_id);
SELECT * FROM drafts WHERE user_id = '123';
-- Jumps directly to user's drafts (FAST)
```

### When to Create Indexes

```sql
-- 1. Columns used in WHERE clauses
CREATE INDEX idx_drafts_user_id ON drafts(user_id);
-- For: SELECT * FROM drafts WHERE user_id = '...'

-- 2. Columns used in JOIN conditions
CREATE INDEX idx_comments_draft_id ON comments(draft_id);
-- For: JOIN comments ON drafts.id = comments.draft_id

-- 3. Columns used in ORDER BY
CREATE INDEX idx_drafts_created_at ON drafts(created_at DESC);
-- For: ORDER BY created_at DESC

-- 4. Unique constraints (automatically indexed)
-- PRIMARY KEY and UNIQUE create indexes automatically
```

### Index Trade-offs

| Benefit | Cost |
|---------|------|
| Faster reads (SELECT) | Slower writes (INSERT/UPDATE) |
| Efficient sorting | Storage space |
| Quick lookups | Maintenance overhead |

### Your Projects Should Have These Indexes

```sql
-- SoulThread indexes
CREATE INDEX idx_drafts_user_id ON drafts(user_id);
CREATE INDEX idx_drafts_is_published ON drafts(is_published);
CREATE INDEX idx_comments_draft_id ON comments(draft_id);
CREATE INDEX idx_upvotes_draft_id ON draft_upvotes(draft_id);

-- LAIPath indexes
CREATE INDEX idx_syllabus_user_id ON syllabus(user_id);
CREATE INDEX idx_reflections_day_id ON reflections(day_id);
```

### Interview Question: "How do you optimize database queries?"

> "I use several strategies: First, I add indexes on columns frequently used in WHERE clauses and JOINs. Second, I use EXPLAIN ANALYZE to understand query execution plans and identify slow operations. Third, I select only needed columns instead of SELECT *. Fourth, I use pagination with LIMIT and OFFSET for large datasets. In SoulThread, I indexed user_id on the drafts table which improved query performance significantly for user-specific queries."

---

## 8. Your Projects' Database Architecture

### LAIPath Database Flow

```
User creates learning goal
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│  SUPABASE POSTGRESQL                                     │
│                                                          │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │   users     │    │  syllabus   │    │ reflections │  │
│  │             │◄───│             │◄───│             │  │
│  │ id          │    │ id          │    │ id          │  │
│  │ email       │    │ user_id     │    │ syllabus_id │  │
│  │ created_at  │    │ goal        │    │ day_number  │  │
│  └─────────────┘    │ days (JSON) │    │ content     │  │
│                     │ current_day │    │ eval (JSON) │  │
│                     └─────────────┘    └─────────────┘  │
│                                                          │
│  RLS: Users can only access their own syllabus          │
└─────────────────────────────────────────────────────────┘
```

### SoulThread Database Flow

```
User writes newsletter
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│  SUPABASE POSTGRESQL                                     │
│                                                          │
│  auth.users ──┬──► voicedna (1:1)                       │
│               │                                          │
│               ├──► drafts (1:many)                       │
│               │      │                                   │
│               │      ├──► draft_upvotes (many:many)     │
│               │      │                                   │
│               │      └──► comments (1:many)              │
│               │                                          │
│               └──► user_stats (1:1)                      │
│                                                          │
│  RLS Policies:                                           │
│  - Users see own voicedna, drafts                       │
│  - Anyone can see published drafts                       │
│  - Anyone can upvote/comment on published drafts        │
└─────────────────────────────────────────────────────────┘
```

### ChromaDB (RAG Project) - Vector Database

```python
# ChromaDB stores embeddings for semantic search
import chromadb

# Initialize
client = chromadb.Client()
collection = client.create_collection("projects")

# Store document with embedding
collection.add(
    documents=["LAIPath is an AI learning platform..."],
    metadatas=[{"project": "LAIPath", "type": "description"}],
    ids=["laipath-desc"]
)
# ChromaDB automatically generates embeddings!

# Query by semantic similarity
results = collection.query(
    query_texts=["What projects use AI for learning?"],
    n_results=3
)
# Returns documents semantically similar to query
```

---

## Quick Reference: Database Interview Answers

### "What is ACID?"
> "ACID stands for Atomicity (transactions complete fully or not at all), Consistency (data remains valid after transactions), Isolation (concurrent transactions don't interfere), and Durability (committed data survives crashes). PostgreSQL, which I used through Supabase, is ACID compliant."

### "Explain normalization"
> "Normalization is organizing database tables to reduce data redundancy. Instead of storing author details in every newsletter row, I create a separate users table and reference it with a foreign key. This means if a user updates their name, I only change it in one place."

### "What's the N+1 query problem?"
> "N+1 is when you fetch N records, then make N additional queries for related data. For example, fetching 10 drafts then querying comments for each one = 11 queries. I solve this by using JOINs or Supabase's nested select syntax to fetch related data in one query."

### "How does your database handle concurrent users?"
> "PostgreSQL handles concurrency through MVCC (Multi-Version Concurrency Control). Each transaction sees a consistent snapshot of data, and conflicts are resolved at commit time. Combined with proper indexing and connection pooling in Supabase, my apps can handle many concurrent users."

---

Next: [Part-4-Libraries-Encyclopedia.md](./Part-4-Libraries-Encyclopedia.md)
