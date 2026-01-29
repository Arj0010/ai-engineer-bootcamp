# Interview Cheat Sheet

> **Quick reference - Review 15 minutes before your interview**

---

## Your Projects Summary

| Project | What It Does | Tech Stack | Key Metric |
|---------|--------------|------------|------------|
| **LAIPath** | AI learning system with adaptive syllabi | React + Express + Supabase + OpenAI | 70% API cost reduction |
| **Drug-Likeness** | ML model predicts if compound is drug-like | Flask + TensorFlow + RDKit | ~85% accuracy, ROC-AUC ~0.89 |
| **PlanLift** | 2D blueprints → 3D renders | Next.js + Express + Replicate + Cloudinary | 96% faster than manual |
| **SoulThread** | AI newsletter with voice matching | Next.js + Supabase + OpenAI | $0 cost with template mode |

---

## 30-Second Pitches

### LAIPath
> "An AI-powered learning system that generates personalized syllabi from any goal. Uses GPT-4o-mini for topic-scoped tutoring with embedding-based validation to prevent drift. Dynamically adapts based on reflections."

### Drug-Likeness Predictor
> "Deep learning model that predicts drug-likeness from SMILES notation. Hybrid CNN-BiLSTM architecture trained on 250K molecules, ~85% accuracy. Flask backend with RDKit visualization."

### PlanLift
> "Converts 2D architectural blueprints to 3D renders in minutes. Upload blueprint, select style, get AI-generated visualization. Next.js frontend, Express backend, Replicate AI."

### SoulThread
> "AI newsletter platform that learns your writing voice. Template-based generation works free, optional AI mode available. Community features with gamification."

---

## Key Technologies You Used

### Frontend
| Tech | What It Does |
|------|--------------|
| **React** | Component-based UI library with hooks |
| **Next.js** | React framework with SSR, routing, API routes |
| **TypeScript** | Static type checking for JavaScript |
| **Tailwind CSS** | Utility-first CSS framework |

### Backend
| Tech | What It Does |
|------|--------------|
| **Express.js** | Node.js web framework for APIs |
| **Flask** | Python web framework |
| **Helmet** | Security headers middleware |
| **Multer** | File upload handling |
| **CORS** | Cross-origin request handling |

### Database
| Tech | What It Does |
|------|--------------|
| **PostgreSQL** | Relational database (via Supabase) |
| **Supabase** | Backend-as-a-service (auth + DB + storage) |
| **ChromaDB** | Vector database for embeddings |

### AI/ML
| Tech | What It Does |
|------|--------------|
| **OpenAI API** | GPT-4o-mini for generation, embeddings for similarity |
| **Replicate** | Host open-source AI models as APIs |
| **TensorFlow** | Deep learning framework |
| **RDKit** | Chemistry library for molecules |

---

## SQL Quick Reference

```sql
-- SELECT (Read)
SELECT name, email FROM users WHERE is_active = true;

-- INSERT (Create)
INSERT INTO drafts (title, user_id) VALUES ('My Draft', '123');

-- UPDATE (Modify)
UPDATE drafts SET is_published = true WHERE id = '456';

-- DELETE (Remove)
DELETE FROM drafts WHERE id = '789';

-- JOIN (Combine tables)
SELECT u.name, d.title
FROM users u
JOIN drafts d ON u.id = d.user_id;
```

---

## Common Interview Answers

### "What is REST?"
> Resource-based URLs with HTTP methods (GET/POST/PUT/DELETE). Stateless. I used REST conventions in all my APIs.

### "What is middleware?"
> Functions that run between request and response. I used: helmet (security), cors (cross-origin), rate-limit, multer (uploads).

### "Difference between SQL and NoSQL?"
> SQL: structured schema, JOINs, ACID. NoSQL: flexible schema, embedded data. I chose PostgreSQL for relational data integrity.

### "What is the virtual DOM?"
> JavaScript representation of the real DOM. React diffs virtual DOM to update only changed elements efficiently.

### "Props vs State?"
> Props: read-only inputs from parent. State: internal mutable data. Props flow down, state changes trigger re-renders.

### "What is useEffect?"
> Hook for side effects (API calls, subscriptions). Empty deps = run once. With deps = run when deps change.

### "Why TypeScript?"
> Catch errors at compile time, better IDE autocomplete, serves as documentation. I defined interfaces for API responses.

---

## Your Resume Key Points

### Education
- **St. Joseph's University** - BCA (Data Analytics)
- CGPA: 7.7 (First Class with Distinction)
- 2022-2025

### Experience
| Company | Role | Key Achievement |
|---------|------|-----------------|
| **Oryzed** | AI Intern | LLM workflows with GoQ LLaMA 3.1 |
| **Green Builders** | Data Analyst | Automated workflows, 20% error reduction |
| **Sastic Minds** | Analyst | Data consolidation app for ML pipelines |

### Certifications
- Supervised Machine Learning (Stanford/DeepLearning.AI)
- Advanced Learning Algorithms (Stanford/DeepLearning.AI)
- Excel Skills for Business (Macquarie University)

---

## Key Metrics to Remember

| Metric | Value | Project |
|--------|-------|---------|
| Model accuracy | ~85% | Drug-Likeness |
| ROC-AUC score | ~0.89 | Drug-Likeness |
| API cost reduction | 70% | LAIPath |
| Scope validation accuracy | 99%+ | LAIPath |
| Cosine similarity threshold | 0.22 | LAIPath |
| Rate limit | 100 req/15min | PlanLift |
| File size limit | 10MB | PlanLift |
| Template generation time | <1 second | SoulThread |
| Fallback success rate | 100% | SoulThread |

---

## Why You Chose Each Tech

| Choice | Why |
|--------|-----|
| **Next.js over CRA** | SSR, file routing, API routes, better DX |
| **Express over Fastify** | Larger ecosystem, more middleware, familiar |
| **Supabase over Firebase** | Open source, PostgreSQL, better free tier |
| **TensorFlow over PyTorch** | Better Keras API for sequential models |
| **Vercel over others** | Best Next.js support, edge functions |

---

## Architecture Patterns

### Frontend Structure
```
app/
├── page.tsx          # Pages
├── components/       # Reusable UI
├── lib/              # Utilities
├── hooks/            # Custom hooks
└── api/              # API routes
```

### Backend Structure
```
src/
├── routes/           # HTTP handlers
├── middleware/       # Request processing
├── services/         # Business logic
└── config/           # Configuration
```

### Error Handling
1. Try-catch in route handlers
2. Global error middleware
3. Production error masking
4. Custom error classes

---

## Questions to Ask Them

1. "What does a typical day look like?"
2. "What's the tech stack?"
3. "How does the team handle code reviews?"
4. "What are the biggest challenges right now?"
5. "What does success look like in 6 months?"

---

## Final Checklist

- [ ] Know your 30-second pitch for each project
- [ ] Know 2-3 challenges you faced and solutions
- [ ] Know why you chose each technology
- [ ] Know your metrics (accuracy, cost savings, response times)
- [ ] Have questions ready to ask them
- [ ] Relax and be confident - you built these projects!

---

## Emergency Answers

**"I don't know"**
> "I haven't worked with that specifically, but based on my experience with [similar thing], I would approach it by [logical approach]. I'd also look into [resource] to learn more."

**"Tell me about yourself"**
> "I'm a backend and AI engineer focused on building production-ready AI systems. I recently graduated from St. Joseph's University with a BCA in Data Analytics. I've built several full-stack AI applications including LAIPath, a learning system with adaptive AI tutoring, and I interned at Oryzed working on LLM-powered workflows. I'm excited about this role because [specific reason]."

**"What's your weakness?"**
> "I sometimes dive into coding before fully planning, which can lead to refactoring later. I've been improving by writing out architecture decisions before implementing, especially for complex features."

---

**Good luck! You've got this.**
